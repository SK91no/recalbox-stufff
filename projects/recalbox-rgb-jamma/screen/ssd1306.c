#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ssd1306.h>
#include <font.h>

static uint8_t fb[8][128];
static int kern_fd = -1;
static uint8_t max_lines = 0;
static uint8_t max_columns = 0;
static uint8_t global_x = 0;
static uint8_t global_y = 0;

uint8_t ssd1306_init(uint8_t i2c_dev)
{
    (void)i2c_dev;
    kern_fd = open("/dev/recalboxrgbjamma_screen", O_WRONLY);
    if (kern_fd < 0)
        return 1;
    memset(fb, 0, sizeof(fb));
    return 0;
}

void ssd1306_flush(void)
{
    if (kern_fd >= 0)
        write(kern_fd, fb, 1024);
}

uint8_t ssd1306_oled_default_config(uint8_t oled_lines, uint8_t oled_columns)
{
    if (oled_lines != SSD1306_128_64_LINES && oled_lines != SSD1306_128_32_LINES)
        oled_lines = SSD1306_128_64_LINES;
    if (oled_columns != SSD1306_128_64_COLUMNS && oled_columns != SSD1306_128_32_COLUMNS)
        oled_columns = SSD1306_128_64_COLUMNS;
    max_lines = oled_lines;
    max_columns = oled_columns;
    global_x = 0;
    global_y = 0;
    return 0;
}

uint8_t ssd1306_oled_set_XY(uint8_t x, uint8_t y)
{
    if (x >= max_columns || y >= (max_lines / 8)) return 1;
    global_x = x;
    global_y = y;
    return 0;
}

uint8_t ssd1306_oled_write_line(uint8_t size, char* ptr)
{
    uint8_t* font_table = 0;
    uint8_t font_table_width = 0;
    uint8_t font_table_lines = 1;

    if (ptr == 0) return 1;

    if (size == SSD1306_FONT_SMALL) {
        font_table = (uint8_t*)font5x7;
        font_table_width = 5;
    } else if (size == SSD1306_FONT_PIXTYPE) {
        font_table = (uint8_t*)Pixeltype16x16;
        font_table_width = 12;
        font_table_lines = 2;
    } else
        return 1;

    uint8_t saved_x = global_x;
    for (uint8_t current_line = 0; current_line < font_table_lines; current_line++) {
        uint8_t page = global_y + current_line;
        if (page >= 8) break;
        uint8_t col = saved_x;

        for (uint16_t index = 0; ptr[index] != 0; index++) {
            if (ptr[index] < ' ' || ptr[index] > '~') return 1;
            uint8_t* font_ptr = &font_table[(ptr[index] - 0x20) * font_table_width * font_table_lines];
            for (uint8_t j = current_line * font_table_width; j < (current_line + 1) * font_table_width; j++) {
                if (col >= max_columns) break;
                fb[page][col++] = font_ptr[j];
            }
            if (size == SSD1306_FONT_SMALL) {
                if (col < max_columns) fb[page][col++] = 0x00;
            }
        }
    }
    return 0;
}

uint8_t ssd1306_oled_clear_line(uint8_t row)
{
    if (row >= (max_lines / 8)) return 1;
    memset(fb[row], 0, 128);
    return 0;
}

uint8_t ssd1306_oled_clear_screen(void)
{
    memset(fb, 0, sizeof(fb));
    return 0;
}

uint8_t ssd1306_oled_draw_hline(uint x, uint y, uint len)
{
    uint8_t page = y / 8;
    uint8_t bit = 1 << (y % 8);
    if (page >= 8) return 1;
    for (uint col = x; col < x + len && col < 128; col++)
        fb[page][col] |= bit;
    return 0;
}

uint8_t ssd1306_oled_draw_vline(uint x, uint y, uint len)
{
    (void)len;
    while (y / 8 < 8) {
        if (x < 128) fb[y / 8][x] = 0xFF;
        y += 8;
    }
    return 0;
}
