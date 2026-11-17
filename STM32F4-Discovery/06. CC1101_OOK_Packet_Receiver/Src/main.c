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
#include <stdint.h>
#include <stdarg.h>

/* =========================================================
   STM32 PERIPHERALS
   ========================================================= */

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* =========================================================
   CC1101 CONNECTIONS
   ========================================================= */

#define CC1101_CS_PORT       GPIOA
#define CC1101_CS_PIN        GPIO_PIN_4

#define CC1101_GDO0_PORT     GPIOB
#define CC1101_GDO0_PIN      GPIO_PIN_3

#define CC1101_SELECT() \
    HAL_GPIO_WritePin(CC1101_CS_PORT, CC1101_CS_PIN, GPIO_PIN_RESET)

#define CC1101_DESELECT() \
    HAL_GPIO_WritePin(CC1101_CS_PORT, CC1101_CS_PIN, GPIO_PIN_SET)

/* =========================================================
   CC1101 COMMANDS
   ========================================================= */

#define CC1101_SRES          0x30
#define CC1101_SRX           0x34
#define CC1101_SIDLE         0x36
#define CC1101_SFRX          0x3A

/* =========================================================
   CC1101 STATUS REGISTERS
   ========================================================= */

#define CC1101_PARTNUM       0x30
#define CC1101_VERSION       0x31
#define CC1101_MARCSTATE     0x35
#define CC1101_RXBYTES       0x3B

/* =========================================================
   PACKET SETTINGS
   ========================================================= */

#define PACKET_LENGTH        12
#define RX_PACKET_LENGTH     14

/* 12 payload bytes + 2 appended receiver status bytes */

/* =========================================================
   DECLARE
   ========================================================= */
void Error_Handler(void);

/* =========================================================
   UART PRINT
   ========================================================= */

static void UART_Printf(const char *format, ...)
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
                          (uint16_t)len,
                          HAL_MAX_DELAY);
    }
}

/* =========================================================
   CLOCK CONFIGURATION
   HSI 16 MHz
   ========================================================= */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

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

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

/* =========================================================
   GPIO CONFIGURATION
   ========================================================= */

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* CS output */

    HAL_GPIO_WritePin(CC1101_CS_PORT,
                      CC1101_CS_PIN,
                      GPIO_PIN_SET);

    GPIO_InitStruct.Pin = CC1101_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(CC1101_CS_PORT, &GPIO_InitStruct);

    /* GDO0 input */

    GPIO_InitStruct.Pin = CC1101_GDO0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(CC1101_GDO0_PORT, &GPIO_InitStruct);
}

/* =========================================================
   SPI1
   PA5 SCK
   PA6 MISO
   PA7 MOSI
   ========================================================= */

static void MX_SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin =
        GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;

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
   USART2
   PA2 TX
   PA3 RX
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
    uint8_t data[2] = {address, value};

    CC1101_SELECT();

    if (CC1101_WaitReady())
        HAL_SPI_Transmit(&hspi1, data, 2, 100);

    CC1101_DESELECT();
}

static uint8_t CC1101_ReadStatus(uint8_t address)
{
    uint8_t tx[2] = {address | 0xC0, 0x00};
    uint8_t rx[2] = {0};

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
        HAL_SPI_Transmit(&hspi1, &command, 1, 100);

    CC1101_DESELECT();
}

static void CC1101_ReadFIFO(uint8_t *data, uint8_t length)
{
    uint8_t tx[RX_PACKET_LENGTH + 1] = {0};
    uint8_t rx[RX_PACKET_LENGTH + 1] = {0};

    /* FIFO burst read command */

    tx[0] = 0xFF;

    CC1101_SELECT();

    if (CC1101_WaitReady())
    {
        HAL_SPI_TransmitReceive(&hspi1,
                                tx,
                                rx,
                                length + 1,
                                100);

        memcpy(data, &rx[1], length);
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
   CC1101 RECEIVER CONFIGURATION
   433.92 MHz
   ASK/OOK
   26 MHz crystal
   Approximately 4.74 kbit/s
   ========================================================= */

static void CC1101_ConfigureReceiver(void)
{
    CC1101_Reset();

    /* GDO0: Sync word received */

    CC1101_WriteReg(0x00, 0x06);
    CC1101_WriteReg(0x01, 0x2E);
    CC1101_WriteReg(0x02, 0x06);

    /* Packet configuration */

    CC1101_WriteReg(0x03, 0x47);

    /* Sync word: D3 91 */

    CC1101_WriteReg(0x04, 0xD3);
    CC1101_WriteReg(0x05, 0x91);

    /* Packet length: 12 bytes */

    CC1101_WriteReg(0x06, 0x0C);

    /* Append RSSI and LQI status */

    CC1101_WriteReg(0x07, 0x04);

    /* Fixed packet length, CRC enabled */

    CC1101_WriteReg(0x08, 0x04);

    CC1101_WriteReg(0x09, 0x00);
    CC1101_WriteReg(0x0A, 0x00);

    /* Frequency: 433.92 MHz */

    CC1101_WriteReg(0x0B, 0x06);
    CC1101_WriteReg(0x0C, 0x00);
    CC1101_WriteReg(0x0D, 0x10);
    CC1101_WriteReg(0x0E, 0xB0);
    CC1101_WriteReg(0x0F, 0x71);

    /* Modem configuration */

    CC1101_WriteReg(0x10, 0xF7);
    CC1101_WriteReg(0x11, 0x7E);

    /* ASK/OOK modulation, 16-bit sync detection */

    CC1101_WriteReg(0x12, 0x32);

    CC1101_WriteReg(0x13, 0x22);

    /* Deviation has no effect in OOK mode */

    CC1101_WriteReg(0x14, 0xF8);
    CC1101_WriteReg(0x15, 0x14);

    /* State machine */

    CC1101_WriteReg(0x16, 0x07);
    CC1101_WriteReg(0x17, 0x00);
    CC1101_WriteReg(0x18, 0x18);

    /* Frequency offset and bit synchronization */

    CC1101_WriteReg(0x19, 0x16);
    CC1101_WriteReg(0x1A, 0x6C);

    /* Initial AGC settings */

    CC1101_WriteReg(0x1B, 0x43);
    CC1101_WriteReg(0x1C, 0x40);
    CC1101_WriteReg(0x1D, 0x91);

    /* Front end */

    CC1101_WriteReg(0x21, 0x56);
    CC1101_WriteReg(0x22, 0x10);

    /* Enter idle state */

    CC1101_Strobe(CC1101_SIDLE);
    HAL_Delay(2);

    UART_Printf("CC1101 OOK receiver configured.\r\n");
}

/* =========================================================
   RECEIVE ONE PACKET
   ========================================================= */

static void CC1101_ReceivePacket(void)
{
    uint8_t packet[RX_PACKET_LENGTH] = {0};

    uint8_t rxBytes;
    uint8_t marcState;

    uint8_t rssi;
    uint8_t lqi;

    char message[PACKET_LENGTH + 1];

    /* Start receiver */

    CC1101_Strobe(CC1101_SRX);

    UART_Printf("Listening for packets...\r\n");

    while (1)
    {
        rxBytes = CC1101_ReadStatus(CC1101_RXBYTES);

        /* FIFO overflow */

        if (rxBytes & 0x80)
        {
            UART_Printf("RX FIFO OVERFLOW. Resetting receiver.\r\n");

            CC1101_Strobe(CC1101_SIDLE);
            CC1101_Strobe(CC1101_SFRX);
            CC1101_Strobe(CC1101_SRX);

            HAL_Delay(10);
            continue;
        }

        /* Complete 12-byte packet plus 2 status bytes */

        if ((rxBytes & 0x7F) >= RX_PACKET_LENGTH)
        {
            marcState = CC1101_ReadStatus(CC1101_MARCSTATE);

            UART_Printf("Packet detected. MARCSTATE=%02X\r\n",
                        marcState);

            CC1101_Strobe(CC1101_SIDLE);

            rxBytes = CC1101_ReadStatus(CC1101_RXBYTES);

            if ((rxBytes & 0x80) ||
                ((rxBytes & 0x7F) < RX_PACKET_LENGTH))
            {
                UART_Printf("Incomplete packet in FIFO.\r\n");

                CC1101_Strobe(CC1101_SFRX);
                CC1101_Strobe(CC1101_SRX);
                continue;
            }

            CC1101_ReadFIFO(packet, RX_PACKET_LENGTH);

            /* First 12 bytes are message */

            memcpy(message, packet, PACKET_LENGTH);
            message[PACKET_LENGTH] = '\0';

            /* Appended receiver status */

            rssi = packet[12];
            lqi = packet[13];

            UART_Printf("\r\n");
            UART_Printf("================================\r\n");
            UART_Printf("PACKET RECEIVED\r\n");
            UART_Printf("================================\r\n");

            UART_Printf("MESSAGE: %s\r\n", message);

            UART_Printf("RSSI BYTE: 0x%02X\r\n", rssi);
            UART_Printf("LQI BYTE:  0x%02X\r\n", lqi);

            if (lqi & 0x80)
            {
                UART_Printf("CRC CHECK: PASSED\r\n");
            }
            else
            {
                UART_Printf("CRC CHECK: FAILED\r\n");
            }

            UART_Printf("GDO0: %d\r\n",
                (int)HAL_GPIO_ReadPin(CC1101_GDO0_PORT,
                                      CC1101_GDO0_PIN));

            UART_Printf("================================\r\n\r\n");

            /* Prepare for next packet */

            CC1101_Strobe(CC1101_SFRX);
            CC1101_Strobe(CC1101_SRX);

            UART_Printf("Listening for next packet...\r\n");
        }

        HAL_Delay(10);
    }
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
    UART_Printf("STM32F4 + CC1101 RECEIVER\r\n");
    UART_Printf("=========================\r\n");

    CC1101_ConfigureReceiver();

    uint8_t partnum = CC1101_ReadStatus(CC1101_PARTNUM);
    uint8_t version = CC1101_ReadStatus(CC1101_VERSION);

    UART_Printf("CC1101 PARTNUM = %02X\r\n", partnum);
    UART_Printf("CC1101 VERSION = %02X\r\n", version);

    if (partnum != 0x00 || version != 0x14)
    {
        UART_Printf("WARNING: Unexpected CC1101 identification.\r\n");
    }

    UART_Printf("\r\nRECEIVER READY\r\n");

    CC1101_ReceivePacket();

    while (1)
    {
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
