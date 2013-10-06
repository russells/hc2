#ifndef lcd_h_INCLUDED
#define lcd_h_INCLUDED

#include <stdint.h>

void lcd_init(void);
void lcd_clear(void);
void lcd_show(const char *s);
void lcd_showchar(char ch, uint8_t pos);
void lcd_showstring(const char *s, uint8_t startpos);

#endif
