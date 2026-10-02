#ifndef __SSD1306_H__
#define __SSD1306_H__

#define SSD1306_FONT_SMALL          0x00
#define SSD1306_FONT_PIXTYPE        0x02

#define SSD1306_128_64_LINES        64
#define SSD1306_128_32_LINES        32
#define SSD1306_128_64_COLUMNS      128
#define SSD1306_128_32_COLUMNS      128

uint8_t ssd1306_init(uint8_t i2c_dev);
void    ssd1306_flush(void);
uint8_t ssd1306_oled_default_config(uint8_t oled_lines, uint8_t oled_columns);
uint8_t ssd1306_oled_set_XY(uint8_t x, uint8_t y);
uint8_t ssd1306_oled_write_line(uint8_t size, char* ptr);
uint8_t ssd1306_oled_clear_line(uint8_t row);
uint8_t ssd1306_oled_clear_screen(void);
uint8_t ssd1306_oled_draw_hline(uint x, uint y, uint len);
uint8_t ssd1306_oled_draw_vline(uint x, uint y, uint len);

#endif
