/*
- Wiring:
- CC1101 Pin 1 -> STM32 GND
- CC1101 Pin 2 -> STM32 3V3
- CC1101 Pin 3 -> STM32 PWM/D3 (GDO0)
- CC1101 Pin 4 -> STM32 A2
- CC1101 Pin 5 -> STM32 SCK/D13 (PA5 - SCK)
- CC1101 Pin 6 -> STM32 PWM/MOSI/D11 (PA7 - MOSI)
- CC1101 Pin 7 -> STM32 MISO/D12 (PA6 - MISO)
- CC1101 Pin 8 -> STM32 PWM/D6 (GDO2)
 **/

#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* =========================================================
   PERIPHERAL HANDLES
   ========================================================= */

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* =========================================================
   CC1101 PIN DEFINITIONS
   ========================================================= */

#define CC1101_CS_PORT       GPIOA
#define CC1101_CS_PIN        GPIO_PIN_4

#define CC1101_GDO0_PORT     GPIOB
#define CC1101_GDO0_PIN      GPIO_PIN_3

#define CC1101_SELECT()      HAL_GPIO_WritePin(CC1101_CS_PORT, CC1101_CS_PIN, GPIO_PIN_RESET)
#define CC1101_DESELECT()    HAL_GPIO_WritePin(CC1101_CS_PORT, CC1101_CS_PIN, GPIO_PIN_SET)

/* =========================================================
   CC1101 COMMAND STROBES
   ========================================================= */

#define CC1101_SRES          0x30
#define CC1101_SFSTXON       0x31
#define CC1101_SXOFF         0x32
#define CC1101_SCAL          0x33
#define CC1101_SRX           0x34
#define CC1101_STX           0x35
#define CC1101_SIDLE         0x36
#define CC1101_SFRX          0x3A
#define CC1101_SFTX          0x3B

/* =========================================================
   CC1101 REGISTERS
   ========================================================= */

#define CC1101_IOCFG0        0x02
#define CC1101_PKTLEN        0x06
#define CC1101_FREQ2         0x0D
#define CC1101_FREQ1         0x0E
#define CC1101_FREQ0         0x0F
#define CC1101_MDMCFG4       0x10
#define CC1101_MDMCFG3       0x11
#define CC1101_MDMCFG2       0x12
#define CC1101_MDMCFG1       0x13
#define CC1101_MDMCFG0       0x14
#define CC1101_DEVIATN       0x15
#define CC1101_MCSM1         0x17
#define CC1101_MCSM0         0x18

#define CC1101_PATABLE       0x3E
#define CC1101_TXFIFO        0x3F

#define CC1101_MARCSTATE     0x35
#define CC1101_TXBYTES       0x3A

/* =========================================================
   FUNCTIONS DECLARATION
   ========================================================= */
void Error_Handler(void);

/* =========================================================
   UART PRINT FUNCTION
   ========================================================= */

void UART_Printf(const char *format, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, format);

    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if (len > 0)
    {
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;

        HAL_UART_Transmit(&huart2,
                          (uint8_t *)buffer,
                          len,
                          HAL_MAX_DELAY);
    }
}

/* =========================================================
   STM32 CLOCK CONFIGURATION
   Uses internal 16 MHz HSI clock.
   ========================================================= */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

/* =========================================================
   GPIO INITIALIZATION
   ========================================================= */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* CC1101 chip select */

    HAL_GPIO_WritePin(CC1101_CS_PORT,
                      CC1101_CS_PIN,
                      GPIO_PIN_SET);

    GPIO_InitStruct.Pin = CC1101_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(CC1101_CS_PORT, &GPIO_InitStruct);

    /* CC1101 GDO0 input */

    GPIO_InitStruct.Pin = CC1101_GDO0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(CC1101_GDO0_PORT, &GPIO_InitStruct);
}

/* =========================================================
   SPI1 INITIALIZATION
   PA5 = SCK
   PA6 = MISO
   PA7 = MOSI
   ========================================================= */

static void MX_SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin =
        GPIO_PIN_5 |
        GPIO_PIN_6 |
        GPIO_PIN_7;

    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        Error_Handler();
}

/* =========================================================
   USART2 INITIALIZATION
   PA2 = TX
   PA3 = RX
   ========================================================= */

static void MX_USART2_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

/* =========================================================
   CC1101 SPI HELPERS
   ========================================================= */

static uint8_t CC1101_WaitReady(void)
{
    uint32_t timeout = HAL_GetTick() + 100;

    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
    {
        if ((int32_t)(HAL_GetTick() - timeout) >= 0)
            return 0;
    }

    return 1;
}

static void CC1101_WriteReg(uint8_t address, uint8_t value)
{
    uint8_t data[2];

    data[0] = address;
    data[1] = value;

    CC1101_SELECT();

    if (CC1101_WaitReady())
    {
        HAL_SPI_Transmit(&hspi1, data, 2, 100);
    }

    CC1101_DESELECT();
}

static uint8_t CC1101_ReadStatus(uint8_t address)
{
    uint8_t tx[2];
    uint8_t rx[2] = {0};

    tx[0] = address | 0xC0;
    tx[1] = 0x00;

    CC1101_SELECT();

    if (!CC1101_WaitReady())
    {
        CC1101_DESELECT();
        return 0xFF;
    }

    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);

    CC1101_DESELECT();

    return rx[1];
}

static void CC1101_Strobe(uint8_t command)
{
    CC1101_SELECT();

    if (CC1101_WaitReady())
    {
        HAL_SPI_Transmit(&hspi1, &command, 1, 100);
    }

    CC1101_DESELECT();
}

static void CC1101_WriteBurst(uint8_t address,
                              uint8_t *data,
                              uint8_t length)
{
    CC1101_SELECT();

    if (CC1101_WaitReady())
    {
        uint8_t command = address | 0x40;

        HAL_SPI_Transmit(&hspi1, &command, 1, 100);
        HAL_SPI_Transmit(&hspi1, data, length, 100);
    }

    CC1101_DESELECT();
}

/* =========================================================
   CC1101 RESET
   ========================================================= */

static void CC1101_Reset(void)
{
    CC1101_DESELECT();
    HAL_Delay(5);

    CC1101_Strobe(CC1101_SRES);

    HAL_Delay(10);
}

/* =========================================================
   CC1101 INITIALIZATION
   433.92 MHz
   2-FSK
   26 MHz crystal
   Approximately 4.73 kbit/s
   ========================================================= */

static void CC1101_Configure(void)
{
    uint8_t paValue = 0x12;

    CC1101_Reset();

    /* GDO0: sync word sent/received */

    CC1101_WriteReg(0x00, 0x2E);
    CC1101_WriteReg(0x01, 0x2E);
    CC1101_WriteReg(0x02, 0x06);

    /* Packet configuration */

    CC1101_WriteReg(0x03, 0x47);
    CC1101_WriteReg(0x04, 0xD3);
    CC1101_WriteReg(0x05, 0x91);
    CC1101_WriteReg(0x06, 0x0C);
    CC1101_WriteReg(0x07, 0x00);
    CC1101_WriteReg(0x08, 0x04);
    CC1101_WriteReg(0x09, 0x00);
    CC1101_WriteReg(0x0A, 0x00);

    /* Frequency control: 433.92 MHz */

    CC1101_WriteReg(0x0B, 0x06);
    CC1101_WriteReg(0x0C, 0x00);
    CC1101_WriteReg(0x0D, 0x10);
    CC1101_WriteReg(0x0E, 0xB0);
    CC1101_WriteReg(0x0F, 0x71);

    /* Modem configuration */

    CC1101_WriteReg(0x10, 0xF7);
    CC1101_WriteReg(0x11, 0x7E);
    CC1101_WriteReg(0x12, 0x02);
    CC1101_WriteReg(0x13, 0x22);
    CC1101_WriteReg(0x14, 0xF8);
    CC1101_WriteReg(0x15, 0x14);

    /* State machine */

    CC1101_WriteReg(0x16, 0x07);
    CC1101_WriteReg(0x17, 0x00);
    CC1101_WriteReg(0x18, 0x18);

    /* Frequency offset and bit synchronization */

    CC1101_WriteReg(0x19, 0x16);
    CC1101_WriteReg(0x1A, 0x6C);

    /* Automatic gain control */

    CC1101_WriteReg(0x1B, 0x43);
    CC1101_WriteReg(0x1C, 0x40);
    CC1101_WriteReg(0x1D, 0x91);

    /* Front-end configuration */

    CC1101_WriteReg(0x21, 0x56);
    CC1101_WriteReg(0x22, 0x10);

    /* Low-power PA setting */

    CC1101_WriteBurst(CC1101_PATABLE, &paValue, 1);

    CC1101_Strobe(CC1101_SIDLE);
    HAL_Delay(2);

    UART_Printf("CC1101 configured for 433.92 MHz, 2-FSK\r\n");
}

/* =========================================================
   TRANSMIT HELLO WORLD
   ========================================================= */

static void CC1101_TransmitHelloWorld(void)
{
    uint8_t message[] = "Hello World!";
    uint8_t length = sizeof(message) - 1;

    uint32_t startTime;
    uint32_t lastPrint = 0;

    UART_Printf("\r\nPreparing TX packet...\r\n");

    /* Put radio in idle */

    CC1101_Strobe(CC1101_SIDLE);
    HAL_Delay(2);

    /* Flush TX FIFO */

    CC1101_Strobe(CC1101_SFTX);
    HAL_Delay(2);

    /* Load packet */

    CC1101_WriteBurst(CC1101_TXFIFO, message, length);

    UART_Printf("TX FIFO: %d bytes\r\n", length);

    /* Start transmission */

    UART_Printf("Starting transmission...\r\n");

    CC1101_Strobe(CC1101_STX);

    startTime = HAL_GetTick();

    while ((HAL_GetTick() - startTime) < 1500)
    {
        uint8_t marcState;
        uint8_t txBytes;
        GPIO_PinState gdo0;

        marcState = CC1101_ReadStatus(CC1101_MARCSTATE);
        txBytes = CC1101_ReadStatus(CC1101_TXBYTES);

        gdo0 = HAL_GPIO_ReadPin(CC1101_GDO0_PORT,
                                CC1101_GDO0_PIN);

        /* Print diagnostics every 100 ms */

        if ((HAL_GetTick() - lastPrint) >= 100)
        {
            UART_Printf("MARCSTATE=%02X TXBYTES=%02X GDO0=%d\r\n",
                        marcState,
                        txBytes,
                        (int)gdo0);

            lastPrint = HAL_GetTick();
        }

        /* Successful packet transmission */

        if (marcState == 0x01 && (txBytes & 0x7F) == 0)
        {
            UART_Printf("TX COMPLETE!\r\n");
            return;
        }

        HAL_Delay(10);
    }

    /* Capture state before flushing FIFO */

    UART_Printf("\r\nTX TIMEOUT.\r\n");

    UART_Printf("Final MARCSTATE = %02X\r\n",
                CC1101_ReadStatus(CC1101_MARCSTATE));

    UART_Printf("Final TXBYTES = %02X\r\n",
                CC1101_ReadStatus(CC1101_TXBYTES));

    UART_Printf("Final GDO0 = %d\r\n",
                (int)HAL_GPIO_ReadPin(CC1101_GDO0_PORT,
                                      CC1101_GDO0_PIN));

    /* Return radio to a safe idle state */

    CC1101_Strobe(CC1101_SIDLE);
    CC1101_Strobe(CC1101_SFTX);
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();

    HAL_Delay(500);

    UART_Printf("\r\n");
    UART_Printf("STM32F4 + CC1101 TEST\r\n");
    UART_Printf("=====================\r\n");

    CC1101_Configure();

    uint8_t partnum = CC1101_ReadStatus(0x30);
    uint8_t version = CC1101_ReadStatus(0x31);

    UART_Printf("CC1101 PARTNUM = %02X\r\n", partnum);
    UART_Printf("CC1101 VERSION = %02X\r\n", version);

    while (1)
    {
        CC1101_TransmitHelloWorld();

        HAL_Delay(3000);
    }
}

/* =========================================================
   ERROR HANDLER
   ========================================================= */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
