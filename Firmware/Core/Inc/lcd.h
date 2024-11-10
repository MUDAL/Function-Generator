/*
 * lcd.h
 *
 *  Created on: Nov 7, 2024
 *  	Author: Mahamudul Hasan
 *      Author: Olaoluwa Raji
 *
 *  Obtained from:
 *  https://embeddedthere.com/interfacing-stm32-with-i2c-lcd-with-hal-code-example/
 *
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "i2c.h"

extern void LCD_Init(void);

extern void LCD_WriteString(char *str);

extern void LCD_SetCursor(uint8_t row, uint8_t column);

extern void LCD_Clear(void);

extern void LCD_Backlight(uint8_t state);

#endif /* INC_LCD_H_ */
