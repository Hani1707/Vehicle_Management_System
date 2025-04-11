#ifndef I2C_LCD_H
#define I2C_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

// Function Prototypes

/**
 * @brief Initializes the LCD in 4-bit mode using I2C.
 *        This must be called before using other LCD functions.
 */
void LCD_Init(void);

/**
 * @brief Sends a command to the LCD.
 * @param Command: Command byte to send (e.g., clear screen, set cursor).
 */
void LCD_SendCommand(uint8_t Command);

/**
 * @brief Sends data (characters) to the LCD.
 * @param Data: Data byte to send (ASCII character to display).
 */
void LCD_SendData(uint8_t Data);

/**
 * @brief Sends a string to the LCD starting from the current cursor position.
 * @param Str: Pointer to the null-terminated string to send.
 */
void LCD_SendString(const char *Str);

/**
 * @brief Clears the LCD display.
 *        This sends the clear screen command (0x01) to the LCD.
 */
void LCD_ClearDisplay(void);

/**
 * @brief Moves the cursor to a specific position on the LCD.
 * @param Row: Row number (0 or 1).
 * @param Col: Column number (0 to 15).
 */
void LCD_SetCursor(uint8_t Row, uint8_t Col);

#ifdef __cplusplus
}
#endif

#endif /* I2C_LCD_H */
