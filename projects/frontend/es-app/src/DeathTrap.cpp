#include "DeathTrap.h"

#ifndef DEBUG

#include <utils/Log.h>

#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <sys/ucontext.h>

#ifdef USE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

namespace
{
  // Dedicated stack for the crash handler so SIGSEGV from stack overflow
  // can still be reported instead of looping on the same exhausted stack.
  constexpr int sAltStackSize = 64 * 1024;
  alignas(16) char sAltStack[sAltStackSize];

  // Snapshot of the log file path captured at install time. We cannot safely
  // touch String/Path/Mutex from a signal handler, so we keep a plain C buffer.
  constexpr int sLogPathMax = 512;
  char sLogPath[sLogPathMax] = {};

  // GNU build-id of the main executable, captured at install time. Logged on
  // every crash so the log can be matched to the exact binary that produced
  // the addresses (essential when looking up symbols in an offline .debug file).
  constexpr int sBuildIdMax = 32;
  unsigned char sBuildId[sBuildIdMax] = {};
  size_t sBuildIdLen = 0;

  // ---- async-signal-safe primitives (no malloc, no stdio, no C++ objects) --

  void SafeWrite(int fd, const char* s, size_t n)
  {
    while (n > 0)
    {
      ssize_t w = write(fd, s, n);
      if (w <= 0) break;
      n -= (size_t)w;
      s += (size_t)w;
    }
  }

  void SafeWriteStr(int fd, const char* s) { SafeWrite(fd, s, strlen(s)); }

  void SafeWriteDec(int fd, long long v)
  {
    char buf[24];
    int i = (int)sizeof(buf);
    bool neg = v < 0;
    unsigned long long u = neg ? (unsigned long long)(-(v + 1)) + 1ULL : (unsigned long long)v;
    do { buf[--i] = (char)('0' + (int)(u % 10)); u /= 10; } while (u != 0 && i > 0);
    if (neg && i > 0) buf[--i] = '-';
    SafeWrite(fd, buf + i, sizeof(buf) - (size_t)i);
  }

  void SafeWriteHex(int fd, unsigned long long v)
  {
    static const char hex[] = "0123456789abcdef";
    char buf[20];
    int i = (int)sizeof(buf);
    do { buf[--i] = hex[v & 0xFu]; v >>= 4; } while (v != 0 && i > 0);
    if (i > 0) buf[--i] = 'x';
    if (i > 0) buf[--i] = '0';
    SafeWrite(fd, buf + i, sizeof(buf) - (size_t)i);
  }

  // Continuous hex dump without the "0x" prefix — for build-id, hashes, ...
  void SafeWriteHexBytes(int fd, const unsigned char* bytes, size_t n)
  {
    static const char hex[] = "0123456789abcdef";
    char buf[80];
    if (n > sizeof(buf) / 2) n = sizeof(buf) / 2;
    for (size_t i = 0; i < n; ++i)
    {
      buf[i * 2]     = hex[bytes[i] >> 4];
      buf[i * 2 + 1] = hex[bytes[i] & 0xFu];
    }
    SafeWrite(fd, buf, n * 2);
  }

  // ---- ucontext register dump ----------------------------------------------

  // si_addr is the *faulting data address* (what SEGV touched, when available),
  // while ucontext->pc is the *faulting instruction* — the bug location itself.
  // Both answer different questions and are both worth logging.
  void SafeWriteUContext(int fd, void* ctx)
  {
    if (ctx == nullptr) return;
    auto* uc = static_cast<ucontext_t*>(ctx);
#if defined(__aarch64__)
    SafeWriteStr(fd, "[DEATHTRAP] Fault PC  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.pc);
    SafeWriteStr(fd, "\n[DEATHTRAP] Fault LR  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.regs[30]);
    SafeWriteStr(fd, "\n[DEATHTRAP] Fault SP  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.sp);
    SafeWriteStr(fd, "\n");
#elif defined(__x86_64__)
    SafeWriteStr(fd, "[DEATHTRAP] Fault PC  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.gregs[REG_RIP]);
    SafeWriteStr(fd, "\n[DEATHTRAP] Fault SP  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.gregs[REG_RSP]);
    SafeWriteStr(fd, "\n[DEATHTRAP] Fault BP  : ");
    SafeWriteHex(fd, (unsigned long long)uc->uc_mcontext.gregs[REG_RBP]);
    SafeWriteStr(fd, "\n");
#endif
  }

  // ---- /proc/self/maps dump (executable mappings only) ---------------------

  // Logs every currently mapped executable region so the absolute IPs in the
  // backtrace can be converted back to file offsets offline:
  //   addr2line -e <binary>.debug $((IP - module_load_base))
  // For PIE binaries (modern default) this is the only way to symbolize a
  // crash from a separate .debug companion file.
  void SafeWriteMaps(int fd)
  {
    int mapsFd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (mapsFd < 0) return;

    SafeWriteStr(fd, "[DEATHTRAP] Modules   :\n");

    char buf[8192];
    size_t held = 0;
    int written = 0;
    constexpr int sMaxLines = 64;

    while (written < sMaxLines)
    {
      ssize_t r = read(mapsFd, buf + held, sizeof(buf) - held);
      if (r <= 0) break;
      size_t end = held + (size_t)r;

      size_t lineStart = 0;
      for (size_t i = 0; i < end; ++i)
      {
        if (buf[i] != '\n') continue;
        const char* line = buf + lineStart;
        size_t len = i - lineStart;
        // Permissions are the 4 chars immediately after the first space.
        const char* sp = (const char*)memchr(line, ' ', len);
        if (sp != nullptr && (size_t)(sp - line) + 4 < len && sp[3] == 'x')
        {
          SafeWriteStr(fd, "[DEATHTRAP]   ");
          SafeWrite(fd, line, len);
          SafeWriteStr(fd, "\n");
          if (++written >= sMaxLines) break;
        }
        lineStart = i + 1;
      }

      if (lineStart < end) { held = end - lineStart; memmove(buf, buf + lineStart, held); }
      else                 { held = 0; }
      if (held >= sizeof(buf)) held = 0; // drop pathological too-long line
    }

    close(mapsFd);
  }

  // ---- frame collection ----------------------------------------------------

  // Collects up to maxFrames return addresses from the current call stack.
  // When libunwind is compiled in AND a signal ucontext is available, walks
  // the stack starting from the faulting instruction itself — no handler
  // trampoline frames in the result, even for nested signal re-raises.
  // Otherwise falls back to libgcc's backtrace(), which starts inside the
  // handler and leaves FindGuiltyFrame to locate the real crash site.
  // Returns the number of frames written and sets startsAtFault to true iff
  // frames[0] is the faulting instruction.
  int CollectFrames(void** frames, int maxFrames, void* ctx, bool& startsAtFault)
  {
    startsAtFault = false;
#ifdef USE_LIBUNWIND
    // unw_context_t is binary-compatible with ucontext_t on glibc Linux for
    // x86_64 and aarch64 — the signal handler's ctx pointer is reusable as-is.
    if (ctx != nullptr)
    {
      unw_cursor_t cur;
      if (unw_init_local(&cur, (unw_context_t*)ctx) == 0)
      {
        int n = 0;
        do {
          unw_word_t ip = 0;
          if (unw_get_reg(&cur, UNW_REG_IP, &ip) != 0) break;
          frames[n++] = (void*)(uintptr_t)ip;
        } while (n < maxFrames && unw_step(&cur) > 0);
        startsAtFault = true;
        return n;
      }
    }
#else
    (void)ctx;
#endif
    return backtrace(frames, maxFrames);
  }

  // ---- frame symbolization with guilty-frame marker ------------------------

  // The kernel pushes a sigreturn trampoline frame as the "return address" of
  // any signal handler, so the frame immediately *after* the last sigreturn is
  // the instruction the kernel interrupted. With nested re-raises (e.g. SDL
  // catching a signal then calling pthread_kill), there are several sigreturn
  // frames; the last one bounds the deepest live frame — the real bug site.
  // dladdr is not POSIX-async-signal-safe but does not allocate in practice;
  // all major crash handlers (gperftools, breakpad, ...) rely on it.
  int FindGuiltyFrame(void** frames, int count)
  {
    int lastSigreturn = -1;
    for (int i = 0; i < count; ++i)
    {
      Dl_info di;
      if (dladdr(frames[i], &di) == 0) continue;
      if (di.dli_sname == nullptr) continue;
      if (strstr(di.dli_sname, "sigreturn") != nullptr) lastSigreturn = i;
    }
    if (lastSigreturn >= 0 && lastSigreturn + 1 < count) return lastSigreturn + 1;
    return count > 1 ? 1 : 0; // fallback: skip just our own handler frame
  }

  // Writes one line per frame in the format
  //   module(symbol+0xoffset)[0xabsolute_ip]
  // with a "> " marker on the guilty frame. Symbol names stay mangled (no
  // async-signal-safe demangler exists); pipe the log through c++filt offline.
  void SafeWriteFrames(int fd, void** frames, int count, int guiltyIdx)
  {
    for (int i = 0; i < count; ++i)
    {
      SafeWriteStr(fd, i == guiltyIdx ? "[DEATHTRAP] > " : "[DEATHTRAP]   ");

      Dl_info di;
      bool resolved = dladdr(frames[i], &di) != 0;
      if (resolved && di.dli_fname != nullptr)
      {
        SafeWriteStr(fd, di.dli_fname);
        if (di.dli_sname != nullptr)
        {
          SafeWriteStr(fd, "(");
          SafeWriteStr(fd, di.dli_sname);
          SafeWriteStr(fd, "+");
          SafeWriteHex(fd, (unsigned long long)((uintptr_t)frames[i] - (uintptr_t)di.dli_saddr));
          SafeWriteStr(fd, ")");
        }
        else if (di.dli_fbase != nullptr)
        {
          SafeWriteStr(fd, "(+");
          SafeWriteHex(fd, (unsigned long long)((uintptr_t)frames[i] - (uintptr_t)di.dli_fbase));
          SafeWriteStr(fd, ")");
        }
      }
      SafeWriteStr(fd, "[");
      SafeWriteHex(fd, (unsigned long long)(uintptr_t)frames[i]);
      SafeWriteStr(fd, "]\n");
    }
  }

  // ---- signal decoders (string literals only) ------------------------------

  const char* DeathTrapSignalName(int sig)
  {
    switch (sig)
    {
      case SIGSEGV: return "SIGSEGV";
      case SIGBUS:  return "SIGBUS";
      case SIGFPE:  return "SIGFPE";
      case SIGILL:  return "SIGILL";
      case SIGSYS:  return "SIGSYS";
      default:      return "UNKNOWN";
    }
  }

  const char* DeathTrapCodeName(int sig, int code)
  {
    // Generic origin codes — a signal delivered via kill(2)/raise(3),
    // sigqueue(3), a POSIX timer, etc. carries one of these in si_code
    // regardless of the signal number. Their value space (<= 0, or 0x80
    // for SI_KERNEL) is disjoint from signal-specific codes (1..15), so
    // matching them before the per-signal switch is unambiguous.
    switch (code)
    {
      case SI_USER:    return "sent by kill(2)/raise(3)";
      case SI_KERNEL:  return "sent by the kernel";
      case SI_QUEUE:   return "sent by sigqueue(3)";
      case SI_TIMER:   return "POSIX timer expired";
      case SI_MESGQ:   return "POSIX message queue state changed";
      case SI_ASYNCIO: return "asynchronous I/O completed";
      case SI_SIGIO:   return "queued SIGIO";
      case SI_TKILL:   return "sent by tkill(2)/tgkill(2)";
      default: break;
    }

    switch (sig)
    {
      case SIGSEGV:
        switch (code)
        {
          case SEGV_MAPERR:   return "address not mapped to object";
          case SEGV_ACCERR:   return "invalid permissions for mapped object";
#ifdef SEGV_BNDERR
          case SEGV_BNDERR:   return "failed address bound checks";
#endif
#ifdef SEGV_PKUERR
          case SEGV_PKUERR:   return "failed protection key checks";
#endif
#ifdef SEGV_ACCADI
          case SEGV_ACCADI:   return "ADI not enabled for mapped object";
#endif
#ifdef SEGV_ADIDERR
          case SEGV_ADIDERR:  return "disrupting MCD error";
#endif
#ifdef SEGV_ADIPERR
          case SEGV_ADIPERR:  return "precise MCD exception";
#endif
#ifdef SEGV_MTEAERR
          case SEGV_MTEAERR:  return "asynchronous ARM MTE tag check fault";
#endif
#ifdef SEGV_MTESERR
          case SEGV_MTESERR:  return "synchronous ARM MTE tag check fault";
#endif
#ifdef SEGV_CPERR
          case SEGV_CPERR:    return "control protection fault";
#endif
          default:            return "unknown SIGSEGV cause";
        }
      case SIGBUS:
        switch (code)
        {
          case BUS_ADRALN:     return "invalid address alignment";
          case BUS_ADRERR:     return "non-existent physical address";
          case BUS_OBJERR:     return "object-specific hardware error";
#ifdef BUS_MCEERR_AR
          case BUS_MCEERR_AR:  return "hardware memory error (action required)";
#endif
#ifdef BUS_MCEERR_AO
          case BUS_MCEERR_AO:  return "hardware memory error (action optional)";
#endif
          default:             return "unknown SIGBUS cause";
        }
      case SIGFPE:
        switch (code)
        {
          case FPE_INTDIV:    return "integer divide by zero";
          case FPE_INTOVF:    return "integer overflow";
          case FPE_FLTDIV:    return "floating-point divide by zero";
          case FPE_FLTOVF:    return "floating-point overflow";
          case FPE_FLTUND:    return "floating-point underflow";
          case FPE_FLTRES:    return "floating-point inexact result";
          case FPE_FLTINV:    return "floating-point invalid operation";
          case FPE_FLTSUB:    return "subscript out of range";
#ifdef FPE_FLTUNK
          case FPE_FLTUNK:    return "undiagnosed floating-point exception";
#endif
#ifdef FPE_CONDTRAP
          case FPE_CONDTRAP:  return "trap on condition";
#endif
          default:            return "unknown SIGFPE cause";
        }
      case SIGILL:
        switch (code)
        {
          case ILL_ILLOPC:    return "illegal opcode";
          case ILL_ILLOPN:    return "illegal operand";
          case ILL_ILLADR:    return "illegal addressing mode";
          case ILL_ILLTRP:    return "illegal trap";
          case ILL_PRVOPC:    return "privileged opcode";
          case ILL_PRVREG:    return "privileged register";
          case ILL_COPROC:    return "coprocessor error";
          case ILL_BADSTK:    return "internal stack error";
#ifdef ILL_BADIADDR
          case ILL_BADIADDR:  return "unimplemented instruction address";
#endif
          default:            return "unknown SIGILL cause";
        }
      case SIGSYS:
        switch (code)
        {
#ifdef SYS_SECCOMP
          case SYS_SECCOMP:        return "seccomp filter triggered";
#endif
#ifdef SYS_USER_DISPATCH
          case SYS_USER_DISPATCH:  return "syscall user dispatch triggered";
#endif
          default:                 return "bad system call";
        }
      default:     return "unknown cause";
    }
  }

  // ---- build-id capture (called once at install time, never from handler) ---

  // Iterates the program headers of the main executable looking for a PT_NOTE
  // segment containing a GNU build-id note, and caches the bytes for the
  // handler. Not called from signal context — uses dl_iterate_phdr freely.
  int CaptureBuildIdCallback(struct dl_phdr_info* info, size_t /*size*/, void* /*data*/)
  {
    for (int i = 0; i < info->dlpi_phnum; ++i)
    {
      if (info->dlpi_phdr[i].p_type != PT_NOTE) continue;
      const char* notes = (const char*)(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
      size_t notesSz = info->dlpi_phdr[i].p_memsz;
      size_t off = 0;
      while (off + sizeof(ElfW(Nhdr)) <= notesSz)
      {
        const auto* nh = (const ElfW(Nhdr)*)(notes + off);
        size_t nameAligned = (nh->n_namesz + 3u) & ~3u;
        size_t descAligned = (nh->n_descsz + 3u) & ~3u;
        const char* name = notes + off + sizeof(ElfW(Nhdr));
        const char* desc = name + nameAligned;
        if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4
            && memcmp(name, "GNU", 4) == 0)
        {
          size_t copyLen = nh->n_descsz < sBuildIdMax ? nh->n_descsz : sBuildIdMax;
          memcpy(sBuildId, desc, copyLen);
          sBuildIdLen = copyLen;
          return 1;
        }
        off += sizeof(ElfW(Nhdr)) + nameAligned + descAligned;
      }
    }
    return 1; // stop after main exe (always the first entry in iteration)
  }

  // ---- handler -------------------------------------------------------------

  void DeathTrapHandler(int sig, siginfo_t* info, void* ctx)
  {
    // Re-entrancy guard: a second fault inside the handler must not loop.
    // SA_RESETHAND also restores SIG_DFL so a follow-up fault produces a core dump.
    static volatile sig_atomic_t entered = 0;
    if (entered != 0) _exit(128 + sig);
    entered = 1;

    int fd = open(sLogPath, O_CREAT | O_APPEND | O_WRONLY, 0666);
    int outFd = (fd >= 0) ? fd : 2; // fall back to stderr if log file unavailable

    timespec ts {};
    clock_gettime(CLOCK_REALTIME, &ts);

    SafeWriteStr(outFd, "\n[");
    SafeWriteDec(outFd, (long long)ts.tv_sec);
    SafeWriteStr(outFd, ".");
    long ms = ts.tv_nsec / 1000000;
    if (ms < 100) SafeWriteStr(outFd, "0");
    if (ms < 10)  SafeWriteStr(outFd, "0");
    SafeWriteDec(outFd, (long long)ms);
    SafeWriteStr(outFd, "] (FATAL) [DEATHTRAP] Signal ");
    SafeWriteStr(outFd, DeathTrapSignalName(sig));
    SafeWriteStr(outFd, " trapped !\n[DEATHTRAP] Cause     : ");
    SafeWriteStr(outFd, DeathTrapCodeName(sig, info != nullptr ? info->si_code : 0));

    // For sender-based deliveries (kill/tkill/sigqueue/timer/...), si_addr is
    // repurposed: si_pid/si_uid identify the sender, si_addr is meaningless.
    // SI_KERNEL (hardware trap) keeps si_addr as the faulting memory address.
    if (info != nullptr && info->si_code <= 0)
    {
      SafeWriteStr(outFd, "\n[DEATHTRAP] Sender    : PID=");
      SafeWriteDec(outFd, (long long)info->si_pid);
      SafeWriteStr(outFd, " UID=");
      SafeWriteDec(outFd, (long long)info->si_uid);
    }
    else
    {
      SafeWriteStr(outFd, "\n[DEATHTRAP] Fault @   : ");
      SafeWriteHex(outFd, info != nullptr ? (unsigned long long)(uintptr_t)info->si_addr : 0ULL);
    }
    SafeWriteStr(outFd, "\n");

    SafeWriteUContext(outFd, ctx);

    SafeWriteStr(outFd, "[DEATHTRAP] PID/TID   : ");
    SafeWriteDec(outFd, (long long)getpid());
    SafeWriteStr(outFd, "/");
    SafeWriteDec(outFd, (long long)gettid());
    SafeWriteStr(outFd, "\n");

    if (sBuildIdLen > 0)
    {
      SafeWriteStr(outFd, "[DEATHTRAP] Build-ID  : ");
      SafeWriteHexBytes(outFd, sBuildId, sBuildIdLen);
      SafeWriteStr(outFd, "\n");
    }

    SafeWriteMaps(outFd);

    SafeWriteStr(outFd, "[DEATHTRAP] Backtrace :\n");
    void* frames[64];
    bool startsAtFault = false;
    int frameCount = CollectFrames(frames, 64, ctx, startsAtFault);
    int guilty = startsAtFault ? 0 : FindGuiltyFrame(frames, frameCount);
    SafeWriteFrames(outFd, frames, frameCount, guilty);
    SafeWriteStr(outFd, "[DEATHTRAP] (end of backtrace)\n");

    if (fd >= 0) { fsync(fd); close(fd); }

    // Returning from the handler re-runs the faulting instruction; SA_RESETHAND
    // restored the default disposition so a core dump is produced. _exit is the
    // last-resort fallback for non-restartable cases (e.g. SIGSYS).
    _exit(128 + sig);
  }

  // ---- shared install helpers (no Log/String/Path/Mutex dependency) --------

  // Copies a log path into the handler's plain-C buffer. Safe to call from a
  // pre-main constructor: it touches only a static char array, never a C++
  // object whose own constructor might not have run yet.
  void SetLogPath(const char* path)
  {
    if (path == nullptr) return;
    size_t n = strlen(path);
    if (n >= sLogPathMax) n = sLogPathMax - 1;
    memcpy(sLogPath, path, n);
    sLogPath[n] = '\0';
  }

  // Arms the sigaltstack + sigaction handlers. Idempotent and free of any
  // LOG()/String/Path/Mutex use, so it is safe to run during early static
  // initialization. The one-shot warmup (libgcc/libunwind priming + build-id
  // capture) runs only once; the sigaction arming runs on every call so
  // InstallDeathTrap() can re-arm after a crash already disarmed the handler
  // through SA_RESETHAND. Failures are reported straight to stderr because the
  // Log subsystem may not exist yet at this point.
  void ArmHandlers()
  {
    static volatile sig_atomic_t warmedUp = 0;
    if (warmedUp == 0)
    {
      warmedUp = 1;

      // Force libgcc unwinder to load now so backtrace() in the handler doesn't
      // trigger dlopen/malloc on first use. The dynamic loader is fully usable
      // before .init_array runs, and we are still single-threaded here.
      void* warmup[4];
      (void)backtrace(warmup, 4);

#ifdef USE_LIBUNWIND
      // Same idea for libunwind: prime its lazy initialization out of band.
      {
        unw_context_t uctx;
        unw_cursor_t ucur;
        if (unw_getcontext(&uctx) == 0) (void)unw_init_local(&ucur, &uctx);
      }
#endif

      // Cache the main executable's GNU build-id so each crash log carries the
      // fingerprint of the binary that produced it (needed to find the matching
      // .debug companion file when symbolizing offline).
      dl_iterate_phdr(&CaptureBuildIdCallback, nullptr);

      stack_t altStack {};
      altStack.ss_sp = sAltStack;
      altStack.ss_size = sAltStackSize;
      altStack.ss_flags = 0;
      if (sigaltstack(&altStack, nullptr) != 0)
        SafeWriteStr(2, "[DEATHTRAP] sigaltstack() failed, stack-overflow crashes won't be reported\n");
    }

    struct sigaction sa {};
    sa.sa_sigaction = &DeathTrapHandler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);

    // Only trap program-error signals. SIGABRT is intentionally NOT trapped:
    // LOG(LogFatal) elsewhere calls abort() and would re-enter the handler.
    // Quit/kill signals (SIGTERM, SIGINT, SIGQUIT, SIGHUP, SIGUSR*) are not crashes.
    static const int trapped[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGSYS };
    for (int s : trapped)
      if (sigaction(s, &sa, nullptr) != 0)
      {
        SafeWriteStr(2, "[DEATHTRAP] Failed to install handler for signal ");
        SafeWriteDec(2, s);
        SafeWriteStr(2, "\n");
      }
  }
}

// Installed as early as the toolchain allows — before any C++ global object
// constructor runs — so a crash *during* static initialization (the classic
// "no log, dies before main()" failure) is still trapped and logged.
//
// Priority 101 is the lowest value available to user code (0-100 are reserved
// for the implementation). The linker places this in section .init_array.00101,
// which the C runtime executes ahead of every default-priority constructor
// (i.e. every ordinary file-scope / class-static object) across all translation
// units. So the handler is armed before the rest of the program initializes.
//
// Log::PathChars() is safe to call this early: the paths are lazily constructed
// on first use (Log::Paths(), a Meyers singleton), so there is no static-
// initialization-order dependency to worry about.
__attribute__((constructor(101)))
static void DeathTrapEarlyInstall()
{
  SetLogPath(Log::PathChars(LogType::Generic));
  ArmHandlers();
}

void InstallDeathTrap()
{
  // Re-capture the path (it may have been overridden at runtime via SetPath) and
  // re-arm: the early install disarms itself via SA_RESETHAND on any crash it
  // already handled during static initialization.
  SetLogPath(Log::PathChars(LogType::Generic));
  ArmHandlers();
}

#else  // DEBUG

// In DEBUG builds, Google sanitizers (asan/ubsan) handle fatal signals with
// richer output, so we stay out of the way.
void InstallDeathTrap() {}

#endif
