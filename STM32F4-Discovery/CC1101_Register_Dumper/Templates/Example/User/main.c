
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ==========================================
   PERIPHERAL HANDLES
   ========================================== */

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* ==========================================
   CC1101 CONNECTIONS
   ========================================== */

#define CC1101_CS_PORT       GPIOA
#define CC1101_CS_PIN        GPIO_PIN_4

#define CC1101_MISO_PORT     GPIOA
#define CC1101_MISO_PIN      GPIO_PIN_6

#define CC1101_SPI_TIMEOUT   100
#define CC1101_READY_TIMEOUT 100

/* ==========================================
   CC1101 REGISTER DEFINITIONS
   ========================================== */

/* Configuration registers */

#define CC1101_IOCFG2       0x00
#define CC1101_IOCFG1       0x01
#define CC1101_IOCFG0       0x02
#define CC1101_FIFOTHR      0x03
#define CC1101_SYNC1        0x04
#define CC1101_SYNC0        0x05
#define CC1101_PKTLEN       0x06
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_ADDR         0x09
#define CC1101_CHANNR       0x0A
#define CC1101_FSCTRL1      0x0B
#define CC1101_FSCTRL0      0x0C
#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F
#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15
#define CC1101_MCSM2        0x16
#define CC1101_MCSM1        0x17
#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19
#define CC1101_BSCFG        0x1A
#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D
#define CC1101_WOREVT1      0x1E
#define CC1101_WOREVT0      0x1F
#define CC1101_WORCTRL      0x20
#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22
#define CC1101_FSCAL3       0x23
#define CC1101_FSCAL2       0x24
#define CC1101_FSCAL1       0x25
#define CC1101_FSCAL0       0x26
#define CC1101_RCCTRL1      0x27
#define CC1101_RCCTRL0      0x28
#define CC1101_FSTEST       0x29
#define CC1101_PTEST        0x2A
#define CC1101_AGCTEST      0x2B
#define CC1101_TEST2        0x2C
#define CC1101_TEST1        0x2D
#define CC1101_TEST0        0x2E

/* Status registers */

#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_WORTIME1     0x36
#define CC1101_WORTIME0     0x37
#define CC1101_PKTSTATUS    0x38
#define CC1101_VCO_VC_DAC   0x39
#define CC1101_TXBYTES      0x3A
#define CC1101_RXBYTES      0x3B
#define CC1101_RCCTRL1_S    0x3C
#define CC1101_RCCTRL0_S    0x3D

/* ==========================================
   REGISTER INFORMATION STRUCTURE
   ========================================== */

typedef struct
{
    uint8_t address;
    const char *name;
    bool is_status;

} CC1101_RegInfo;

/* ==========================================
   REGISTER LOOKUP TABLE
   ========================================== */

static const CC1101_RegInfo reg_list[] =
{
    {0x00, "IOCFG2", false},
    {0x01, "IOCFG1", false},
    {0x02, "IOCFG0", false},
    {0x03, "FIFOTHR", false},
    {0x04, "SYNC1", false},
    {0x05, "SYNC0", false},
    {0x06, "PKTLEN", false},
    {0x07, "PKTCTRL1", false},
    {0x08, "PKTCTRL0", false},
    {0x09, "ADDR", false},
    {0x0A, "CHANNR", false},
    {0x0B, "FSCTRL1", false},
    {0x0C, "FSCTRL0", false},
    {0x0D, "FREQ2", false},
    {0x0E, "FREQ1", false},
    {0x0F, "FREQ0", false},
    {0x10, "MDMCFG4", false},
    {0x11, "MDMCFG3", false},
    {0x12, "MDMCFG2", false},
    {0x13, "MDMCFG1", false},
    {0x14, "MDMCFG0", false},
    {0x15, "DEVIATN", false},
    {0x16, "MCSM2", false},
    {0x17, "MCSM1", false},
    {0x18, "MCSM0", false},
    {0x19, "FOCCFG", false},
    {0x1A, "BSCFG", false},
    {0x1B, "AGCCTRL2", false},
    {0x1C, "AGCCTRL1", false},
    {0x1D, "AGCCTRL0", false},
    {0x1E, "WOREVT1", false},
    {0x1F, "WOREVT0", false},
    {0x20, "WORCTRL", false},
    {0x21, "FREND1", false},
    {0x22, "FREND0", false},
    {0x23, "FSCAL3", false},
    {0x24, "FSCAL2", false},
    {0x25, "FSCAL1", false},
    {0x26, "FSCAL0", false},
    {0x27, "RCCTRL1", false},
    {0x28, "RCCTRL0", false},
    {0x29, "FSTEST", false},
    {0x2A, "PTEST", false},
    {0x2B, "AGCTEST", false},
    {0x2C, "TEST2", false},
    {0x2D, "TEST1", false},
    {0x2E, "TEST0", false},

    /* Status registers */

    {0x30, "PARTNUM", true},
    {0x31, "VERSION", true},
    {0x32, "FREQEST", true},
    {0x33, "LQI", true},
    {0x34, "RSSI", true},
    {0x35, "MARCSTATE", true},
    {0x36, "WORTIME1", true},
    {0x37, "WORTIME0", true},
    {0x38, "PKTSTATUS", true},
    {0x39, "VCO_VC_DAC", true},
    {0x3A, "TXBYTES", true},
    {0x3B, "RXBYTES", true},
    {0x3C, "RCCTRL1_STATUS", true},
    {0x3D, "RCCTRL0_STATUS", true}
};

/* ==========================================
   FUNCTION PROTOTYPES
   ========================================== */

void Error_Handler(void);

static void SPI1_Init(void);
static void UART2_Init(void);

static void UART_Print(const char *msg);

static bool CC1101_WaitReady(void);

static bool CC1101_ReadRegister(
    uint8_t addr,
    bool is_status,
    uint8_t *value
);

static bool CC1101_CheckConnection(void);
static void CC1101_RegisterDump(void);

/* ==========================================
   SPI1 INITIALIZATION
   ========================================== */

static void SPI1_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* SPI1:
       PA5 = SCK
       PA6 = MISO
       PA7 = MOSI
    */

    gpio.Pin = GPIO_PIN_5 |
               GPIO_PIN_6 |
               GPIO_PIN_7;

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;

    HAL_GPIO_Init(GPIOA, &gpio);

    /* Chip Select PA4 */

    gpio.Pin = CC1101_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(CC1101_CS_PORT, &gpio);

    /* CC1101 initially deselected */

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_SET
    );

    /* SPI configuration */

    hspi1.Instance = SPI1;

    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

    /* SPI Mode 0 */

    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

    hspi1.Init.NSS = SPI_NSS_SOFT;

    /* Conservative SPI clock */

    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_64;

    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation =
        SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================
   UART2 INITIALIZATION
   ========================================== */

static void UART2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* PA2 = USART2 TX
       PA3 = USART2 RX
    */

    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance = USART2;

    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;

    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================
   UART PRINT FUNCTION
   ========================================== */

static void UART_Print(const char *msg)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)msg,
        strlen(msg),
        HAL_MAX_DELAY
    );
}

/* ==========================================
   CC1101 CHIP_RDYn CHECK
   ========================================== */

/*
   After CSn goes LOW:

   SO/MISO = HIGH : CC1101 not ready
   SO/MISO = LOW  : CC1101 ready

   PA6 is the SPI MISO pin.
*/

static bool CC1101_WaitReady(void)
{
    uint32_t start = HAL_GetTick();

    while (
        HAL_GPIO_ReadPin(
            CC1101_MISO_PORT,
            CC1101_MISO_PIN
        ) == GPIO_PIN_SET
    )
    {
        if ((HAL_GetTick() - start) >=
            CC1101_READY_TIMEOUT)
        {
            return false;
        }
    }

    return true;
}

/* ==========================================
   CC1101 REGISTER READ
   ========================================== */

static bool CC1101_ReadRegister(
    uint8_t addr,
    bool is_status,
    uint8_t *value
)
{
    uint8_t cmd;
    uint8_t status_byte = 0;
    uint8_t dummy = 0x00;

    if (value == NULL)
    {
        return false;
    }

    *value = 0;

    /*
       Configuration register:
       Read = address | 0x80

       Status register:
       Read = address | 0xC0
    */

    cmd = addr | (is_status ? 0xC0 : 0x80);

    /* Select CC1101 */

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_RESET
    );

    /* Wait for CHIP_RDYn */

    if (!CC1101_WaitReady())
    {
        HAL_GPIO_WritePin(
            CC1101_CS_PORT,
            CC1101_CS_PIN,
            GPIO_PIN_SET
        );

        return false;
    }

    /* Send register address */

    if (HAL_SPI_TransmitReceive(
            &hspi1,
            &cmd,
            &status_byte,
            1,
            CC1101_SPI_TIMEOUT
        ) != HAL_OK)
    {
        HAL_GPIO_WritePin(
            CC1101_CS_PORT,
            CC1101_CS_PIN,
            GPIO_PIN_SET
        );

        return false;
    }

    /* Clock out register data */

    if (HAL_SPI_TransmitReceive(
            &hspi1,
            &dummy,
            value,
            1,
            CC1101_SPI_TIMEOUT
        ) != HAL_OK)
    {
        HAL_GPIO_WritePin(
            CC1101_CS_PORT,
            CC1101_CS_PIN,
            GPIO_PIN_SET
        );

        return false;
    }

    /* Deselect CC1101 */

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_SET
    );

    return true;
}

/* ==========================================
   CC1101 CONNECTION TEST
   ========================================== */

static bool CC1101_CheckConnection(void)
{
    uint8_t partnum;
    uint8_t version;

    char msg[120];

    uint8_t previous_partnum = 0;
    uint8_t previous_version = 0;

    UART_Print("\r\n");
    UART_Print("================================\r\n");
    UART_Print("      CC1101 CONNECTION TEST\r\n");
    UART_Print("================================\r\n");

    for (int i = 0; i < 5; i++)
    {
        if (!CC1101_ReadRegister(
                CC1101_PARTNUM,
                true,
                &partnum))
        {
            UART_Print("ERROR: PARTNUM SPI READ FAILED\r\n");
            return false;
        }

        if (!CC1101_ReadRegister(
                CC1101_VERSION,
                true,
                &version))
        {
            UART_Print("ERROR: VERSION SPI READ FAILED\r\n");
            return false;
        }

        snprintf(
            msg,
            sizeof(msg),
            "Test %d: PARTNUM=0x%02X VERSION=0x%02X\r\n",
            i + 1,
            partnum,
            version
        );

        UART_Print(msg);

        if (i == 0)
        {
            previous_partnum = partnum;
            previous_version = version;
        }
        else
        {
            if (partnum != previous_partnum ||
                version != previous_version)
            {
                UART_Print(
                    "ERROR: IDENTIFICATION VALUES CHANGED!\r\n"
                );

                UART_Print(
                    "Check SPI communication and wiring.\r\n"
                );

                return false;
            }
        }

        HAL_Delay(20);
    }

    /* PARTNUM should normally be zero for CC1101 */

    if (partnum != 0x00)
    {
        UART_Print(
            "WARNING: PARTNUM IS NOT 0x00\r\n"
        );

        UART_Print(
            "Device identity is not confirmed.\r\n"
        );

        return false;
    }

    UART_Print("\r\n");
    UART_Print("PARTNUM IS VALID.\r\n");
    UART_Print("IDENTIFICATION READS ARE STABLE.\r\n");

    snprintf(
        msg,
        sizeof(msg),
        "Detected VERSION: 0x%02X\r\n",
        version
    );

    UART_Print(msg);

    return true;
}

/* ==========================================
   COMPLETE REGISTER DUMP
   ========================================== */

static void CC1101_RegisterDump(void)
{
    char msg[150];

    size_t count =
        sizeof(reg_list) / sizeof(reg_list[0]);

    uint8_t value;

    UART_Print("\r\n");
    UART_Print("----------------------------------------\r\n");
    UART_Print("       CC1101 REGISTER DUMP\r\n");
    UART_Print("----------------------------------------\r\n");

    for (size_t i = 0; i < count; i++)
    {
        const CC1101_RegInfo *reg = &reg_list[i];

        if (!CC1101_ReadRegister(
                reg->address,
                reg->is_status,
                &value))
        {
            snprintf(
                msg,
                sizeof(msg),
                "[0x%02X] %-18s : SPI ERROR\r\n",
                reg->address,
                reg->name
            );

            UART_Print(msg);

            continue;
        }

        snprintf(
            msg,
            sizeof(msg),
            "[0x%02X] %-18s : 0x%02X\r\n",
            reg->address,
            reg->name,
            value
        );

        UART_Print(msg);

        HAL_Delay(2);
    }

    UART_Print("----------------------------------------\r\n");
}

/* ==========================================
   MAIN
   ========================================== */

int main(void)
{
    HAL_Init();

    SPI1_Init();
    UART2_Init();

    char msg[100];

    HAL_Delay(100);

    UART_Print("\r\n");
    UART_Print("========================================\r\n");
    UART_Print("       STM32F4 + CC1101 DIAGNOSTIC\r\n");
    UART_Print("========================================\r\n");

    snprintf(
        msg,
        sizeof(msg),
        "SPI Clock Prescaler: 64\r\n"
    );

    UART_Print(msg);

    UART_Print("UART: 115200 8N1\r\n");

    /*
       Repeatedly check the CC1101.

       Do not dump registers if identification
       is unstable or SPI communication fails.
    */

    while (1)
    {
        UART_Print("\r\nChecking CC1101...\r\n");

        if (CC1101_CheckConnection())
        {
            UART_Print("\r\n");
            UART_Print("CC1101 COMMUNICATION OK\r\n");

            CC1101_RegisterDump();
        }
        else
        {
            UART_Print("\r\n");
            UART_Print("CC1101 COMMUNICATION FAILED\r\n");
            UART_Print("Register dump skipped.\r\n");
        }

        UART_Print("\r\nNext test in 3 seconds...\r\n");

        HAL_Delay(3000);
    }
}

/* ==========================================
   ERROR HANDLER
   ========================================== */

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}
