#ifndef lcd_h_INCLUDED
#define lcd_h_INCLUDED

#include <stdint.h>

void lcd_init(void);
void lcd_clear(void);
void lcd_setsegments(uint8_t c);
void lcd_showchar(char ch, uint8_t pos);
void lcd_showstring(const char *s);
void lcd_showdigits(const char *ds);
void lcd_colon(uint8_t onoff);
void lcd_timeouts(uint8_t timeouts);
void lcd_battery(uint8_t b);
void lcd_buttons(uint8_t buttons);

#define LCD_BUTTON_DOWN 0x80
#define LCD_BUTTON_ENTER 0x40
#define LCD_BUTTON_UP 0x20
#define LCD_BUTTON_CANCEL 0x10

#define LCD_BUTTONS_ENTER_UP_DOWN_CANCEL			\
	(LCD_BUTTON_ENTER					\
	 |LCD_BUTTON_UP						\
	 |LCD_BUTTON_DOWN					\
	 |LCD_BUTTON_CANCEL)
#define LCD_BUTTONS_ALL LCD_BUTTONS_ENTER_UP_DOWN_CANCEL
#define LCD_BUTTONS_ENTER_UP_DOWN				\
	(LCD_BUTTON_ENTER					\
	 |LCD_BUTTON_UP						\
	 |LCD_BUTTON_DOWN)
#define LCD_BUTTONS_UP_DOWN_CANCEL				\
	(LCD_BUTTON_UP						\
	 |LCD_BUTTON_DOWN					\
	 |LCD_BUTTON_CANCEL)
#define LCD_BUTTONS_ENTER_DOWN_CANCEL				\
	(LCD_BUTTON_ENTER					\
	 |LCD_BUTTON_DOWN					\
	 |LCD_BUTTON_CANCEL)
#define LCD_BUTTONS_ENTER_UP_CANCEL				\
	(LCD_BUTTON_ENTER					\
	 |LCD_BUTTON_UP						\
	 |LCD_BUTTON_CANCEL)

#endif
