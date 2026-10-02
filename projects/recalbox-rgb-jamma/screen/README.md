# Recalbox RGB JAMMA oled I2C driver

## Compile a new font

To compile a 11pt bitmap font, use the 12 size + font height as: 
```bash
python3 font-to-header.py -s12 --font_height 11 -f . --ascii -o .
```

## Credits
- driver based on https://github.com/armlabs/ssd1306_linux
- font compilation based on https://github.com/jdmorise/TTF2BMH
