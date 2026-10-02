#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ssd1306.h>
#include <mqtt.h>
#include <kms-mode.h>
#include <sys/stat.h>
#include "log.h"

int readTemp() {
    int temp = 0;
    size_t len = 0;
    ssize_t read;
    char *line = NULL;
    FILE *fptr = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fptr != NULL) {
        if ((read = getline(&line, &len, fptr)) != -1) {
            sscanf(line, "%d", &temp);
        }
        fclose(fptr);
    }
    return temp;
}


void draw_game_name(const char *game_name) {
    char buf[21];
    memset(buf, 0, 21);
    strncpy(buf, game_name, strlen(game_name) < 16 ? strlen(game_name) : 16);
    ssd1306_oled_clear_line(0);
    ssd1306_oled_set_XY(0, 0);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, buf);
}

void draw_temp(int temp) {
    char buf[10];
    sprintf(buf, "%d", (int) (temp / 1000));
    ssd1306_oled_set_XY(98, 0);
    ssd1306_oled_write_line(SSD1306_FONT_PIXTYPE, buf);
    ssd1306_oled_set_XY(122, 0);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, "o");
}

void draw_system_name(const char *system_name) {
    char buf[64];
    memset(buf, 0, 64);
    strncpy(buf, system_name, strlen(system_name) < 16 ? strlen(system_name) : 16);
    ssd1306_oled_clear_line(1);
    ssd1306_oled_set_XY(0, 1);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, buf);
}

void draw_lines() {
    ssd1306_oled_draw_hline(0, 16, 128);
    ssd1306_oled_draw_vline(64, 16, 52);
}

static int lastTime = 0;
static video_mode lastMode;
void update_screen() {
    struct stat st;
    video_mode mode = get_current_mode();
    if (mode.height != lastMode.height || mode.width != lastMode.width || mode.vfreq != lastMode.vfreq){
        // Mode changed
        printf("Mode changed\n");
        if (lastMode.height < 576 && mode.height >= 576) {
            // When priority to hdmi is true, when booting, the vga is set to off and hdmi to on for 2 secondes
            // It will display the hdmi resolution for 2 seconds, that is not the current resolution sent through dpi.
            // If the mode has changed to SD to HD, we wait 3 seconds and let next loop update the screen
            lastMode = mode;
            sleep(3);
            return;
        }
        lastMode = mode;
    } else {
        printf("Mode not changed checking es_state.inf\n");
        stat("/tmp/es_state.inf", &st);
        if(st.st_mtime == lastTime) {
            printf("File not changed\n");
            draw_temp(readTemp());
            ssd1306_flush();
            return;
        }
        lastTime = st.st_mtime;
    }

    printf("Refreshing all screen\n");

    FILE *fptr = NULL;
    char *line = NULL;
    char buf[1024];
    memset(buf, 0, 1024);
    size_t len = 0;
    ssize_t read;
    fptr = fopen("/tmp/es_state.inf", "r");
    ssd1306_oled_clear_screen();
    draw_lines();
    draw_game_name("Frontend");
    draw_system_name("");
    if (fptr != NULL) {
        while ((read = getline(&line, &len, fptr)) != -1) {
            int sred = sscanf(line, "Game=%[^\n\r]", buf);
            if (sred && strlen(buf) > 0) {
                //printf("Got game name : %s\n", buf);
                draw_game_name(buf);
                continue;
            };
            sred = sscanf(line, "System=%[^\n\r]", buf);
            if (sred) {
                //printf("Got system name : %s\n", buf);
                draw_system_name(buf);
                continue;
            };
        }
        fclose(fptr);
    }

    // Mode - HSync
    ssd1306_oled_set_XY(30, 3);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, "H");
    ssd1306_oled_set_XY(5, 4);
    sprintf(buf, "%.3fkHz", (float) mode.hfreq / 1000);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, buf);
    // Mode - VSync
    ssd1306_oled_set_XY(94, 3);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, "V");
    ssd1306_oled_set_XY(75, 4);
    sprintf(buf, "%.3fHz", mode.vfreq);
    ssd1306_oled_write_line(SSD1306_FONT_SMALL, buf);

    // Resolution & Factor
    if (mode.height <= 576)
    {
        ssd1306_oled_set_XY(20, 6);
        sprintf(buf, "%dx%d%c", mode.width, mode.height, mode.interlaced ? 'i' : 'p');
        ssd1306_oled_write_line(SSD1306_FONT_PIXTYPE, buf);
        int factor = mode.width / (mode.height * 4 / 3);
        ssd1306_oled_set_XY(0, 7);
        sprintf(buf, "x%d", factor);
        ssd1306_oled_write_line(SSD1306_FONT_SMALL, buf);
    } else {
        ssd1306_oled_set_XY(5, 6);
        sprintf(buf, "%dx%d", mode.width, mode.height);
        ssd1306_oled_write_line(SSD1306_FONT_PIXTYPE, buf);
    }

    // Temp
    draw_temp(readTemp());
    ssd1306_flush();
}

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    //printf("%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);
    if (strncmp("rungame", msg->payload, sizeof("rungame")) == 0
        || strncmp("rundemo", msg->payload, sizeof("rundemo")) == 0
        || strncmp("endgame", msg->payload, sizeof("endgame")) == 0
        || strncmp("enddemo", msg->payload, sizeof("enddemo")) == 0
        || strncmp("runkodi", msg->payload, sizeof("runkodi")) == 0
        || strncmp("start", msg->payload, sizeof("start")) == 0
        || strncmp("stop", msg->payload, sizeof("stop")) == 0) {
        update_screen();
    }
}

int main(int argc, char **argv) {
    _openlog("jammascreen", 0, LOG_DAEMON, LOG_INFO);
    _syslog(LOG_INFO, "Starting Recalbox RGB JAMMA screen");
    uint8_t rc = 0;
    rc = ssd1306_init(0);
    ssd1306_oled_default_config(64, 128);
    if (rc != 0) {
        _syslog(LOG_INFO, "no oled screen found at /dev/recalboxrgbjamma_screen");
        return 1;
    }

    bool clear = false;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--clear") == 0) clear = true;

    if (clear) {
        _syslog(LOG_INFO, "Clear screen");
        ssd1306_oled_clear_screen();
        ssd1306_flush();
        return 0;
    }

    while (true) {
        update_screen();
        sleep(2);
    }
    return 0;
}
