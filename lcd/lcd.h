#ifndef _LCD_H
#define _LCD_H

#include "stm32f10x.h"  // SPL for GPIO

// Define the data and control ports and pins for the LCD
#define DATA_PORT  GPIOA      // Data port for LCD (GPIOA)
#define CTRL_PORT  GPIOA      // Control port for LCD (GPIOA)
#define D0_PIN_Start  0       // Starting pin for D0
#define RS_PIN  12            // Pin for RS (Register Select)
#define RW_PIN  11            // Pin for RW (Read/Write)
#define EN_PIN  10            // Pin for EN (Enable)

// Function declarations for LCD operations
void lcd_init(void);                       // Initialize the LCD
void lcd_clear(void);                      // Clear the LCD display
void lcd_gotoxy(unsigned char x, unsigned char y); // Move cursor to (x, y)
void lcd_puts(char *str);                  // Print a string to the LCD
void lcd_puts_long(char *str);             // Print a long string (multi-line)
void lcd_putchar(unsigned char data);      // Print a single character to the LCD
void LCD_GPIO_Config(void);

#endif  // _LCD_H
