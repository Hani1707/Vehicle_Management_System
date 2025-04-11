#include "rc522.h"


/*
 * Ten ham: Write_MFRC5200
 * Chuc nang: wait 1 byte is stored in register MFRC522
 * Input:addr-> I just wrote down, val-> Value to record
 * Check: No
 */
 uint8_t RC522_SPI_Transfer(uint8_t data)
{
    // Wait until the transmit buffer is empty
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);

    // Send data through the SPI2 peripheral
    SPI_I2S_SendData(SPI2, data);

    // Wait for data to be received
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);

    // Get the received data
    return SPI_I2S_ReceiveData(SPI2);
}


/*
 * Ten ham:Write_MFRC5200
 * Note: wait 1 byte is stored in register MFRC522
 * Input: addr-> DIa write only, val-> Value to write
 * Check: No
 */
void Write_MFRC522(uint8_t addr, uint8_t val)
{
    /* CS LOW */
    // GPIO_ResetBits(MFRC522_CS_GPIO, MFRC522_CS_PIN);  
    GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_RESET);  // Set CS low to begin SPI communication

    // The address is located: 0XXXXXX0 (shift the address and clear the LSB for write)
    RC522_SPI_Transfer((addr << 1) & 0x7E);

    // Transmit the value to write into the register
    RC522_SPI_Transfer(val);

    /* CS HIGH */
    // GPIO_SetBits(MFRC522_CS_GPIO, MFRC522_CS_PIN);  
	GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_SET);  // Set CS high to end SPI communication
}


 /*
 * Ten ham:Read_MFRC522
 * Note: A 1-byte doc is stored from a register MFRC522
 * Input: addr-> address doc
 * Look up: Value in the read register
 */
uint8_t Read_MFRC522(uint8_t addr)
{
    uint8_t val;

    /* CS LOW */
    // GPIO_ResetBits(MFRC522_CS_GPIO, MFRC522_CS_PIN);  
    GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_RESET);  // Set CS low to begin SPI communication

    // The address is located: 1XXXXXX0 (read operation)
    RC522_SPI_Transfer(((addr << 1) & 0x7E) | 0x80);  // Shift address and set read flag (bit 7)

    // Read value from the register
    val = RC522_SPI_Transfer(0x00);  // Send dummy byte to receive data

    /* CS HIGH */
    // GPIO_SetBits(MFRC522_CS_GPIO, MFRC522_CS_PIN);  
    GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_SET);  // Set CS high to end SPI communication

    return val;
}


/*
 * Ten ham:SetBitMask
 * Note: Set bits in an MFRC522 register
 * Input: reg - Registers settings; mask - value set
 * Check: No
 */
void SetBitMask(uint8_t reg, uint8_t mask)  
{
    uint8_t tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp | mask);  // set bit mask
}


/*
 * Ten ham:ClearBitMask
 * Note: Reset bit in register MFRC522
 * Input: reg - Dia register; mask - Value bit can clear
 * Check: No
 */
void ClearBitMask(uint8_t reg, uint8_t mask)  
{
    uint8_t tmp;
    tmp = Read_MFRC522(reg);
    Write_MFRC522(reg, tmp & (~mask));  // clear bit mask
} 


/*
 * Ten Ham:AntennaOn
 * Chuc Nang: Mo antenna, should have at least 1 ms
 * Input: no
 * Check: no
 */
void AntennaOn(void)
{
	Read_MFRC522(TxControlReg);
//	if (!(temp & 0x03))
//	{
//		SetBitMask(TxControlReg, 0x03);
//	}
	SetBitMask(TxControlReg, 0x03);
}


/*
 * Ten ham:AntennaOff
 * Dong Anten, should have at least 1 ms
 * Input: no
 * Check: no
 */
void AntennaOff(void)
{
	ClearBitMask(TxControlReg, 0x03);
}


/*
 * Ten ham:ResetMFRC522
 * Look: Restart RC522
 * Input: No.
 * Return: No.
 */
void MFRC522_Reset(void)
{
    Write_MFRC522(CommandReg, PCD_RESETPHASE);
}


/*
 * Ten ham:MFRC522_SPI_Init
 * Wish: Start SPI
 * Input: No.
 * Tra va: No.
 */
void MFRC522_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    SPI_InitTypeDef SPI_InitStruct;

    // Enable clocks for GPIO (SCK, MISO, MOSI) and CS pin
    RCC_APB2PeriphClockCmd(MFRC522_SPI_GPIO_RCC | MFRC522_CS_RCC | MFRC522_RST_RCC | RCC_APB2Periph_AFIO, ENABLE);

    // Enable clock for SPI2
    RCC_APB1PeriphClockCmd(MFRC522_SPI_RCC, ENABLE);

    // Configure SCK, MISO, and MOSI pins (Alternate Function Push-Pull)
    GPIO_InitStruct.GPIO_Pin = MFRC522_SCK_PIN | MFRC522_MISO_PIN | MFRC522_MOSI_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MFRC522_SPI_GPIO, &GPIO_InitStruct);  // Initialize GPIO pins for SPI

    // Configure CS (Chip Select) pin as output push-pull
    GPIO_InitStruct.GPIO_Pin = MFRC522_CS_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MFRC522_CS_GPIO, &GPIO_InitStruct);   // Initialize CS pin

    // SPI configuration
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  // Set clock prescaler (adjust as needed)
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  // Full-duplex communication
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                       // Master mode
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                   // 8-bit data size
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;                          // Clock polarity low
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;                        // Clock phase 1st edge
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                           // Software NSS management
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                  // Transmit MSB first
    SPI_InitStruct.SPI_CRCPolynomial = 7;                            // Polynomial for CRC (not used)
    SPI_Init(MFRC522_SPI, &SPI_InitStruct);                          // Initialize SPI with these settings

    // Enable SPI
    SPI_Cmd(MFRC522_SPI, ENABLE);
}


/*
 * Ten ham:InitMFRC522
 * Start RC522
 * Input:  No.
 * Look up:  No.
 */
void MFRC522_Init(void)
{
    // Activate the RFID reader by setting CS and RST pins high
    GPIO_WriteBit(GPIOA, GPIO_Pin_9, Bit_SET);  // Set CS pin high (PA9)
    GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);  // Set RST pin high (PA8)
    // GPIO_SetBits(MFRC522_CS_GPIO, MFRC522_CS_PIN);  
    // GPIO_SetBits(MFRC522_RST_GPIO, MFRC522_RST_PIN);  

    // SPI configuration for MFRC522 (already handled in MFRC522_SPI_Init, you can uncomment this if needed)
    MFRC522_SPI_Init();

    // Reset MFRC522
    MFRC522_Reset();

    // Timer configuration: TPrescaler * TreloadVal / 6.78MHz = 24ms
    Write_MFRC522(TModeReg, 0x8D);         // auto=1; f(Timer) = 6.78MHz / TPrescaler
    Write_MFRC522(TPrescalerReg, 0x3E);    // TModeReg[3..0] + TPrescalerReg
    Write_MFRC522(TReloadRegL, 30);        // Reload low byte
    Write_MFRC522(TReloadRegH, 0);         // Reload high byte

    // Configure modulation
    Write_MFRC522(TxAutoReg, 0x40);        // 100% ASK (Amplitude Shift Keying)
    Write_MFRC522(ModeReg, 0x3D);          // CRC preset 0x6363

    // Uncomment these lines if needed for advanced configuration
    // ClearBitMask(Status2Reg, 0x08);      // Disable MFCrypto1On
    // Write_MFRC522(RxSelReg, 0x86);      // RxWait = RxSelReg[5..0]
    // Write_MFRC522(RFCfgReg, 0x7F);      // RxGain = 48dB

    // Turn on the antenna
    AntennaOn();
}



/*
 * Ten ham: MFRC522_ToCard
 * Highlights: information between RC522 and ISO14443
 * Input: command - send to MF522,
 *			 sendData - Remittance sent to the state MFRC522, 
 *			 sendLen - Send the message number
 *			 backData - BackData is returned
 *			 backLen - Retrieve the data number
 * Check: MI_OK if successful
 */
uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint *backLen)
{
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;

    // Set interrupt enable and waitForIRQ based on the command
    switch (command)
    {
        case PCD_AUTHENT:  // Authentication command
            irqEn = 0x12;
            waitIRq = 0x10;
            break;

        case PCD_TRANSCEIVE:  // Transceive command (sending and receiving data)
            irqEn = 0x77;
            waitIRq = 0x30;
            break;

        default:
            break;
    }

    // Enable interrupts in CommIEnReg and clear CommIrqReg
    Write_MFRC522(CommIEnReg, irqEn | 0x80);  // Enable interrupts
    ClearBitMask(CommIrqReg, 0x80);           // Clear interrupt bits

    // Flush the FIFO buffer
    SetBitMask(FIFOLevelReg, 0x80);           // Flush buffer

    // Set PCD_IDLE to cancel any current commands
    Write_MFRC522(CommandReg, PCD_IDLE);

    // Write data to the FIFO
    for (i = 0; i < sendLen; i++)
    {
        Write_MFRC522(FIFODataReg, sendData[i]);  // Send data to FIFO
    }

    // Start the command
    Write_MFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {
        SetBitMask(BitFramingReg, 0x80);  // Start sending data
    }

    // Wait for the response from the card, up to a maximum of 25ms
    i = 2000;  // Maximum time to wait, depending on clock frequency
    do
    {
        n = Read_MFRC522(CommIrqReg);  // Check interrupt status
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));  // Exit when either time expires or valid IRQ

    // Stop sending data
    ClearBitMask(BitFramingReg, 0x80);

    // Check if the operation timed out or succeeded
    if (i != 0)
    {
        // Check for any errors in the operation
        if (!(Read_MFRC522(ErrorReg) & 0x1B))  // Check for BufferOvfl, CollErr, CRCErr, or ProtecolErr
        {
            status = MI_OK;  // No error, operation was successful

            // Check if the card is no longer present
            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;  // No tag found
            }

            // If transceive command, process the received data
            if (command == PCD_TRANSCEIVE)
            {
                n = Read_MFRC522(FIFOLevelReg);  // Number of bytes in FIFO
                lastBits = Read_MFRC522(ControlReg) & 0x07;  // Check for incomplete bytes in last transmission
                if (lastBits)
                {
                    *backLen = (n - 1) * 8 + lastBits;  // Calculate total bits received
                }
                else
                {
                    *backLen = n * 8;  // Full byte data received
                }

                // Limit the number of bytes to maximum buffer size
                if (n == 0)
                {
                    n = 1;
                }
                if (n > MAX_LEN)
                {
                    n = MAX_LEN;
                }

                // Read the received data from FIFO
                for (i = 0; i < n; i++)
                {
                    backData[i] = Read_MFRC522(FIFODataReg);  // Read received data
                }
            }
        }
        else
        {
            status = MI_ERR;  // Error in operation
        }
    }

	//SetBitMask(ControlReg,0x80);           //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE); 
	
    return status;  // Return the status of the operation
}


/*
 * Ten ham:MFRC522_Request
 * Show it, read it
 * Input: reqMode - Phat is able,
 *			 TagType - Type of check
 *			 	0x4400 = Mifare_UltraLight
 *				0x0400 = Mifare_One(S50)
 *				0x0200 = Mifare_One(S70)
 *				0x0800 = Mifare_Pro(X)
 *				0x4403 = Mifare_DESFire
 * Return: MI_OK if the bar is curved
 */
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
{
	uint8_t status;  
	uint backBits;			//The bits are manipulated

	Write_MFRC522(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]	???
	
	TagType[0] = reqMode;
	status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{    
		status = MI_ERR;
	}
   
	return status;
}

/*
 * Ten ham:MFRC522_Anticoll
 * Detect the collision, select and read the serial number
 * Input: serNum - Look up the serial the 4 byte, byte 5 is the ma checksum
 * Check: MI_OK if successful
 */
uint8_t MFRC522_Anticoll(uint8_t *serNum)
{
    uint8_t status;
    uint8_t i;
	uint8_t serNumCheck=0;
    uint unLen;
    

    //ClearBitMask(Status2Reg, 0x08);		//TempSensclear
    //ClearBitMask(CollReg,0x80);			//ValuesAfterColl
	Write_MFRC522(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]
 
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
	{
		//Check the serial number
		for (i=0; i<4; i++)
		{   
		 	serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{   
			status = MI_ERR;    
		}
    }

    //SetBitMask(CollReg, 0x80);		//ValuesAfterColl=1

    return status;
} 


/*
 * Ten Ham:CalulateCRC
 * MFRC522 is a formula of RC522
 * Input: pIndata - Data CRC into calculator, wool - Data data, pOutData - CRC calculation
 * Check: No
 */
void CalulateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;

    ClearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
    SetBitMask(FIFOLevelReg, 0x80);			//Con tro FIFO
    //Write_MFRC522(CommandReg, PCD_IDLE);

	//Record in FIFO
    for (i=0; i<len; i++)
    {   
		Write_MFRC522(FIFODataReg, *(pIndata+i));   
	}
    Write_MFRC522(CommandReg, PCD_CALCCRC);

	// Let the CRC computer complete
    i = 0xFF;
    do 
    {
        n = Read_MFRC522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));			//CRCIrq = 1

	//Doc results in CRC calculation
    pOutData[0] = Read_MFRC522(CRCResultRegL);
    pOutData[1] = Read_MFRC522(CRCResultRegM);
}


/*
 * Ten ham:MFRC522_SelectTag
 * read the right way
 * Input:serNum--So serial the
 * Check: Use the same amount of inspection
 */
uint8_t MFRC522_SelectTag(uint8_t *serNum)
{
	uint8_t i;
	uint8_t status;
	uint8_t size;
	uint recvBits;
	uint8_t buffer[9]; 

	//ClearBitMask(Status2Reg, 0x08);			//MFCrypto1On=0

    buffer[0] = PICC_SElECTTAG;
    buffer[1] = 0x70;
    for (i=0; i<5; i++)
    {
    	buffer[i+2] = *(serNum+i);
    }
	CalulateCRC(buffer, 7, &buffer[7]);		//??
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);
    
    if ((status == MI_OK) && (recvBits == 0x18))
    {   
		size = buffer[0]; 
	}
    else
    {   
		size = 0;    
	}

    return size;
}


/*
 * Ten Ham:MFRC522_Auth
 * Identify the bad face
 * Input: authMode - Check your password
                 0x60 = Film confirmation A
                 0x61 = Film confirmation B
             BlockAddr - Addresses
             Sectorkey - The shadow area
             serNum - So serial the, 4 bytes
 * Check: MI_OK if successful
 */

#include "stdio.h"
extern void UART_SendString(USART_TypeDef* USARTx, char* str);
extern char str3[17];

uint8_t MFRC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *Sectorkey, uint8_t *serNum)
{
    uint8_t status;
    uint recvBits;
    uint8_t i;
	uint8_t buff[12]; 

	// Confirmation + Address + password + quick number
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i=0; i<6; i++)
    {    
		buff[i+2] = *(Sectorkey+i);   
	}
    for (i=0; i<4; i++)
    {    
		buff[i+8] = *(serNum+i);   
	}

    status = MFRC522_ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);
	
    if ((status != MI_OK) || (!(Read_MFRC522(Status2Reg) & 0x08)))
    {   
		status = MI_ERR;   
	}
    
    return status;
}


/*
 * Ten ham:MFRC522_Read
 * Doc with data
 * Input: blockAddr - Address location; recvData - Retrieve document output
 * Check: MI_OK if successful
 */
uint8_t MFRC522_Read(uint8_t blockAddr, uint8_t *recvData)
{
    uint8_t status;
    uint unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    CalulateCRC(recvData,2, &recvData[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((status != MI_OK) || (unLen != 0x90))
    {
        status = MI_ERR;
    }
    
    return status;
}



/*
 * Ten ham:MFRC522_Write
 * wait repeats data
 * Input: blockAddr - locations; writeData - write data
 * Check: MI_OK if successful
 */
uint8_t MFRC522_Write(uint8_t blockAddr, uint8_t *writeData)
{
    uint8_t status;
    uint recvBits;
    uint8_t i;
	uint8_t buff[18]; 
    
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    CalulateCRC(buff, 2, &buff[2]);
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {   
		status = MI_ERR;   
	}
        
    if (status == MI_OK)
    {
        for (i=0; i<16; i++)		//16 FIFO bytes recorded
        {    
        	buff[i] = *(writeData+i);   
        }
        CalulateCRC(buff, 16, &buff[16]);
        status = MFRC522_ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
        
		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {   
			status = MI_ERR;   
		}
    }
    
    return status;
}


/*
 * Ten ham:MFRC522_Halt
 * CHuc nang: Dua the vao ngu dong
 * Input: NO.
 * Look up: NO.
 */
void MFRC522_Halt(void)
{
	uint unLen;
	uint8_t buff[4]; 

	buff[0] = PICC_HALT;
	buff[1] = 0;
	CalulateCRC(buff, 2, &buff[2]);
 
	MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}
void MFRC522_StopCrypto1(void) {
	// Clear MFCrypto1On bit
	ClearBitMask(Status2Reg, 0x08); // Status2Reg[7..0] bits are: TempSensClear I2CForceHS reserved reserved   MFCrypto1On ModemState[2:0]
} // End PCD_StopCrypto1()
