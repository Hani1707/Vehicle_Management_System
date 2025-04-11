#include "stm32f10x.h"       // SPL header for STM32F1
#include "stm32f10x_gpio.h"  // GPIO Library for SPL
#include "stm32f10x_spi.h"   // SPI Library for SPL
#include "stm32f10x_usart.h" // UART/USART Library for SPL
#include "stm32f10x_rcc.h"   // RCC (Reset and Clock Control) Library for SPL
#include "stm32f10x_flash.h" // Keil::Device:StdPeriph Drivers:Flash
#include "stm32f10x_exti.h"  // Keil::Device:StdPeriph Drivers:EXTI

#include "i2c-lcd.h"
#include "delay.h"
#include "rc522.h"
#include "string.h"
#include "stdio.h"
#include "fatfs.h"
#include "fatfs_sd.h"
#include <stdbool.h>

/* Macro pins of items for RC522 */
#define LED1_Pin GPIO_Pin_3
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_Pin_4
#define LED2_GPIO_Port GPIOB
#define Button2_Pin GPIO_Pin_8 // This button use for adding new user SD Card
#define Button2_GPIO_Port GPIOB
#define Button1_Pin GPIO_Pin_5 // This button use for earasing current user SD Card
#define Button1_GPIO_Port GPIOB

/* Macro pins of items for SDCard */

/* Private variables for RC522 */
uint8_t Key1;
uint8_t Key2;
uint8_t i;
uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16

uint8_t serNum[5];

// It will be used for the password operation.
// char password[16]="123456"; //Max lenght of password is 16 charaters
// char keypass[16];
// int cnt=0;

uint8_t value = 0;
char str1[50] = {'\0'};
char str2[25] = {'\0'};
char str3[25] = {'\0'};
char str4[25] = {'\0'};
char tmp_str[65] = {'\0'};

uint8_t UID[5];
/*******************************/

/* Private variables for SDCard*/
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t total;
UINT bw;
/*******************************/

/****** Function prototypes ****/
void SystemClock_Config(void);
static void MX_RCC_Init(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);		// DISPLAYING LCD
static void MX_SPI1_Init(void); 	// SDCard comunication, RC522 doesn't need, because it's included by MFRC522_Init()
static void MX_SPI1_DeInit(void);
static void MX_USART3_UART_Init(void);	// DISPLAYING TERMINAL
static void EXTI_Config(void);
void UART_SendString(USART_TypeDef *USARTx, char *str);
/*******************************/

/****** Pravte Function ********/

/**
 * @brief Authorizes an SD card by matching UID against stored data.
 * @retval None
 *
 * Reinitializes SPI and FATFS, mounts the SD card, and checks the "user_data.csv" file for a matching UID.
 * Sends authorization status via UART3.
 */
void AutCard(void)
{
    // Reinitialize SPI1 and FATFS before reading the SD card
    MX_SPI1_Init();
    MX_FATFS_Init();

    char buffer[128];
    bool uid_found = false;

    // Mount the SD card
    fres = f_mount(&fs, "", 0);
    if (fres != FR_OK)
    {
        sprintf(str1, "Error mounting SD card\n\r");
        UART_SendString(USART3, str1);
        while (1)
            ; // D?ng chuong trình n?u không mount du?c
    }

    // Open file
    fres = f_open(&fil, "user_data.csv", FA_READ);
    if (fres != FR_OK)
    {
        sprintf(str1, "Error opening file\n\r");
        UART_SendString(USART3, str1);
        while (1)
            ;
    }

    // Move courso to ahead of the file
    fres = f_lseek(&fil, 0);
    if (fres != FR_OK)
    {
        sprintf(str1, "Error seeking to the beginning of file\n\r");
        UART_SendString(USART3, str1);
    }

    while (f_gets(buffer, sizeof(buffer), &fil))
    {
        buffer[strcspn(buffer, "\r\n")] = 0; // Lo?i b? ký t? newline
        char uid_str[16];
        sprintf(uid_str, "%02x %02x %02x %02x", UID[0], UID[1], UID[2], UID[3]);
        if (strstr(buffer, uid_str))
        {
            uid_found = true;
            break;
        }
    }

    // Send message to Terminal UART3
    if (uid_found)
    {
        sprintf(str1, "Card is authorized\n\r");
        UART_SendString(USART3, str1);
    }
    else
    {
        sprintf(str1, "Card is not recognized\n\r");
        UART_SendString(USART3, str1);
    }

    // Deinitialize SPI1 and FATFS after reading the SD card
    MX_FATFS_DeInit();
    MX_SPI1_DeInit();
}

/**
 * @brief Removes a specified UID from the "user_data.csv" file on an SD card.
 * @param UID Pointer to a 4-byte array representing the UID to remove.
 * @retval bool True if the UID was successfully removed, false otherwise.
 *
 * Mounts the SD card, reads the file line by line, and removes the line containing the specified UID.
 * Updates the file content and truncates excess data. Deinitializes peripherals after operation.
 */
bool RemoveUIDFromFile(uint8_t *UID)
{
    char buffer[128];
    char uid_str[16];

    // Convert UID to string format
    sprintf(uid_str, "%02x %02x %02x %02x", UID[0], UID[1], UID[2], UID[3]);

    // Initialize SPI1 and FATFS
    MX_SPI1_Init();
    MX_FATFS_Init();

    // Mount the SD card
    fres = f_mount(&fs, "", 0);
    if (fres != FR_OK)
    {
        return false; // Mount failed
    }

    // Open the file in read/write mode
    fres = f_open(&fil, "user_data.csv", FA_READ | FA_WRITE);
    if (fres != FR_OK)
    {
        f_mount(NULL, "", 1);
        return false; // Open failed
    }

    // Move file pointer to the beginning
    if (f_lseek(&fil, 0) != FR_OK)
    {
        f_close(&fil);
        f_mount(NULL, "", 1);
        return false; // lseek failed
    }

    // Temporary buffer to store updated file content
    DWORD write_pos = 0; // Position to overwrite lines
    while (f_gets(buffer, sizeof(buffer), &fil))
    {
        buffer[strcspn(buffer, "\r\n")] = 0; // Remove newline characters

        // Debug: Print each line read
        UART_SendString(USART3, "Read line: \n\r");
        UART_SendString(USART3, buffer);

        // Skip the line containing the UID
        if (strstr(buffer, uid_str) != NULL)
        {
            UART_SendString(USART3, "Found UID, skipping line\n\r");
            continue; // Do not write this line back
        }

        // Otherwise, write the line back to the file
        UINT bw;
        if (f_lseek(&fil, write_pos) != FR_OK)
        { // Set position for overwriting
            f_close(&fil);
            f_mount(NULL, "", 1);
            return false; // lseek failed
        }

        fres = f_write(&fil, buffer, strlen(buffer), &bw); // Write the line
        if (fres != FR_OK || bw != strlen(buffer))
        {
            f_close(&fil);
            f_mount(NULL, "", 1);
            return false; // Write failed
        }

        // Write newline character
        fres = f_write(&fil, "\n", 1, &bw);
        if (fres != FR_OK || bw != 1)
        {
            f_close(&fil);
            f_mount(NULL, "", 1);
            return false; // Write failed
        }

        // Update write position
        write_pos += strlen(buffer) + 1; // +1 for newline
    }

    // Truncate the file to remove leftover content
    if (f_lseek(&fil, write_pos) != FR_OK)
    {
        f_close(&fil);
        f_mount(NULL, "", 1);
        return false; // lseek failed
    }

    if (f_truncate(&fil) != FR_OK)
    {
        f_close(&fil);
        f_mount(NULL, "", 1);
        return false; // Truncate failed
    }

    // Close the file and unmount the SD card
    f_close(&fil);
    f_mount(NULL, "", 1);

    // Deinitialize SPI1 and FATFS
    MX_FATFS_DeInit();
    MX_SPI1_DeInit();

    UART_SendString(USART3, "UID successfully removed\n\r");
    return true;
}

/**
 * @brief Adds a specified UID to the "user_data.csv" file on an SD card.
 * @param UID Pointer to a 4-byte array representing the UID to add.
 * @retval bool True if the UID was successfully added, false otherwise.
 *
 * Mounts the SD card, checks if the UID already exists in the file, and appends it to the end if not.
 * Deinitializes peripherals after operation and handles errors with appropriate feedback via UART3.
 */
bool AddUIDToFile(uint8_t *UID)
{
    char buffer[128];
    char uid_str[16];

    // Convert UID to string format
    sprintf(uid_str, "%02x %02x %02x %02x", UID[0], UID[1], UID[2], UID[3]);

    // Initialize SPI1 and FATFS
    MX_SPI1_Init();
    MX_FATFS_Init();

    // Mount the SD card
    fres = f_mount(&fs, "", 0);
    if (fres != FR_OK)
    {
        UART_SendString(USART3, "Error mounting SD card\n\r");
        MX_SPI1_DeInit();
        return false;
    }

    // Open the file in read/write mode
    fres = f_open(&fil, "user_data.csv", FA_READ | FA_WRITE);
    if (fres != FR_OK)
    {
        UART_SendString(USART3, "Error opening file\n\r");
        f_mount(NULL, "", 1);
        MX_SPI1_DeInit();
        return false; // File open failed
    }

    // Move to the end of the file to append new data
    if (f_lseek(&fil, f_size(&fil)) != FR_OK)
    {
        UART_SendString(USART3, "Error seeking to file end\n\r");
        f_close(&fil);
        f_mount(NULL, "", 1);
        MX_SPI1_DeInit();
        return false;
    }

    // Check if UID already exists in the file
    char temp_buffer[128];
    while (f_gets(temp_buffer, sizeof(temp_buffer), &fil))
    {
        temp_buffer[strcspn(temp_buffer, "\r\n")] = 0; // Remove newline characters
        if (strcmp(temp_buffer, uid_str) == 0)
        {
            UART_SendString(USART3, "UID already exists\n\r");
            f_close(&fil);
            f_mount(NULL, "", 1);
            MX_SPI1_DeInit();
            return false; // UID already exists
        }
    }

    // Write the new UID to the file
    UINT bw;
    fres = f_write(&fil, uid_str, strlen(uid_str), &bw);
    if (fres != FR_OK || bw != strlen(uid_str))
    {
        UART_SendString(USART3, "Error writing UID\n\r");
        f_close(&fil);
        f_mount(NULL, "", 1);
        MX_SPI1_DeInit();
        return false; // Write failed
    }

    // Write newline character after UID
    fres = f_write(&fil, "\n", 1, &bw);
    if (fres != FR_OK || bw != 1)
    {
        UART_SendString(USART3, "Error writing newline\n\r");
        f_close(&fil);
        f_mount(NULL, "", 1);
        MX_SPI1_DeInit();
        return false; // Write failed
    }

    // Close file and unmount SD card
    f_close(&fil);
    f_mount(NULL, "", 1);

    // Deinitialize SPI1 and FATFS
    MX_FATFS_DeInit();
    MX_SPI1_DeInit();

    UART_SendString(USART3, "UID added successfully22\n\r");
    return true;
}
/*******************************/
volatile uint8_t button1Pressed = 0;
volatile uint8_t button2Pressed = 0;
int main(void)
{
    /* System Initialization */
    SystemClock_Config();
    MX_RCC_Init();
	TIM2_Config();
    MX_GPIO_Init();
	MX_I2C1_Init();
    MX_USART3_UART_Init();
	EXTI_Config();
	
    Sys_DelayInit();
    while (DWT_Delay_Init()){}

    GPIO_SetBits(GPIOA, GPIO_Pin_8);
    Delay_Ms(100);
    MFRC522_Init();
    Delay_Ms(1000);

    uint8_t status, cardstr[MAX_LEN + 1];
    uint8_t card_data[17];
    uint32_t delay_val = 1000; // ms
    uint16_t result = 0;

    // a private key to scramble data writing/reading to/from RFID card:
    uint8_t Mx1[7][5] = {{0x12, 0x45, 0xF2, 0xA8}, {0xB2, 0x6C, 0x39, 0x83}, {0x55, 0xE5, 0xDA, 0x18}, {0x1F, 0x09, 0xCA, 0x75}, {0x99, 0xA2, 0x50, 0xEC}, {0x2C, 0x88, 0x7F, 0x3D}};

    uint8_t SectorKey[7];

	LCD_Init();
	LCD_ClearDisplay();
	LCD_SetCursor(0, 0);
	LCD_SendString("Ready to operate");

    sprintf(str1, "Ready to operate\n\r");
    UART_SendString(USART3, str1);

	bool stateUpdated = false;

	while (1) 
	{
		// 1. Detect card
		status = MFRC522_Request(PICC_REQIDL, cardstr);
		if (status != MI_OK) {
			if (!stateUpdated) {
				// Displaying Terminal
				sprintf(str1, "Waiting for Card\n\r");
				UART_SendString(USART3, str1);
				// Displaying LCD
				LCD_SetCursor(1, 0);
				LCD_SendString("Waiting for Card");
				Delay_Ms(100);

				stateUpdated = true;
			}
			continue;
		}

		// 2. Read UID
		status = MFRC522_Anticoll(cardstr);
		if (status == MI_OK) {
			memcpy(UID, cardstr, 5);
			// Displaying Terminal
			sprintf(str2, "UID: %02x %02x %02x %02x\n\r", UID[0], UID[1], UID[2], UID[3]);
			UART_SendString(USART3, str2);

			// 3. Check UID in CSV
			AutCard();

			stateUpdated = false;
		}
		if (button1Pressed) 
		{
			button1Pressed = 0;  // Clear flag
			Delay_Ms(10);
		}

		if (button2Pressed) 
		{
			button2Pressed = 0;  // Clear flag
			Delay_Ms(10);
		}
	}

}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    /* Enable HSE (High-Speed External) Oscillator */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    if (RCC_WaitForHSEStartUp() == SUCCESS)
    {
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        /* Set Flash latency to 2 wait states */
        FLASH_SetLatency(FLASH_Latency_2);

        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK (APB2 clock) */
        RCC_PCLK2Config(RCC_HCLK_Div1);

        /* PCLK1 = HCLK/2 (APB1 clock) */
        RCC_PCLK1Config(RCC_HCLK_Div2);

        /* Configure PLL: PLLCLK = HSE * 9 = 72 MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

        /* Enable PLL */
        RCC_PLLCmd(ENABLE);

        /* Wait till PLL is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
            ;

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x08)
            ; // 0x08 means PLL is used as system clock
    }
    else
    {
        while (1)
            ;
    }
}

/**
 * @brief Peripheral RCC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RCC_Init(void)
{
    /* Enable the clock for SPI1 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* Enable the clock for SPI2 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* Enable the clock for USART3 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	/* Enable the clock for I2C1 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* Enable the clock for GPIO */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
						| RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
}
/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{
    SPI_InitTypeDef SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable the clock for SPI1 and GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);

    // Configure GPIO pins for SPI1: SCK, MISO, MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure CS pin (PA4) as Output Push-Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_4); // Set CS high (inactive, not selecting slave)

    // Configure SPI in Master mode
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // NSS managed by software
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &SPI_InitStructure);

    // Enable SPI1
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief SPI1 DeInitialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_DeInit(void)
{
    // Disable SPI1
    SPI_Cmd(SPI1, DISABLE);

    // Deinitialize SPI1 peripheral
    SPI_I2S_DeInit(SPI1);

    // Deinitialize GPIO pins used by SPI1
    GPIO_InitTypeDef GPIO_InitStruct;

    // Configure SCK, MISO, MOSI as analog or input to save power
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; // SPI1_SCK, SPI1_MISO, SPI1_MOSI
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;                       // Analog input (low power mode)
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // If using CS pin, set it to high to deselect SD card
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;        // SPI1_CS pin
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; // Push-pull output
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_SetBits(GPIOA, GPIO_Pin_4); // Set CS high
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
void MX_I2C1_Init()
{
    I2C_InitTypeDef I2C_InitStructure;

	I2C_DeInit(I2C1);
	I2C_Cmd(I2C1, DISABLE);
	
    I2C_InitStructure.I2C_ClockSpeed = 400000; // Fast mode
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00; // Address Master
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
    
    // 1. Configure SCL and SDA as GPIO Output Push-Pull
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 2. Generate 8 clock pulses on SCL to release SDA
    for (volatile int i = 0; i < 8; i++) {
        // Set SCL high
        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        Delay_Us(1000);
        
        // Set SCL low
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        Delay_Us(1000);
    }
    
    // 3. Set both SCL and SDA to high level
    GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);
	Delay_Us(1000);
	
    // 4. Reconfigure pins for I2C mode (Alternate Function Open-Drain)
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void)
{
    USART_InitTypeDef USART_InitStructure;

    /* USART3 configuration */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    /* Initialize USART3 */
    USART_Init(USART3, &USART_InitStructure);

    /* Enable USART3 */
    USART_Cmd(USART3, ENABLE);
}


/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* === 1. Set Initial Pin Output Levels === */
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9); // Set PA8, PA9 LOW
    GPIO_ResetBits(GPIOB, LED1_Pin | LED2_Pin);     // Set LED1, LED2 LOW
    GPIO_SetBits(SD_CS_GPIO_Port, SD_CS_Pin);       // Set SD_CS_Pin HIGH (inactive)
    GPIO_ResetBits(SD_LED1_GPIO_Port, SD_LED1_Pin); // Set SD_LED1_Pin LOW (active)
    GPIO_ResetBits(SD_LED2_GPIO_Port, SD_LED2_Pin); // Set SD_LED2_Pin LOW (active)

    /* === 2. Configure Output Pins === */
    // Configure PA8, PA9 as output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // Push-Pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // High speed
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure LED1_Pin, LED2_Pin as output push-pull
    GPIO_InitStructure.GPIO_Pin = LED1_Pin | LED2_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // Push-Pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // Medium speed
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure SD_CS_Pin as output push-pull
    GPIO_InitStructure.GPIO_Pin = SD_CS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // Push-Pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; // Medium speed
    GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStructure);

    // Configure SD_LED1_Pin, SD_LED2_Pin as output push-pull
    GPIO_InitStructure.GPIO_Pin = SD_LED1_Pin | SD_LED2_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // Push-Pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;   // Medium speed
    GPIO_Init(SD_LED1_GPIO_Port, &GPIO_InitStructure); // LEDs are on GPIOA

    /* === 3. Configure Input Pins === */
    // Configure Key1_Pin, Key2_Pin as input with pull-up
    GPIO_InitStructure.GPIO_Pin = Button2_Pin | Button1_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // Input with pull-up
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure SD_WREN_Pin, SD_DETECT_Pin as input with pull-up
    GPIO_InitStructure.GPIO_Pin = SD_WREN_Pin | SD_DETECT_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // Input with pull-up
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* === 4. Configure Peripheral Pins === */
    // Disable JTAG to free PB3, PB4 (SWJ remap)
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    // Configure USART3 TX (PB10) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // High speed
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   // Alternate function push-pull
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure USART3 RX (PB11) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Input floating
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void EXTI_Config(void) 
{
    // Configure EXTI5 for PB5
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line5;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Trigger on falling edge
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	
   // Configure EXTI8 for PB8
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
    EXTI_InitStructure.EXTI_Line = EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  // Trigger on falling edge
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // Enable NVIC for EXTI5 (handled by EXTI9_5 IRQ handler)
    NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_SetPriority(SysTick_IRQn, 1);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable NVIC for EXTI8 (also handled by EXTI9_5 IRQ handler)
    // No additional NVIC configuration needed as EXTI9_5 covers both EXTI5 and EXTI8
}


void EXTI9_5_IRQHandler(void) {
    // Check if EXTI5 (PB5 - Button1) triggered the interrupt
    if ((EXTI_GetITStatus(EXTI_Line5) != RESET) && (button1Pressed == 0)) 
	{
		button1Pressed = 1;
	}
	
	if (button1Pressed == 1)
	{
        // PB5 action (Remove UID from file)
        RemoveUIDFromFile(UID);
		
		// Clear the interrupt flag
        EXTI_ClearITPendingBit(EXTI_Line5);
	}


    // Check if EXTI8 (PB8 - Button2) triggered the interrupt
    if ((EXTI_GetITStatus(EXTI_Line8) != RESET) && (button2Pressed == 0)) 
	{
		button2Pressed = 1;
	}
	
	if (button2Pressed == 1)
	{
        // PB8 action (Add UID to file) 
        AddUIDToFile(UID);

        // Clear the interrupt flag
        EXTI_ClearITPendingBit(EXTI_Line8);
	
    }
}

/**
 * @brief Sends a string via UART.
 * @param USARTx UART instance.
 * @param str Pointer to the string to send.
 * @retval None
 */
void UART_SendString(USART_TypeDef *USARTx, char *str)
{
    while (*str)
    {
        USART_SendData(USARTx, *str++);
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
            ;
    }
}
