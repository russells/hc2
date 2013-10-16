#ifndef lcd_h_INCLUDED
#define lcd_h_INCLUDED

#include <stdint.h>

void lcd_init(void);
void lcd_clear(void);
void lcd_setsegments(uint8_t c);
void lcd_showchar(char ch, uint8_t pos);
void lcd_showstring(const char *s);

#endif
