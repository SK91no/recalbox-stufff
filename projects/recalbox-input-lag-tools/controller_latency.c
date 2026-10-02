/*
 * controller_latency - Measure input controller latency on Raspberry Pi 5
 *
 * Wiring: GPIO output pin → 2N2222 transistor → controller button input
 * The GPIO simulates a button press, then we measure how long it takes
 * for the corresponding evdev event to arrive.
 *
 * Usage: sudo ./controller_latency -g <gpiochip> -p <pin> -d <evdev_device> [-n <samples>] [-k <keycode>]
 *
 * Build: cmake -B build && cmake --build build
 * Requires: libgpiod >= 2.0
 *
 * (c) 2026 - Recalbox / DigitalLumberjack
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <getopt.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <dirent.h>
#include <gpiod.h>

/* ---------- defaults ---------- */
#define DEFAULT_GPIO_PIN    17
#define DEFAULT_SAMPLES     100
#define DEFAULT_TIMEOUT_MS  200               /* max wait for event */
#define SETTLE_US           5000              /* 5ms settle after GPIO toggle */
#define INTER_TEST_MS       50                /* pause between tests */

static volatile int g_running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* ---------- helpers ---------- */

static double timespec_diff_us(const struct timespec *a, const struct timespec *b)
{
    return (double)(b->tv_sec - a->tv_sec) * 1e6
         + (double)(b->tv_nsec - a->tv_nsec) / 1e3;
}

static int cmp_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    return (da > db) - (da < db);
}

/* Drain any pending events on the evdev fd */
static void drain_events(int fd)
{
    struct input_event ev;
    struct pollfd pfd = { .fd = fd, .events = POLLIN };

    while (poll(&pfd, 1, 0) > 0) {
        if (read(fd, &ev, sizeof(ev)) < 0)
            break;
    }
}

/* Wait for a specific key event (press or release).
 * Returns latency in µs from *t_start, or -1 on timeout. */
static double wait_for_event(int fd, int keycode, int value,
                             const struct timespec *t_start, int timeout_ms)
{
    struct input_event ev;
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    struct timespec t_event;

    while (1) {
        int ret = poll(&pfd, 1, timeout_ms);
        if (ret <= 0)
            return -1.0;  /* timeout or error */

        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n < (ssize_t)sizeof(ev))
            return -1.0;

        if (ev.type == EV_KEY && ev.code == (unsigned)keycode && ev.value == value) {
            /* Use the kernel event timestamp if available, else our own */
            if (ev.time.tv_sec != 0 || ev.time.tv_usec != 0) {
                t_event.tv_sec  = ev.time.tv_sec;
                t_event.tv_nsec = ev.time.tv_usec * 1000L;
            } else {
                clock_gettime(CLOCK_MONOTONIC, &t_event);
            }
            return timespec_diff_us(t_start, &t_event);
        }
        /* not our event, keep waiting */
    }
}

/* ---------- print helpers ---------- */

static void print_device_info(int fd)
{
    char name[256] = "Unknown";
    struct input_id id;

    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    printf("  Device : %s\n", name);

    if (ioctl(fd, EVIOCGID, &id) == 0) {
        printf("  ID     : bus=%04x vendor=%04x product=%04x version=%04x\n",
               id.bustype, id.vendor, id.product, id.version);
    }
}

static void print_stats(double *samples, int count)
{
    if (count == 0) {
        printf("\nNo valid samples collected.\n");
        return;
    }

    qsort(samples, count, sizeof(double), cmp_double);

    double sum = 0, sum_sq = 0;
    for (int i = 0; i < count; i++) {
        sum    += samples[i];
        sum_sq += samples[i] * samples[i];
    }

    double mean   = sum / count;
    double var    = (sum_sq / count) - (mean * mean);
    double stddev = sqrt(var > 0 ? var : 0);

    printf("\n╔══════════════════════════════════════╗\n");
    printf("║       LATENCY RESULTS (%3d samples)  ║\n", count);
    printf("╠══════════════════════════════════════╣\n");
    printf("║  Min      : %10.1f µs  (%5.2f ms) ║\n", samples[0],       samples[0] / 1000.0);
    printf("║  Max      : %10.1f µs  (%5.2f ms) ║\n", samples[count-1], samples[count-1] / 1000.0);
    printf("║  Mean     : %10.1f µs  (%5.2f ms) ║\n", mean,             mean / 1000.0);
    printf("║  Median   : %10.1f µs  (%5.2f ms) ║\n", samples[count/2], samples[count/2] / 1000.0);
    printf("║  p95      : %10.1f µs  (%5.2f ms) ║\n",
           samples[(int)(count * 0.95)], samples[(int)(count * 0.95)] / 1000.0);
    printf("║  p99      : %10.1f µs  (%5.2f ms) ║\n",
           samples[(int)(count * 0.99)], samples[(int)(count * 0.99)] / 1000.0);
    printf("║  Std Dev  : %10.1f µs  (%5.2f ms) ║\n", stddev,           stddev / 1000.0);
    printf("╚══════════════════════════════════════╝\n");
}

/* ---------- usage ---------- */

static void usage(const char *prog)
{
    printf("Usage: %s [options]\n\n"
           "Options:\n"
           "  -g, --gpiochip <path>    GPIO chip device (default: auto-detect)\n"
           "  -p, --pin <num>          GPIO pin number (default: %d)\n"
           "  -d, --device <path>      evdev device (e.g. /dev/input/event0)\n"
           "  -k, --keycode <code>     Expected EV_KEY code (default: auto-detect)\n"
           "  -n, --samples <num>      Number of samples (default: %d)\n"
           "  -t, --timeout <ms>       Event timeout in ms (default: %d)\n"
           "  -a, --active-low         GPIO is active-low (button shorts to GND)\n"
           "  -h, --help               Show this help\n\n"
           "Wiring (with 2N2222 NPN transistor):\n"
           "  GPIO pin ──[1kΩ]──┤B\n"
           "                    │     2N2222\n"
           "  Controller BTN ───┤C\n"
           "               GND ─┤E\n\n",
           prog, DEFAULT_GPIO_PIN,
           DEFAULT_SAMPLES, DEFAULT_TIMEOUT_MS);
}

/* ---------- GPIO chip auto-detection ---------- */

/* Known GPIO controller labels for Raspberry Pi boards */
static const char *known_labels[] = {
    "pinctrl-rp1",      /* RPi5 (RP1 chip, typically gpiochip4) */
    "pinctrl-bcm2711",  /* RPi4 */
    "pinctrl-bcm2835",  /* RPi3 / RPi2 / RPi1 / Zero */
    NULL
};

/*
 * Scan /dev/gpiochip* and return the path of the first chip whose
 * label matches a known Raspberry Pi GPIO controller.
 * Returns a malloc'd string, or NULL if nothing found.
 */
static char *detect_gpio_chip(void)
{
    DIR *dir = opendir("/dev");
    if (!dir)
        return NULL;

    struct dirent *ent;
    char path[128];

    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "gpiochip", 8) != 0)
            continue;

        snprintf(path, sizeof(path), "/dev/%s", ent->d_name);

        struct gpiod_chip *chip = gpiod_chip_open(path);
        if (!chip)
            continue;

        struct gpiod_chip_info *info = gpiod_chip_get_info(chip);
        if (!info) {
            gpiod_chip_close(chip);
            continue;
        }

        const char *label = gpiod_chip_info_get_label(info);

        for (const char **known = known_labels; *known; known++) {
            if (label && strcmp(label, *known) == 0) {
                printf("Auto-detected GPIO chip: %s (%s)\n", path, label);
                gpiod_chip_info_free(info);
                gpiod_chip_close(chip);
                closedir(dir);
                return strdup(path);
            }
        }

        gpiod_chip_info_free(info);
        gpiod_chip_close(chip);
    }

    closedir(dir);
    return NULL;
}

/* ---------- GPIO v2 helpers ---------- */

static struct gpiod_line_request *gpio_request_output(const char *chip_path,
                                                      unsigned int pin,
                                                      int active_low)
{
    struct gpiod_chip *chip = gpiod_chip_open(chip_path);
    if (!chip) {
        fprintf(stderr, "Cannot open GPIO chip '%s': %s\n", chip_path, strerror(errno));
        return NULL;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) {
        gpiod_chip_close(chip);
        return NULL;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    if (active_low)
        gpiod_line_settings_set_active_low(settings, true);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    if (!line_cfg) {
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return NULL;
    }
    gpiod_line_config_add_line_settings(line_cfg, &pin, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) {
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        gpiod_chip_close(chip);
        return NULL;
    }
    gpiod_request_config_set_consumer(req_cfg, "ctrl-latency");

    struct gpiod_line_request *request =
        gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);  /* chip can be closed, request stays valid */

    if (!request)
        fprintf(stderr, "Cannot request GPIO line %u: %s\n", pin, strerror(errno));

    return request;
}

/* ---------- main ---------- */

int main(int argc, char *argv[])
{
    const char  *gpio_chip  = NULL;  /* auto-detect */
    char        *detected_chip = NULL;
    unsigned int gpio_pin   = DEFAULT_GPIO_PIN;
    const char  *evdev_path = NULL;
    int          keycode    = -1;    /* auto-detect */
    int          num_samples = DEFAULT_SAMPLES;
    int          timeout_ms  = DEFAULT_TIMEOUT_MS;
    int          active_low  = 0;

    static struct option long_opts[] = {
        { "gpiochip",   required_argument, 0, 'g' },
        { "pin",        required_argument, 0, 'p' },
        { "device",     required_argument, 0, 'd' },
        { "keycode",    required_argument, 0, 'k' },
        { "samples",    required_argument, 0, 'n' },
        { "timeout",    required_argument, 0, 't' },
        { "active-low", no_argument,       0, 'a' },
        { "help",       no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "g:p:d:k:n:t:ah", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'g': gpio_chip   = optarg;                 break;
        case 'p': gpio_pin    = (unsigned)atoi(optarg);  break;
        case 'd': evdev_path  = optarg;                  break;
        case 'k': keycode     = atoi(optarg);            break;
        case 'n': num_samples = atoi(optarg);            break;
        case 't': timeout_ms  = atoi(optarg);            break;
        case 'a': active_low  = 1;                       break;
        case 'h':
        default:  usage(argv[0]); return (opt == 'h') ? 0 : 1;
        }
    }

    if (!evdev_path) {
        fprintf(stderr, "Error: -d <evdev_device> is required.\n\n");
        usage(argv[0]);
        return 1;
    }

    /* ---- auto-detect GPIO chip if not specified ---- */
    if (!gpio_chip) {
        detected_chip = detect_gpio_chip();
        if (!detected_chip) {
            fprintf(stderr, "Could not auto-detect GPIO chip.\n"
                            "Use -g to specify (e.g. -g /dev/gpiochip4)\n");
            return 1;
        }
        gpio_chip = detected_chip;
    }

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    /* ---- open GPIO (libgpiod v2) ---- */
    struct gpiod_line_request *gpio_req =
        gpio_request_output(gpio_chip, gpio_pin, active_low);
    if (!gpio_req) {
        free(detected_chip);
        return 1;
    }

    /* ---- open evdev ---- */
    int evfd = open(evdev_path, O_RDONLY | O_NONBLOCK);
    if (evfd < 0) {
        fprintf(stderr, "Cannot open '%s': %s\n", evdev_path, strerror(errno));
        gpiod_line_request_release(gpio_req);
        return 1;
    }

    /* Force evdev timestamps to CLOCK_MONOTONIC (default is CLOCK_REALTIME) */
    unsigned int clkid = CLOCK_MONOTONIC;
    if (ioctl(evfd, EVIOCSCLOCKID, &clkid) < 0) {
        fprintf(stderr, "Warning: cannot set CLOCK_MONOTONIC on evdev: %s\n"
                        "         Latency values may be incorrect.\n", strerror(errno));
    }

    /* ---- auto-detect keycode ---- */
    if (keycode < 0) {
        printf("Auto-detect: triggering GPIO %u to capture keycode...\n", gpio_pin);

        gpiod_line_request_set_value(gpio_req, gpio_pin, GPIOD_LINE_VALUE_ACTIVE);
        usleep(50000);

        struct input_event ev;
        struct pollfd pfd = { .fd = evfd, .events = POLLIN };

        int found = 0;
        for (int tries = 0; tries < 50 && !found; tries++) {
            if (poll(&pfd, 1, 100) > 0) {
                while (read(evfd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if (ev.type == EV_KEY && ev.value == 1) {
                        keycode = ev.code;
                        printf("Detected keycode: %d (0x%x)\n", keycode, keycode);
                        found = 1;
                        break;
                    }
                }
            }
        }

        gpiod_line_request_set_value(gpio_req, gpio_pin, GPIOD_LINE_VALUE_INACTIVE);
        usleep(50000);
        drain_events(evfd);

        if (!found) {
            fprintf(stderr, "Could not auto-detect keycode. Use -k to specify.\n");
            close(evfd);
            gpiod_line_request_release(gpio_req);
            return 1;
        }

        usleep(200000);
    }

    /* ---- info ---- */
    printf("\n── Controller Latency Tester ──\n");
    printf("  GPIO     : %s pin %u (%s)\n", gpio_chip, gpio_pin,
           active_low ? "active-low" : "active-high");
    printf("  Evdev    : %s\n", evdev_path);
    print_device_info(evfd);
    printf("  Keycode  : %d (0x%x)\n", keycode, keycode);
    printf("  Samples  : %d\n", num_samples);
    printf("  Timeout  : %d ms\n\n", timeout_ms);

    /* ---- measurement loop ---- */
    double *results = calloc(num_samples, sizeof(double));
    if (!results) { perror("calloc"); return 1; }

    int valid = 0, timeouts = 0;

    for (int i = 0; i < num_samples && g_running; i++) {
        drain_events(evfd);

        /* random jitter to avoid phase-locking with USB polling */
        usleep(INTER_TEST_MS * 1000 + (rand() % 5000));

        struct timespec t_press;

        /* ---- PRESS ---- */
        clock_gettime(CLOCK_MONOTONIC, &t_press);
        gpiod_line_request_set_value(gpio_req, gpio_pin, GPIOD_LINE_VALUE_ACTIVE);

        double latency = wait_for_event(evfd, keycode, 1, &t_press, timeout_ms);

        if (latency >= 0) {
            results[valid++] = latency;
            printf("\r  [%3d/%d] latency = %7.1f µs  (%.2f ms)",
                   i + 1, num_samples, latency, latency / 1000.0);
            fflush(stdout);
        } else {
            timeouts++;
            printf("\r  [%3d/%d] TIMEOUT                    ", i + 1, num_samples);
            fflush(stdout);
        }

        /* ---- RELEASE ---- */
        usleep(SETTLE_US);
        gpiod_line_request_set_value(gpio_req, gpio_pin, GPIOD_LINE_VALUE_INACTIVE);

        /* wait for release event (ignore timing) */
        wait_for_event(evfd, keycode, 0, &t_press, timeout_ms);
        usleep(SETTLE_US);
    }

    printf("\n");

    if (timeouts > 0)
        printf("  ⚠ %d timeout(s) out of %d attempts\n", timeouts, valid + timeouts);

    /* ---- stats ---- */
    print_stats(results, valid);

    /* ---- cleanup ---- */
    free(results);
    free(detected_chip);
    gpiod_line_request_set_value(gpio_req, gpio_pin, GPIOD_LINE_VALUE_INACTIVE);
    close(evfd);
    gpiod_line_request_release(gpio_req);

    return 0;
}