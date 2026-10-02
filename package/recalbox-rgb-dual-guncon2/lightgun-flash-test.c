/* Minimal SDL2 / KMSDRM frame-perfect test for GunCon 2 lightgun flash
 * latency. NOT BUILT BY DEFAULT — kept here as a debugging helper.
 *
 * What it does:
 *   - Watches the GunCon 2 evdev device.
 *   - On BTN_LEFT rising edge, paints the entire screen white for
 *     FLASH_FRAMES.
 *   - Captures the first valid ABS_X/ABS_Y after each shot and draws
 *     a crosshair at that point for HIT_FRAMES.
 *   - "track" mode (argv[2] = "track"): permanent white background +
 *     live crosshair following the gun.
 *   - BTN_RIGHT toggles between flash mode and tracker mode.
 *
 * DISPLAY MODE NOTE — 31 kHz path only:
 *
 * The GunCon 2 firmware works natively on 15 kHz CRT TVs in its
 * default "TV/15kHz" mode (mode_flags=0x0100): plug it in, the
 * standard 15 kHz timings give a clean X lock. Nothing special
 * needed.
 *
 * On a 31 kHz / multi-sync screen, only ONE specific modeline keeps
 * the photodiode in sync with the beam — anything off (e.g. the
 * VESA-style 25.452 MHz / htotal=808 default) wraps mid-screen and
 * X never locks reliably:
 *
 *     640x480 @ 60 Hz, pclk = 26.1765 MHz
 *     H: 640 682 805 831   (HFP=42 HSYNC=123 HBP=26)   htotal = 831
 *     V: 480 495 502 525   (VFP=15 VSYNC=7  VBP=23)    vtotal = 525
 *     -hsync -vsync                                     HSYNC = 31.500 kHz
 *
 * This is what switchres computes from the recalbox GunCon 2
 * crt_range. SDL2/KMSDRM only selects from the connector's existing
 * modes, so for this binary to actually run on the 31 kHz modeline
 * it must already be the active KMS mode. Two ways:
 *
 *   1. Run inside RetroArch: RetroArch + switchres apply the modeline
 *      via DRM master (see libretroLightGuns.py). Standalone SDL2
 *      can't inject it (limited DRM rights).
 *
 *   2. Patch the modeline as the default for `p640x480` in
 *      projects/recalbox-rgb-dual/recalboxrgbdual.c so the kernel
 *      hands it out at boot.
 *
 * Build (host cross-compile via the buildroot toolchain):
 *   TC=$ROOT/output/per-package/host-gcc-final/host/bin
 *   SYSROOT=$ROOT/output/per-package/retroarch/host/aarch64-buildroot-linux-gnu/sysroot
 *   $TC/aarch64-buildroot-linux-gnu-gcc -O2 \
 *       --sysroot=$SYSROOT -I$SYSROOT/usr/include/SDL2 \
 *       -o lightgun-flash-test lightgun-flash-test.c \
 *       -lSDL2 -lpthread -lm
 *
 * Usage on target:
 *   lightgun-flash-test [/dev/input/eventX] [flash_frames|track]
 *   echo N > /recalbox/share/system/lightgun_deliver_frames  # tunable
 */

#include <SDL2/SDL.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <time.h>

static volatile sig_atomic_t g_running = 1;
static void sig_quit(int sig) { (void)sig; g_running = 0; }

/* Read the current value of ABS_X / ABS_Y from the kernel input layer
 * even if no event has been emitted yet (evdev coalesces equal values). */
static int read_abs_now(int fd, int code, int *out)
{
    struct input_absinfo a;
    if (ioctl(fd, EVIOCGABS(code), &a) < 0)
        return -1;
    *out = a.value;
    return 0;
}

#define WIDTH 640
#define HEIGHT 480
#define DEFAULT_FLASH_FRAMES 6
#define HIT_FRAMES   60      /* draw crosshair for 1 second after impact */
#define CROSS_HALF   12

static double now_sec(void)
{
    /* Use CLOCK_REALTIME so the timestamps line up with the
     * ev.time fields evdev reports (also realtime-based). */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void draw_cross(SDL_Renderer *ren, int x, int y)
{
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
    SDL_RenderDrawLine(ren, x - CROSS_HALF, y, x + CROSS_HALF, y);
    SDL_RenderDrawLine(ren, x, y - CROSS_HALF, x, y + CROSS_HALF);
    /* a small ring at the centre */
    int r = 4;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx * dx + dy * dy <= r * r)
                SDL_RenderDrawPoint(ren, x + dx, y + dy);
}

int main(int argc, char **argv)
{
    /* Usage:
     *   lightgun-flash-test [evdev_path] [flash_frames]
     *   lightgun-flash-test [evdev_path] track
     *
     * In "track" mode the background is permanently white (so the
     * gun firmware always has a target to lock on) and the crosshair
     * follows the live ABS_X/ABS_Y. Useful to observe whether the
     * firmware locks X at all on a given gun/modeline. */
    const char *evdev    = (argc > 1) ? argv[1] : "/dev/input/event1";
    int track_mode       = (argc > 2 && !strcmp(argv[2], "track"));
    int flash_frames     = (argc > 2 && !track_mode) ? atoi(argv[2])
                                                     : DEFAULT_FLASH_FRAMES;
    if (flash_frames < 1) flash_frames = 1;
    if (flash_frames > 30) flash_frames = 30;
    int fd = open(evdev, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", evdev, strerror(errno));
        return 1;
    }

    setenv("SDL_VIDEODRIVER", "kmsdrm", 0);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("flash",
                                       SDL_WINDOWPOS_UNDEFINED,
                                       SDL_WINDOWPOS_UNDEFINED,
                                       WIDTH, HEIGHT,
                                       SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    /* Software renderer: no OpenGL needed (and the box's libGL is
     * missing eglGetPlatformDisplayEXT). PRESENTVSYNC keeps frame
     * timing aligned with the display. */
    SDL_Renderer *ren = SDL_CreateRenderer(
        win, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    /* Quit cleanly on SIGINT (Ctrl+C) and SIGTERM (kill). */
    signal(SIGINT,  sig_quit);
    signal(SIGTERM, sig_quit);

    fprintf(stderr, "[flash-test] ready, screen %dx%d, flash=%d hit=%d "
            "(SIGINT/SIGTERM to quit)\n",
            WIDTH, HEIGHT, flash_frames, HIT_FRAMES);

    int flash_left   = 300;  /* 5 s of white at startup so the gun
                              * auto-cal in the driver has a chance
                              * to converge before the first shot. */
    int hit_left     = 0;
    int hit_x        = -1;
    int hit_y        = -1;
    int last_btn     = 0;
    double t_press   = 0.0;
    int armed_for_pos = 0;       /* waiting for the first valid X+Y */
    int got_x = 0, got_y = 0;
    int pending_x = 0, pending_y = 0;
    int frame_count  = 0;
    int press_frame   = 0;       /* frame_count at the moment of press */
    int pre_x = 0, pre_y = 0;    /* position cached BEFORE the press */

    while (g_running) {
        struct input_event ev;
        ssize_t n;
        while ((n = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
            double ts = ev.time.tv_sec + ev.time.tv_usec / 1e6;
            if (ev.type == EV_KEY && ev.code == BTN_RIGHT && ev.value) {
                /* toggle between flash-on-press and live tracker */
                track_mode = !track_mode;
                fprintf(stderr,
                        "[flash-test] toggle track_mode=%d\n",
                        track_mode);
            }
            if (ev.type == EV_KEY && ev.code == BTN_LEFT) {
                if (ev.value && !last_btn) {
                    /* snapshot the position cached by the driver
                     * BEFORE we trigger the flash, so we can detect
                     * when the gun produces a *new* lock. */
                    pre_x = pre_y = 0;
                    read_abs_now(fd, ABS_X, &pre_x);
                    read_abs_now(fd, ABS_Y, &pre_y);
                    flash_left    = flash_frames;
                    t_press       = ts;
                    press_frame   = frame_count;
                    armed_for_pos = 30; /* watch the next ~500 ms */
                    /* Drop the previous crosshair so this PRESS can
                     * record its own HIT (the HIT check requires
                     * hit_x < 0). */
                    hit_x = hit_y = -1;
                    hit_left = 0;
                    fprintf(stderr,
                            "[flash-test] PRESS  t=%.6f frame=%d "
                            "pre_x=%d pre_y=%d\n",
                            t_press, frame_count, pre_x, pre_y);
                }
                last_btn = ev.value;
            }
        }

        /* While we're armed for a position read, query the current
         * X / Y from the kernel via EVIOCGABS. evdev does not emit
         * a new ABS event when the value is identical to the previous
         * one (e.g. user is locked on the same target across shots)
         * so reading directly is the only way to get the position
         * for every shot. */
        if (armed_for_pos > 0) {
            int x_now = -1, y_now = -1;
            read_abs_now(fd, ABS_X, &x_now);
            read_abs_now(fd, ABS_Y, &y_now);
            /* A "new lock" = the driver reports something different
             * from the snapshot we took at PRESS time AND the value
             * looks valid (> sentinel). */
            int new_lock = (x_now > 1 && y_now > 4
                            && (x_now != pre_x || y_now != pre_y));
            if (hit_x < 0 && new_lock) {
                int frames_to_lock = frame_count - press_frame;
                hit_x    = x_now;
                hit_y    = y_now;
                hit_left = HIT_FRAMES;
                fprintf(stderr,
                        "[flash-test] HIT    t=%.6f X=%d Y=%d "
                        "delta_press=%.3f ms frames_to_lock=%d "
                        "flash_frames=%d\n",
                        now_sec(), hit_x, hit_y,
                        (now_sec() - t_press) * 1000.0,
                        frames_to_lock, flash_frames);
            }
            armed_for_pos--;
            if (armed_for_pos == 0 && hit_x < 0) {
                fprintf(stderr,
                        "[flash-test] MISS   t=%.6f X=%d Y=%d "
                        "(stuck on pre_x=%d pre_y=%d, "
                        "flash_frames=%d)\n",
                        now_sec(), x_now, y_now, pre_x, pre_y,
                        flash_frames);
            }
        }
        if (hit_x < 0) {
            /* prepare for next press */
            hit_x = hit_y = -1;
        }

        /* Render the frame. */
        if (track_mode || flash_left > 0) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            if (flash_left > 0) flash_left--;
        } else {
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        }
        SDL_RenderClear(ren);

        if (track_mode) {
            int tx = -1, ty = -1;
            read_abs_now(fd, ABS_X, &tx);
            read_abs_now(fd, ABS_Y, &ty);
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
            /* live crosshair following the gun */
            SDL_RenderDrawLine(ren, tx - 18, ty, tx + 18, ty);
            SDL_RenderDrawLine(ren, tx, ty - 18, tx, ty + 18);
        }

        /* Aim target at screen centre — only on a black frame, so we
         * don't paint over the white flash. */
        if (!track_mode && flash_left == 0) {
            int cx = WIDTH / 2, cy = HEIGHT / 2;
            SDL_SetRenderDrawColor(ren, 0, 200, 0, 255);
            for (int r = 18; r <= 22; r++) {
                for (int a = 0; a < 720; a++) {
                    double th = a * M_PI / 360.0;
                    SDL_RenderDrawPoint(ren,
                        cx + (int)(r * cos(th)),
                        cy + (int)(r * sin(th)));
                }
            }
            SDL_RenderDrawLine(ren, cx - 30, cy, cx - 8, cy);
            SDL_RenderDrawLine(ren, cx + 8, cy, cx + 30, cy);
            SDL_RenderDrawLine(ren, cx, cy - 30, cx, cy - 8);
            SDL_RenderDrawLine(ren, cx, cy + 8, cx, cy + 30);
        }

        if (hit_left > 0 && hit_x >= 0 && hit_y >= 0) {
            draw_cross(ren, hit_x, hit_y);
            hit_left--;
            if (hit_left == 0) {
                /* clear so the next press re-arms cleanly */
                hit_x = hit_y = -1;
            }
        }

        SDL_RenderPresent(ren);
        frame_count++;
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    close(fd);
    return 0;
}
