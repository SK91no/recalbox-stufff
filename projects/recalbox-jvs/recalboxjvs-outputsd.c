// recalboxjvs-outputsd — Flycast / MAME NetworkOutput → JVS firmware forwarder.
//
//   recalboxjvs-outputsd                 (no args)  daemon mode
//   recalboxjvs-outputsd arm <rom> <core>          a game just started (ES hook)
//   recalboxjvs-outputsd disarm                    the game ended (ES hook)
//
// The daemon connects to localhost:8000 (Flycast/MAME NetworkOutput), parses
// "<name> = <value>" text lines, and writes HID Report 12 (4-byte output state)
// to the ESP32 JVS firmware (auto-discovered by VID:PID 0xD191:0x5487).
//
// Both Flycast and MAME use the same MAME network-output wire protocol, but:
//   - Flycast emits raw indexed "lampN" (N = JVS output bit) and "game = <rom>".
//   - MAME emits semantic names ("Start_lamp", "P1_Gun_Recoil", …) and
//     "mame_start = <rom>", carriage-return terminated.
// The core (passed via arm) selects the interpretation:
//   - Flycast → raw indexed lamps forwarded as JVS bits (HID Report 12).
//   - MAME    → semantic JvsOutputType (HID Report 13); the firmware resolves
//               the JVS bit per active cab (option A). START lamps + gun recoil,
//               plus a small per-game table for drivers that only emit indexed
//               names (e.g. Sega Model 1 out2/out3).
//
// Non-lamp Flycast wire formats (m3ffb / midiffb / awffb) are dropped — they
// don't travel over the JVS bus and would need a separate sidecar.

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define REPORT_ID            12
#define OUTPUT_BYTES         4
#define FW_VID               0xD191
#define FW_PID               0x5487
#define FLYCAST_HOST         "127.0.0.1"
#define FLYCAST_PORT         8000
#define SOCKET_PATH          "/var/run/recalboxjvs-outputsd.sock"
#define RECONNECT_DELAY_SEC  2

// ── Daemon state ────────────────────────────────────────────────────────────
static int  g_stop          = 0;
static int  g_armed         = 0;
static char g_expected_rom[64] = "";
static char g_current_game[64] = "";
static char g_core[32]         = "";  // emulator core, set by arm (flycast-next / mame*)
static unsigned char g_output_state[OUTPUT_BYTES] = {0};

// ── Semantic outputs (option A) ──────────────────────────────────────────────
// For MAME we forward the SEMANTIC output type, not a raw JVS bit — the firmware
// resolves the bit per active cab via its CabDescriptor. HID Report 13 carries a
// JvsOutputType bitmask. These values mirror the firmware enum (append-only):
// components/jvs-config/CabConfiguration.h.
#define REPORT_ID_SEM   13
enum {
    JVSOUT_LAMP_START_P1 = 1,
    JVSOUT_LAMP_START_P2 = 2,
    JVSOUT_LAMP_BILLBOARD = 3,
    JVSOUT_COIN1 = 4,
    JVSOUT_COIN2 = 5,
    JVSOUT_RECOIL_P1 = 6,
    JVSOUT_RECOIL_P2 = 7,
    // LampGeneric0..7 = 8..15
};
static uint32_t g_sem_state = 0;   // JvsOutputType bitmask (bit i = type i on)
static int      g_sem_dirty = 0;

static void on_signal(int sig) { (void)sig; g_stop = 1; }

// Locate the ESP32 JVS firmware's hidraw device by USB VID:PID.
static int find_hidraw(char* out, size_t out_sz)
{
    DIR* d = opendir("/sys/class/hidraw");
    if (!d) return -1;
    int found = -1;
    struct dirent* e;
    while ((e = readdir(d)))
    {
        if (strncmp(e->d_name, "hidraw", 6) != 0) continue;
        char ue_path[512];
        snprintf(ue_path, sizeof(ue_path),
                 "/sys/class/hidraw/%s/device/uevent", e->d_name);
        FILE* f = fopen(ue_path, "r");
        if (!f) continue;
        char line[256];
        while (fgets(line, sizeof(line), f))
        {
            unsigned int bus, vid, pid;
            if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &vid, &pid) == 3
                && vid == FW_VID && pid == FW_PID)
            {
                snprintf(out, out_sz, "/dev/%s", e->d_name);
                found = 0;
                break;
            }
        }
        fclose(f);
        if (found == 0) break;
    }
    closedir(d);
    return found;
}

static int   g_state_dirty = 0;  // set by apply_lamp, drained by flush_state
static int   g_hid_lost    = 0;  // set on a fatal hidraw write → main loop reopens

// Write one HID report; on a fatal error (device re-enumerated / unplugged) flag
// the fd as lost so the main loop closes and reopens it. Transient EAGAIN/EINTR
// on the non-blocking fd are ignored (keep the fd).
static void hid_write(int hid_fd, const unsigned char* buf, size_t len)
{
    if (hid_fd < 0) return;
    ssize_t n = write(hid_fd, buf, len);
    if (n == (ssize_t)len) return;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) return;
    syslog(LOG_WARNING, "hidraw write failed (%s) — reopening", strerror(errno));
    g_hid_lost = 1;
}

// Push the cached full state to the firmware as one HID OUTPUT report.
static void send_state(int hid_fd)
{
    if (hid_fd < 0) return;
    unsigned char buf[1 + OUTPUT_BYTES];
    buf[0] = REPORT_ID;
    memcpy(buf + 1, g_output_state, OUTPUT_BYTES);
    hid_write(hid_fd, buf, sizeof(buf));
}

// Send the cached state if anything changed since the last flush.
static void flush_state(int hid_fd)
{
    if (!g_state_dirty) return;
    send_state(hid_fd);
    g_state_dirty = 0;
}

// Push the cached semantic state (JvsOutputType bitmask) as HID Report 13.
static void send_sem(int hid_fd)
{
    if (hid_fd < 0) return;
    unsigned char buf[1 + 4];
    buf[0] = REPORT_ID_SEM;
    buf[1] = g_sem_state & 0xFF;
    buf[2] = (g_sem_state >> 8) & 0xFF;
    buf[3] = (g_sem_state >> 16) & 0xFF;
    buf[4] = (g_sem_state >> 24) & 0xFF;
    hid_write(hid_fd, buf, sizeof(buf));
}

static void flush_sem(int hid_fd)
{
    if (!g_sem_dirty) return;
    send_sem(hid_fd);
    g_sem_dirty = 0;
}

// Set/clear one JvsOutputType in the semantic cache (flushed once per recv burst).
static void apply_type(int type, int on)
{
    if (type <= 0 || type >= 32) return;
    uint32_t prev = g_sem_state;
    if (on) g_sem_state |= (1u << type);
    else    g_sem_state &= ~(1u << type);
    if (g_sem_state != prev) g_sem_dirty = 1;
}

// Update a single lamp bit in the cache. The caller flushes once per recv()
// so a Flycast burst of N lamp changes collapses into a single HID write.
static void apply_lamp(int lamp_idx, int value)
{
    if (lamp_idx < 0 || lamp_idx >= OUTPUT_BYTES * 8) return;
    int byte_idx = lamp_idx / 8;
    unsigned char mask = (unsigned char)(1u << (lamp_idx % 8));
    unsigned char prev = g_output_state[byte_idx];
    g_output_state[byte_idx] = value ? (prev | mask) : (prev & ~mask);
    if (g_output_state[byte_idx] != prev) g_state_dirty = 1;
}

static char* trim(char* s)
{
    while (*s == ' ' || *s == '\t') s++;
    char* end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
        *--end = '\0';
    return s;
}

static int core_is_mame(void) { return strncmp(g_core, "mame", 4) == 0; }

// Per-game override for MAME drivers that emit only indexed outputs (outN/ledN)
// whose meaning is driver-specific. Until recalbox-mame-db ships a full
// name→semantic table, a few known games are mapped here. idx = the N in the
// indexed name, type = JvsOutputType (semantic; the firmware resolves the bit).
struct index_map { const char* prefix; int idx; int type; };
struct game_map  { const char* rom; struct index_map maps[4]; };
static const struct game_map g_game_maps[] = {
    // Sega Model 1: gen_outputs_w emits out0..7; bit 2 = P1 start, bit 3 = P2 start.
    { "vf",      { {"out", 2, JVSOUT_LAMP_START_P1}, {"out", 3, JVSOUT_LAMP_START_P2}, {NULL, 0, 0} } },
    { "vr",      { {"out", 2, JVSOUT_LAMP_START_P1}, {NULL, 0, 0} } },
    { "wingwar", { {"out", 2, JVSOUT_LAMP_START_P1}, {NULL, 0, 0} } },
    { "swa",     { {"out", 2, JVSOUT_LAMP_START_P1}, {NULL, 0, 0} } },
    { "netmerc", { {"out", 2, JVSOUT_LAMP_START_P1}, {NULL, 0, 0} } },
    { NULL, { {NULL, 0, 0} } },
};

// Look up an indexed MAME output name against the current game's override table.
// Returns the JvsOutputType, or -1 if the game has no override for this name.
static int game_indexed_type(const char* rom, const char* name)
{
    for (const struct game_map* g = g_game_maps; g->rom; g++)
    {
        if (strcmp(g->rom, rom) != 0) continue;
        for (const struct index_map* m = g->maps; m->prefix; m++)
        {
            size_t plen = strlen(m->prefix);
            if (strncmp(name, m->prefix, plen) == 0 && atoi(name + plen) == m->idx)
                return m->type;
        }
        return -1;
    }
    return -1;
}

// Which player a MAME output name refers to (1..6, default 1). Names vary:
// Start_lamp, 1P_Start_lamp, P2_lamp_start, Player2_Gun_Recoil, left/right_...,
// MA/MB_..., startlamp2.
static int name_player(const char* lower)
{
    const char* pl = strstr(lower, "player");
    if (pl && isdigit((unsigned char)pl[6])) return pl[6] - '0';
    for (const char* p = lower; p[0]; p++)
    {
        if (p[0] == 'p' && isdigit((unsigned char)p[1])) return p[1] - '0';
        if (isdigit((unsigned char)p[0]) && p[1] == 'p') return p[0] - '0';
    }
    if (strstr(lower, "right"))     return 2;
    if (strstr(lower, "left"))      return 1;
    if (strncmp(lower, "mb", 2) == 0) return 2;
    if (strncmp(lower, "ma", 2) == 0) return 1;
    size_t n = strlen(lower);
    if (n && isdigit((unsigned char)lower[n - 1])) return lower[n - 1] - '0';
    return 1;
}

// Map a MAME semantic output name to a JvsOutputType, or -1 if unhandled.
// Option A: emit the semantic type; the firmware resolves the JVS bit per cab.
// Covers the two patternable, cab-relevant families: START lamps and gun recoil.
static int mame_output_type(const char* name)
{
    char lower[64];
    size_t i = 0;
    for (; name[i] && i < sizeof(lower) - 1; i++) lower[i] = tolower((unsigned char)name[i]);
    lower[i] = '\0';

    if (strstr(lower, "start") && (strstr(lower, "lamp") || strstr(lower, "led")))
    {
        int player = name_player(lower);
        if (player == 1) return JVSOUT_LAMP_START_P1;
        if (player == 2) return JVSOUT_LAMP_START_P2;
        return -1;   // P3+ not wired on 2-player cabs
    }

    const char* r = strstr(lower, "recoil");
    if (r)
    {
        // Bare 0-based form (recoil0 / gun_recoil1): 0→P1, 1→P2.
        if (isdigit((unsigned char)r[6]))
        {
            int idx = r[6] - '0';
            if (idx == 0) return JVSOUT_RECOIL_P1;
            if (idx == 1) return JVSOUT_RECOIL_P2;
            return -1;
        }
        // Named 1-based form (Player1_Gun_Recoil / P2_gun_recoil).
        int player = name_player(lower);
        if (player == 1) return JVSOUT_RECOIL_P1;
        if (player == 2) return JVSOUT_RECOIL_P2;
        return -1;
    }
    return -1;
}

static void process_line(char* line)
{
    char* eq = strchr(line, '=');
    if (!eq) return;
    *eq = '\0';
    char* key = trim(line);
    char* val = trim(eq + 1);

    // Game handshake — Flycast sends "game = X", MAME sends "mame_start = X".
    if (strcmp(key, "game") == 0 || strcmp(key, "mame_start") == 0)
    {
        if (strcmp(g_current_game, val) != 0)
        {
            syslog(LOG_INFO, "game = %s", val);
            strncpy(g_current_game, val, sizeof(g_current_game) - 1);
            g_current_game[sizeof(g_current_game) - 1] = '\0';
        }
        return;
    }

    if (!g_armed) return;
    if (g_expected_rom[0] && strcmp(g_current_game, g_expected_rom) != 0)
        return;

    if (core_is_mame())
    {
        // Per-game indexed override first (outN/ledN), then the semantic heuristic.
        // Emit the semantic type — the firmware maps it to a JVS bit per cab.
        int type = game_indexed_type(g_current_game, key);
        if (type <= 0) type = mame_output_type(key);
        if (type > 0) apply_type(type, atoi(val));
        return;
    }

    // Flycast: raw indexed lamps, lampN → bit N.
    if (strncmp(key, "lamp", 4) == 0)
    {
        apply_lamp(atoi(key + 4), atoi(val));
        return;
    }

    if (strcmp(key, "m3ffb") == 0 || strcmp(key, "midiffb") == 0
        || strcmp(key, "awffb") == 0)
    {
        // FFB channels need a separate path (Sega drive board / MIDI box).
        // Drop quietly here — log only the first occurrence per game.
        static char last_ffb[16] = "";
        if (strcmp(last_ffb, key) != 0)
        {
            syslog(LOG_INFO, "drop %s (no FFB sidecar wired)", key);
            strncpy(last_ffb, key, sizeof(last_ffb) - 1);
            last_ffb[sizeof(last_ffb) - 1] = '\0';
        }
    }
}

static void clear_all_outputs(int hid_fd)
{
    memset(g_output_state, 0, OUTPUT_BYTES);
    g_state_dirty = 0;
    g_sem_state = 0;
    g_sem_dirty = 0;
    send_state(hid_fd);
    send_sem(hid_fd);
}

static void handle_command(const char* cmd, int hid_fd)
{
    while (*cmd == ' ') cmd++;
    if (strncmp(cmd, "arm", 3) == 0 && (cmd[3] == '\0' || isspace((unsigned char)cmd[3])))
    {
        g_expected_rom[0] = '\0';
        g_core[0] = '\0';
        // "arm <rom> <core>" — sscanf stops each field at whitespace/newline.
        sscanf(cmd + 3, "%63s %31s", g_expected_rom, g_core);
        g_armed = 1;
        syslog(LOG_INFO, "armed rom=%s core=%s", g_expected_rom, g_core);
    }
    else if (strncmp(cmd, "disarm", 6) == 0)
    {
        clear_all_outputs(hid_fd);
        g_armed = 0;
        g_expected_rom[0] = '\0';
        g_current_game[0] = '\0';
        g_core[0] = '\0';
        syslog(LOG_INFO, "disarmed (outputs cleared)");
    }
}

static int connect_flycast(void)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(FLYCAST_PORT);
    inet_aton(FLYCAST_HOST, &addr.sin_addr);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(s);
        return -1;
    }
    return s;
}

static int bind_unix_socket(void)
{
    unlink(SOCKET_PATH);
    int s = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s < 0) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(s);
        return -1;
    }
    chmod(SOCKET_PATH, 0666);
    return s;
}

static int run_daemon(void)
{
    openlog("recalboxjvs-outputsd", LOG_PID, LOG_DAEMON);
    signal(SIGTERM, on_signal);
    signal(SIGINT,  on_signal);
    signal(SIGPIPE, SIG_IGN);

    int unix_fd = bind_unix_socket();
    if (unix_fd < 0)
    {
        syslog(LOG_ERR, "cannot bind unix socket %s: %s",
               SOCKET_PATH, strerror(errno));
        return 1;
    }
    syslog(LOG_INFO, "started — unix=%s, flycast=%s:%d",
           SOCKET_PATH, FLYCAST_HOST, FLYCAST_PORT);

    int flycast_fd  = -1;
    int hid_fd      = -1;
    time_t next_try = 0;
    char  buf[1024];
    char  line[1024];
    size_t line_len = 0;

    while (!g_stop)
    {
        if (flycast_fd < 0 && time(NULL) >= next_try)
        {
            flycast_fd = connect_flycast();
            if (flycast_fd >= 0)
            {
                syslog(LOG_INFO, "flycast connected");
                line_len = 0;
                fcntl(flycast_fd, F_SETFL, O_NONBLOCK);
            }
            else
            {
                next_try = time(NULL) + RECONNECT_DELAY_SEC;
            }
        }
        // A failed write flags the fd as lost (firmware re-enumerated) — drop it
        // so the block below reopens the fresh hidraw and re-syncs the state.
        if (g_hid_lost && hid_fd >= 0)
        {
            close(hid_fd);
            hid_fd = -1;
            g_hid_lost = 0;
        }
        if (hid_fd < 0)
        {
            char hidraw[320];
            if (find_hidraw(hidraw, sizeof(hidraw)) == 0)
            {
                hid_fd = open(hidraw, O_WRONLY | O_NONBLOCK);
                if (hid_fd >= 0)
                {
                    syslog(LOG_INFO, "opened %s", hidraw);
                    send_state(hid_fd);   // re-sync cached state to the fresh fd
                    send_sem(hid_fd);
                }
            }
        }

        struct pollfd pfds[2];
        int nfds = 0;
        pfds[nfds].fd = unix_fd;     pfds[nfds].events = POLLIN; nfds++;
        if (flycast_fd >= 0)
        {
            pfds[nfds].fd = flycast_fd; pfds[nfds].events = POLLIN; nfds++;
        }
        int timeout_ms = (flycast_fd < 0) ? (RECONNECT_DELAY_SEC * 1000) : 1000;
        int r = poll(pfds, nfds, timeout_ms);
        if (r < 0)
        {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "poll: %s", strerror(errno));
            break;
        }

        if (pfds[0].revents & POLLIN)
        {
            char cmd[256];
            ssize_t n = recv(unix_fd, cmd, sizeof(cmd) - 1, 0);
            if (n > 0) { cmd[n] = '\0'; handle_command(cmd, hid_fd); }
        }

        if (flycast_fd >= 0 && nfds > 1)
        {
            if (pfds[1].revents & POLLIN)
            {
                ssize_t n = read(flycast_fd, buf, sizeof(buf));
                if (n <= 0)
                {
                    if (n == 0 || errno != EAGAIN)
                    {
                        syslog(LOG_INFO, "flycast disconnected");
                        close(flycast_fd);
                        flycast_fd = -1;
                        next_try = time(NULL) + RECONNECT_DELAY_SEC;
                    }
                }
                else
                {
                    for (ssize_t i = 0; i < n; i++)
                    {
                        // Flycast terminates lines with \n, MAME with \r.
                        if (buf[i] == '\n' || buf[i] == '\r')
                        {
                            line[line_len] = '\0';
                            if (line_len) process_line(line);
                            line_len = 0;
                        }
                        else if (line_len + 1 < sizeof(line))
                        {
                            line[line_len++] = buf[i];
                        }
                    }
                    // One HID write per burst — collapses a game tick's lamp
                    // changes into a single USB packet. Flycast → raw (Rpt 12),
                    // MAME → semantic (Rpt 13); only the dirty one writes.
                    flush_state(hid_fd);
                    flush_sem(hid_fd);
                }
            }
            else if (pfds[1].revents & (POLLHUP | POLLERR))
            {
                syslog(LOG_INFO, "flycast socket hup/err");
                close(flycast_fd);
                flycast_fd = -1;
                next_try = time(NULL) + RECONNECT_DELAY_SEC;
            }
        }
    }

    syslog(LOG_INFO, "shutting down");
    clear_all_outputs(hid_fd);
    if (flycast_fd >= 0) close(flycast_fd);
    if (hid_fd >= 0)     close(hid_fd);
    close(unix_fd);
    unlink(SOCKET_PATH);
    closelog();
    return 0;
}

static int send_cli(const char* msg)
{
    int s = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s < 0) { perror("socket"); return 1; }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    if (sendto(s, msg, strlen(msg), 0,
               (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "sendto %s: %s\n", SOCKET_PATH, strerror(errno));
        close(s);
        return 1;
    }
    close(s);
    return 0;
}

static void usage(const char* argv0)
{
    fprintf(stderr,
            "usage: %s                    (daemon mode)\n"
            "       %s arm <rom> <core>   (ES hook: a game started)\n"
            "       %s disarm             (ES hook: game ended)\n",
            argv0, argv0, argv0);
}

int main(int argc, char** argv)
{
    if (argc < 2) return run_daemon();
    if (strcmp(argv[1], "arm") == 0)
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "arm %s %s\n",
                 argc >= 3 ? argv[2] : "", argc >= 4 ? argv[3] : "");
        return send_cli(msg);
    }
    if (strcmp(argv[1], "disarm") == 0)
        return send_cli("disarm\n");
    usage(argv[0]);
    return 1;
}
