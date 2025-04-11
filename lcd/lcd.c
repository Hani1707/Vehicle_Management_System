#include "lcd.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "delay.h"  // Assuming you have a delay.h file for delay functionality

// Function to send a command to the LCD
void send_command(unsigned char data)
{
    // Small delay to allow LCD to process the command
    DWT_Delay_ms(2);

    // Set RS = 0 and RW = 0 to indicate command mode
    GPIO_ResetBits(CTRL_PORT, (1 << RS_PIN));
    GPIO_ResetBits(CTRL_PORT, (1 << RW_PIN));

    // Clear current data on data pins
    GPIO_ResetBits(DATA_PORT, 0xFF << D0_PIN_Start);

    // Set command data on data pins
    GPIO_SetBits(DATA_PORT, data << D0_PIN_Start);

    // Pulse the Enable pin to latch the command
    GPIO_SetBits(CTRL_PORT, (1 << EN_PIN));
    GPIO_ResetBits(CTRL_PORT, (1 << EN_PIN));
}

// Function to send a character to the LCD
void lcd_putchar(unsigned char data)
{
    // Small delay to allow LCD to process the data
    DWT_Delay_ms(1);

    // Set RS = 1 and RW = 0 to indicate data mode
    GPIO_SetBits(CTRL_PORT, (1 << RS_PIN));
    GPIO_ResetBits(CTRL_PORT, (1 << RW_PIN));

    // Clear current data on data pins
    GPIO_ResetBits(DATA_PORT, 0xFF << D0_PIN_Start);

    // Set character data on data pins
    GPIO_SetBits(DATA_PORT, data << D0_PIN_Start);

    // Pulse the Enable pin to latch the character
    GPIO_SetBits(CTRL_PORT, (1 << EN_PIN));
    GPIO_ResetBits(CTRL_PORT, (1 << EN_PIN));
}

// Function to initialize the LCD
void lcd_init(void)
{
    DWT_Delay_ms(2);

    // Send initialization commands to the LCD
    send_command(0x3C);  // 8-bit mode, 2 lines
    send_command(0x06);  // Automatic cursor increment
    send_command(0x0C);  // Display on, cursor off
}

// Function to send a string to the LCD
void lcd_puts(char *str)
{
    DWT_Delay_ms(1);

    // Send each character of the string to the LCD
    while (*str != 0) {
        lcd_putchar(*str);
        str++;
    }
}

// Function to send a long string across multiple lines of the LCD
void lcd_puts_long(char *str)
{
    uint16_t cntr = 0;
    DWT_Delay_ms(1);
    lcd_gotoxy(0, 0);

    // Print characters on line 1
    while ((*str != 0) && (cntr < 16)) {
        cntr++;
        lcd_putchar(*str);
        str++;
    }

    // Print characters on line 2 if needed
    if (cntr >= 16) {
        lcd_gotoxy(0, 1);
        while ((*str != 0) && (cntr < 32)) {
            cntr++;
            lcd_putchar(*str);
            str++;
        }
    }

    // Print characters on line 3 if needed
    if (cntr >= 32) {
        lcd_gotoxy(0, 2);
        while ((*str != 0) && (cntr < 48)) {
            cntr++;
            lcd_putchar(*str);
            str++;
        }
    }

    // Print characters on line 4 if needed
    if (cntr >= 48) {
        lcd_gotoxy(0, 3);
        while ((*str != 0) && (cntr < 64)) {
            cntr++;
            lcd_putchar(*str);
            str++;
        }
    }
}

// Function to move the cursor to a specific position on the LCD
void lcd_gotoxy(unsigned char x, unsigned char y)
{
    DWT_Delay_ms(1);

    // Set the cursor position based on the row (y) and column (x)
    switch (y) {
        case 0:
            send_command(0x80 + x);
            break;
        case 1:
            send_command(0xC0 + x);
            break;
        case 2:
            send_command(0x90 + x);
            break;
        case 3:
            send_command(0xD0 + x);
            break;
    }
}

// Function to clear the LCD screen
void lcd_clear(void)
{
    DWT_Delay_ms(1);
    send_command(0x01);  // Clear display
    send_command(0x02);  // Return home
}

// Function to configure the GPIO for the LCD
void LCD_GPIO_Config(void) {
    // Enable clock for GPIOA (Control and Data Ports)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // Initialize GPIO structure
    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure control pins (RS, RW, EN) as output push-pull
    GPIO_InitStruct.GPIO_Pin = (1 << RS_PIN) | (1 << RW_PIN) | (1 << EN_PIN);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;  // Push-pull output
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // Speed of the output pins
    GPIO_Init(CTRL_PORT, &GPIO_InitStruct);

    // Configure data pins (D0-D7) as output push-pull
    GPIO_InitStruct.GPIO_Pin = 0xFF << D0_PIN_Start; // D0-D7 (assuming 8-bit mode)
    GPIO_Init(DATA_PORT, &GPIO_InitStruct);
}

