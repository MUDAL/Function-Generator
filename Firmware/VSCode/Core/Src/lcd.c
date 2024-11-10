/*
 * lcd.c
 *
 *  Created on: Nov 7, 2024
 *  	Author: Mahamudul Hasan
 *      Author: Olaoluwa Raji
 *
 *  Obtained from:
 *  https://embeddedthere.com/interfacing-stm32-with-i2c-lcd-with-hal-code-example/
 *
 */

#include "lcd.h"

#define I2C_ADDR    0x27 // I2C address of the PCF8574
#define RS_BIT      0 // Register select bit
#define EN_BIT      2 // Enable bit
#define BL_BIT      3 // Backlight bit
#define D4_BIT      4 // Data 4 bit
#define D5_BIT      5 // Data 5 bit
#define D6_BIT      6 // Data 6 bit
#define D7_BIT      7 // Data 7 bit

#define LCD_ROWS    2 // Number of rows on the LCD
#define LCD_COLS    16 // Number of columns on the LCD

#define FUNC_SET_8BIT_NIBBLE            0x03
#define FUNC_SET_4BIT_NIBBLE            0x02
#define RETURN_HOME                     0x02
#define FUNC_SET_2LINE_5x8_DOT          0x28
#define CLEAR_DISPLAY                   0x01
#define DISPLAY_ON_CURSOR_OFF           0x0C
#define ENTRY_MODE_INCREMENT_CURSOR     0x06
#define SET_DDRAM_ADDR                  0x80

const uint8_t address[LCD_ROWS][LCD_COLS] =
{// Line 1
 {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F},
 // Line 2
 {0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
  0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F}
};

static uint8_t backlight_state = 0;

static void LCD_WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = nibble << D4_BIT;
    data |= rs << RS_BIT;
    data |= backlight_state << BL_BIT; // Include backlight state in data
    data |= 1 << EN_BIT;
    HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, 50);
    HAL_Delay(1);
    data &= ~(1 << EN_BIT);
    HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, 50);
}

static void LCD_SendCmd(uint8_t cmd)
{
    uint8_t upper_nibble = cmd >> 4;
    uint8_t lower_nibble = cmd & 0x0F;
    LCD_WriteNibble(upper_nibble, 0);
    LCD_WriteNibble(lower_nibble, 0);
    if(cmd == CLEAR_DISPLAY || cmd == RETURN_HOME)
    {
        HAL_Delay(2);
    }
}

static void LCD_SendData(uint8_t data)
{
    uint8_t upper_nibble = data >> 4;
    uint8_t lower_nibble = data & 0x0F;
    LCD_WriteNibble(upper_nibble, 1);
    LCD_WriteNibble(lower_nibble, 1);
}

void LCD_Init(void)
{
    HAL_Delay(50);
    LCD_WriteNibble(FUNC_SET_8BIT_NIBBLE, 0);
    HAL_Delay(5);
    LCD_WriteNibble(FUNC_SET_8BIT_NIBBLE, 0);
    HAL_Delay(1);
    LCD_WriteNibble(FUNC_SET_8BIT_NIBBLE, 0);
    HAL_Delay(1);
    LCD_WriteNibble(FUNC_SET_4BIT_NIBBLE, 0);
    LCD_SendCmd(FUNC_SET_2LINE_5x8_DOT);
    LCD_SendCmd(DISPLAY_ON_CURSOR_OFF);
    LCD_SendCmd(ENTRY_MODE_INCREMENT_CURSOR);
    LCD_SendCmd(CLEAR_DISPLAY);
    HAL_Delay(2);
    LCD_Backlight(1); // Activate the backlight
}

void LCD_WriteString(char *str)
{
    while(*str != '\0')
    {
        LCD_SendData(*str);
        str++;
    }
}

void LCD_SetCursor(uint8_t row, uint8_t column)
{
    if(row >= LCD_ROWS || column >= LCD_COLS)
    {
        return; // Out of range
    }
    LCD_SendCmd(SET_DDRAM_ADDR | address[row][column]);
}

void LCD_Clear(void)
{
    LCD_SendCmd(CLEAR_DISPLAY);
    HAL_Delay(2);
}

void LCD_Backlight(uint8_t state)
{
    backlight_state = state;
}
