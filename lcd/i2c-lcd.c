#include "i2c-lcd.h"
#include "stm32f10x_i2c.h"
#include "Delay.h"

// I2C LCD slave address
#define LCD_I2C_SLAVE_ADDRESS 0x27

/**
 * @brief Writes data to the I2C device.
 * 
 * @param I2CAddress The I2C address of the slave device.
 * @param DataPtr Pointer to the data to be sent.
 * @param DataSize Size of the data in bytes.
 */
void I2C_WriteData(uint8_t I2CAddress, uint8_t *DataPtr, uint8_t DataSize)
{
    I2C_GenerateSTART(I2C1, ENABLE); // Generate START condition
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, I2CAddress << 1, I2C_Direction_Transmitter); // Send slave address
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    for (uint8_t Index = 0; Index < DataSize; Index++) // Transmit data
    {
        I2C_SendData(I2C1, DataPtr[Index]);
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

    I2C_GenerateSTOP(I2C1, ENABLE); // Generate STOP condition
    Delay_Us(5000); // Delay for stability
}

/**
 * @brief Sends a command to the LCD module.
 * 
 * @param Command The command to be sent.
 */
void LCD_SendCommand(uint8_t Command)
{
    uint8_t UpperNibble, LowerNibble;
    uint8_t DataArray[4];

    UpperNibble = (Command & 0xF0); // Extract upper nibble
    LowerNibble = ((Command << 4) & 0xF0); // Extract lower nibble

    DataArray[0] = UpperNibble | 0x0C; // EN=1, RS=0
    DataArray[1] = UpperNibble | 0x08; // EN=0, RS=0
    DataArray[2] = LowerNibble | 0x0C; // EN=1, RS=0
    DataArray[3] = LowerNibble | 0x08; // EN=0, RS=0

    I2C_WriteData(LCD_I2C_SLAVE_ADDRESS, DataArray, 4);
}

/**
 * @brief Sends a character (data) to the LCD module.
 * 
 * @param Data The data (ASCII character) to be sent.
 */
void LCD_SendData(uint8_t Data)
{
    uint8_t UpperNibble, LowerNibble;
    uint8_t DataArray[4];

    UpperNibble = (Data & 0xF0); // Extract upper nibble
    LowerNibble = ((Data << 4) & 0xF0); // Extract lower nibble

    DataArray[0] = UpperNibble | 0x0D; // EN=1, RS=1
    DataArray[1] = UpperNibble | 0x09; // EN=0, RS=1
    DataArray[2] = LowerNibble | 0x0D; // EN=1, RS=1
    DataArray[3] = LowerNibble | 0x09; // EN=0, RS=1

    I2C_WriteData(LCD_I2C_SLAVE_ADDRESS, DataArray, 4);
}

/**
 * @brief Initializes the LCD module in 4-bit mode.
 */
void LCD_Init(void)
{
    LCD_SendCommand(0x33); // Initialize 4-bit interface
    LCD_SendCommand(0x32); // Set to 4-bit mode
    LCD_SendCommand(0x28); // Function set: 4-bit mode, 2-line display
    LCD_SendCommand(0x01); // Clear display
    LCD_SendCommand(0x06); // Entry mode set: Increment cursor, no display shift
    LCD_SendCommand(0x0C); // Display ON, cursor OFF, blink OFF
    LCD_SendCommand(0x02); // Return home
    LCD_SendCommand(0x80); // Set DDRAM address to 0x00 (start of row 0)
}

/**
 * @brief Sends a string of characters to the LCD module.
 * 
 * @param Str Pointer to the null-terminated string to be sent.
 */
void LCD_SendString(const char *Str)
{
    while (*Str)
    {
        LCD_SendData(*Str++); // Send each character
    }
}

/**
 * @brief Clears the display of the LCD module.
 */
void LCD_ClearDisplay(void)
{
    LCD_SendCommand(0x01); // Clear display command
}

/**
 * @brief Sets the cursor position on the LCD module.
 * 
 * @param Row The row number (0 or 1).
 * @param Col The column number (0-15 for a 16x2 LCD).
 */
void LCD_SetCursor(uint8_t Row, uint8_t Col)
{
    uint8_t Address;

    if (Row == 0) // Row 0
    {
        Address = 0x80 + Col; // DDRAM address for Row 0
    }
    else // Row 1
    {
        Address = 0xC0 + Col; // DDRAM address for Row 1
    }

    LCD_SendCommand(Address); // Send command to set DDRAM address
}
