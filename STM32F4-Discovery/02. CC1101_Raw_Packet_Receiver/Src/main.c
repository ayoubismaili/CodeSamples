
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
TIM_HandleTypeDef htim2;

/* ==========================================
   CC1101 CONNECTIONS
   ========================================== */

#define CC1101_CS_PORT       GPIOA
#define CC1101_CS_PIN        GPIO_PIN_4

#define CC1101_MISO_PORT     GPIOA
#define CC1101_MISO_PIN      GPIO_PIN_6

#define CC1101_GDO0_PORT     GPIOA
#define CC1101_GDO0_PIN      GPIO_PIN_0
#define CC1101_GDO0_IRQn     EXTI0_IRQn

#define CC1101_SPI_TIMEOUT   100
#define CC1101_READY_TIMEOUT 100

/* ==========================================
   CC1101 REGISTERS
   ========================================== */

#define CC1101_IOCFG0       0x02
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_FSCTRL1      0x0B

#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F

#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15

#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19

#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D

#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22

#define CC1101_SRES         0x30
#define CC1101_SRX          0x34
#define CC1101_SIDLE        0x36

/* ==========================================
   BURST DETECTION CONFIGURATION
   ========================================== */

#define BURST_IDLE_TIMEOUT_US    5000U

/* ==========================================
   BURST DETECTION VARIABLES
   ========================================== */

/*
 * One pulse is counted for every completed HIGH pulse.
 */

/* ==========================================
   BURST CAPTURE VARIABLES
   ========================================== */

#define BURST_IDLE_TIMEOUT_US  5000U

#define MAX_BURST_INTERVALS    512U

volatile uint32_t burst_pulse_count = 0;
volatile uint32_t last_edge_time = 0;

volatile bool burst_active = false;

/* Detailed interval capture */

volatile uint32_t burst_duration[MAX_BURST_INTERVALS];
volatile uint8_t burst_state[MAX_BURST_INTERVALS];

volatile uint16_t burst_interval_count = 0;

volatile bool burst_overflow = false;

/* Previous signal state */

volatile uint8_t previous_state = 0;
volatile bool edge_initialized = false;

/* ==========================================
   FUNCTION PROTOTYPES
   ========================================== */

void Error_Handler(void);

static void SPI1_Init(void);
static void UART2_Init(void);
static void TIM2_Init(void);
static void GDO0_Interrupt_Init(void);

static void UART_Print(const char *msg);

static bool CC1101_WaitReady(void);
static void CC1101_Strobe(uint8_t strobe);
static bool CC1101_WriteRegister(uint8_t addr,
                                uint8_t value);

static void CC1101_Init433MHz_OOK_Async(void);

/* ==========================================
   SPI1 INITIALIZATION
   ========================================== */

static void SPI1_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* SPI1 SCK, MISO, MOSI */

    gpio.Pin = GPIO_PIN_5 |
               GPIO_PIN_6 |
               GPIO_PIN_7;

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;

    HAL_GPIO_Init(GPIOA, &gpio);

    /* CC1101 Chip Select */

    gpio.Pin = CC1101_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(CC1101_CS_PORT, &gpio);

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_SET
    );

    hspi1.Instance = SPI1;

    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

    hspi1.Init.NSS = SPI_NSS_SOFT;

    hspi1.Init.BaudRatePrescaler =
        SPI_BAUDRATEPRESCALER_64;

    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================
   USART2 INITIALIZATION
   ========================================== */

static void UART2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    /* USART2 TX = PA2
       USART2 RX = PA3 */

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
   TIM2 INITIALIZATION
   1 MHz TIMER
   ========================================== */

static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    /*
     * STM32F4 APB1 timer clock:
     *
     * If APB1 prescaler = 1:
     * TIM2CLK = PCLK1
     *
     * Otherwise:
     * TIM2CLK = 2 * PCLK1
     */

    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();

    uint32_t apb1_div =
        (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;

    uint32_t timer_clock = pclk1;

    if (apb1_div >= 4U)
    {
        timer_clock = pclk1 * 2U;
    }

    if ((timer_clock % 1000000U) != 0U)
    {
        Error_Handler();
    }

    htim2.Instance = TIM2;

    htim2.Init.Prescaler =
        (timer_clock / 1000000U) - 1U;

    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

    htim2.Init.Period = 0xFFFFFFFFU;

    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================
   GDO0 INTERRUPT INITIALIZATION
   PA0 - RISING AND FALLING EDGES
   ========================================== */

static void GDO0_Interrupt_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = CC1101_GDO0_PIN;

    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;

    gpio.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(CC1101_GDO0_PORT, &gpio);

    __HAL_GPIO_EXTI_CLEAR_IT(CC1101_GDO0_PIN);

    HAL_NVIC_SetPriority(CC1101_GDO0_IRQn, 2, 0);

    HAL_NVIC_EnableIRQ(CC1101_GDO0_IRQn);
}

/* ==========================================
   UART PRINT
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
   CC1101 SPI HELPERS
   ========================================== */

static bool CC1101_WaitReady(void)
{
    uint32_t start = HAL_GetTick();

    while (HAL_GPIO_ReadPin(
               CC1101_MISO_PORT,
               CC1101_MISO_PIN) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - start) >=
            CC1101_READY_TIMEOUT)
        {
            return false;
        }
    }

    return true;
}

static void CC1101_Strobe(uint8_t strobe)
{
    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_RESET
    );

    if (!CC1101_WaitReady())
    {
        HAL_GPIO_WritePin(
            CC1101_CS_PORT,
            CC1101_CS_PIN,
            GPIO_PIN_SET
        );

        Error_Handler();
    }

    if (HAL_SPI_Transmit(
            &hspi1,
            &strobe,
            1,
            CC1101_SPI_TIMEOUT) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_SET
    );
}

static bool CC1101_WriteRegister(
    uint8_t addr,
    uint8_t value)
{
    uint8_t data[2] = {addr, value};

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_RESET
    );

    if (!CC1101_WaitReady())
    {
        HAL_GPIO_WritePin(
            CC1101_CS_PORT,
            CC1101_CS_PIN,
            GPIO_PIN_SET
        );

        return false;
    }

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(
            &hspi1,
            data,
            2,
            CC1101_SPI_TIMEOUT
        );

    HAL_GPIO_WritePin(
        CC1101_CS_PORT,
        CC1101_CS_PIN,
        GPIO_PIN_SET
    );

    return (status == HAL_OK);
}

/* ==========================================
   CC1101 ASYNCHRONOUS OOK CONFIGURATION
   ========================================== */

static void CC1101_Init433MHz_OOK_Async(void)
{
    CC1101_Strobe(CC1101_SRES);

    HAL_Delay(10);

    /* Asynchronous serial data output */

    CC1101_WriteRegister(CC1101_IOCFG0, 0x0D);

    CC1101_WriteRegister(CC1101_PKTCTRL1, 0x00);

    CC1101_WriteRegister(CC1101_PKTCTRL0, 0x32);

    CC1101_WriteRegister(CC1101_FSCTRL1, 0x06);

    /* 433.92 MHz */

    CC1101_WriteRegister(CC1101_FREQ2, 0x10);
    CC1101_WriteRegister(CC1101_FREQ1, 0xA7);
    CC1101_WriteRegister(CC1101_FREQ0, 0xA0);

    /* OOK configuration */

    CC1101_WriteRegister(CC1101_MDMCFG4, 0xF7);
    CC1101_WriteRegister(CC1101_MDMCFG3, 0x83);
    CC1101_WriteRegister(CC1101_MDMCFG2, 0x30);
    CC1101_WriteRegister(CC1101_MDMCFG1, 0x22);
    CC1101_WriteRegister(CC1101_MDMCFG0, 0xF8);
    CC1101_WriteRegister(CC1101_DEVIATN, 0x15);

    CC1101_WriteRegister(CC1101_MCSM0, 0x18);

    CC1101_WriteRegister(CC1101_FOCCFG, 0x14);

    /* AGC settings */

    CC1101_WriteRegister(CC1101_AGCCTRL2, 0x03);
    CC1101_WriteRegister(CC1101_AGCCTRL1, 0x40);
    CC1101_WriteRegister(CC1101_AGCCTRL0, 0x91);

    CC1101_WriteRegister(CC1101_FREND1, 0xB6);
    CC1101_WriteRegister(CC1101_FREND0, 0x11);

    /* Enter receive mode */

    CC1101_Strobe(CC1101_SRX);

    UART_Print(
        "CC1101 initialized in asynchronous OOK mode.\r\n"
    );
}

/* ==========================================
   EXTI0 INTERRUPT HANDLER
   ========================================== */

void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(CC1101_GDO0_PIN) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT(CC1101_GDO0_PIN);

        uint32_t current_time =
            __HAL_TIM_GET_COUNTER(&htim2);

        uint8_t current_state =
            (HAL_GPIO_ReadPin(
                CC1101_GDO0_PORT,
                CC1101_GDO0_PIN
            ) == GPIO_PIN_SET) ? 1U : 0U;

        if (!edge_initialized)
        {
            /*
             * First edge:
             * Establish timing reference.
             */

            last_edge_time = current_time;

            previous_state = (current_state == 1U) ? 0U : 1U;

            edge_initialized = true;

            burst_active = true;
        }
        else
        {
            /*
             * Calculate duration of previous state.
             */

            uint32_t duration_us =
                current_time - last_edge_time;

            /*
             * Store interval duration and state.
             */

            if (burst_interval_count < MAX_BURST_INTERVALS)
            {
                burst_duration[burst_interval_count] =
                    duration_us;

                burst_state[burst_interval_count] =
                    previous_state;

                burst_interval_count++;
            }
            else
            {
                burst_overflow = true;
            }

            /*
             * Count completed HIGH pulses.
             *
             * HIGH -> LOW
             */

            if (current_state == 0U)
            {
                burst_pulse_count++;
            }

            /*
             * Update timing reference.
             */

            last_edge_time = current_time;

            previous_state = current_state;

            burst_active = true;
        }
    }
}

/* ==========================================
   MAIN PROGRAM
   ========================================== */

int main(void)
{
    HAL_Init();

    SPI1_Init();
    UART2_Init();
    TIM2_Init();

    UART_Print("\r\n");

    UART_Print("========================================\r\n");
    UART_Print(" STM32F4 CC1101 BURST PULSE ANALYZER\r\n");
    UART_Print("========================================\r\n");

    /* Initialize CC1101 */

    CC1101_Init433MHz_OOK_Async();

    /*
     * Initialize timing reference.
     */

    last_edge_time =
        __HAL_TIM_GET_COUNTER(&htim2);

    /* Enable GDO0 interrupt */

    GDO0_Interrupt_Init();

    UART_Print("Waiting for RF bursts...\r\n\r\n");

    char msg[128];

    uint32_t completed_bursts = 0;

    /*
     * Local buffer for printing.
     */

    uint32_t local_duration[MAX_BURST_INTERVALS];

    uint8_t local_state[MAX_BURST_INTERVALS];

    while (1)
    {
        HAL_NVIC_DisableIRQ(CC1101_GDO0_IRQn);

        uint32_t current_time =
            __HAL_TIM_GET_COUNTER(&htim2);

        uint32_t idle_time =
            current_time - last_edge_time;

        bool burst_finished = false;

        uint32_t captured_pulses = 0;

        uint16_t captured_intervals = 0;

        bool capture_overflow = false;

        /*
         * Detect burst completion.
         */

        if (burst_active &&
            idle_time >= BURST_IDLE_TIMEOUT_US)
        {
            captured_pulses = burst_pulse_count;

            captured_intervals = burst_interval_count;

            capture_overflow = burst_overflow;

            /*
             * Copy captured intervals.
             */

            for (uint16_t i = 0;
                 i < captured_intervals;
                 i++)
            {
                local_duration[i] =
                    burst_duration[i];

                local_state[i] =
                    burst_state[i];
            }

            /*
             * Reset capture variables.
             */

            burst_pulse_count = 0;

            burst_interval_count = 0;

            burst_overflow = false;

            burst_active = false;

            edge_initialized = false;

            burst_finished = true;
        }

        HAL_NVIC_EnableIRQ(CC1101_GDO0_IRQn);

        /*
         * Only process bursts containing
         * 95 to 120 completed HIGH pulses.
         */

        if (burst_finished &&
            captured_pulses >= 95 &&
            captured_pulses <= 120)
        {
            completed_bursts++;

            snprintf(
                msg,
                sizeof(msg),
                "\r\n========================================\r\n"
                "[BURST %lu]\r\n"
                "Total HIGH pulses: %lu\r\n"
                "Total intervals: %u\r\n",
                (unsigned long)completed_bursts,
                (unsigned long)captured_pulses,
                captured_intervals
            );

            UART_Print(msg);

            UART_Print("----------------------------------------\r\n");

            /*
             * Print every captured interval.
             */

            for (uint16_t i = 0;
                 i < captured_intervals;
                 i++)
            {
                snprintf(
                    msg,
                    sizeof(msg),
                    "[%03u] STATE: %s | DURATION: %lu us\r\n",
                    i + 1,
                    (local_state[i] == 1U) ? "HIGH" : "LOW",
                    (unsigned long)local_duration[i]
                );

                UART_Print(msg);
            }

            UART_Print("----------------------------------------\r\n");

            /*
             * Report missing intervals if
             * the capture buffer overflowed.
             */

            if (capture_overflow)
            {
                UART_Print(
                    "WARNING: Capture buffer overflow!\r\n"
                );
            }

            UART_Print("BURST END\r\n");

            UART_Print("========================================\r\n\r\n");
        }

        HAL_Delay(1);
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
