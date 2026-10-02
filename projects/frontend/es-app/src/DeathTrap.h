#pragma once

/*!
 * @brief Install async-signal-safe handlers for fatal program-error signals
 * (SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGSYS).
 *
 * On fault, the handler logs signal name, decoded si_code cause, fault
 * address, PID/TID and a backtrace to the frontend log file (with stderr
 * as fallback), then lets SA_RESETHAND restore the default disposition so
 * the kernel can still produce a core dump.
 *
 * In DEBUG builds this is a no-op: Google sanitizers (asan/ubsan) already
 * produce richer reports for these faults.
 */
void InstallDeathTrap();
