#ifndef __LIS3DSH__H__
#define __LIS3DSH__H__

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stdint.h"
#include "stdbool.h"

/* =============================================================================
   LIS3DSH ACCELEROMETER DRIVER
   Target : STM32F407 Discovery (SPI1, PE3 = CS)
   Interface : SPI 4-wire, Mode 3 (CPOL=0, CPHA=1)
   Author : Aman
   ============================================================================= */

/* -----------------------------------------------------------------------------
   REGISTER MAP
   ----------------------------------------------------------------------------- */

#define LIS3DSH_OUT_T            0x0C   /* Temperature output */
#define LIS3DSH_INFO1            0x0D   /* Device info 1 */
#define LIS3DSH_INFO2            0x0E   /* Device info 2 */
#define LIS3DSH_WHO_AM_I         0x0F   /* Device ID — should return 0x3F */

/* Offset correction registers (8-bit signed, applied to raw output) */
#define LIS3DSH_OFF_X            0x10
#define LIS3DSH_OFF_Y            0x11
#define LIS3DSH_OFF_Z            0x12

/* Constant shift registers */
#define LIS3DSH_CS_X             0x13
#define LIS3DSH_CS_Y             0x14
#define LIS3DSH_CS_Z             0x15

/* Long counter (16-bit, used by state machine timers) */
#define LIS3DSH_LC_L             0x16
#define LIS3DSH_LC_H             0x17

/* Interrupt/status */
#define LIS3DSH_STAT             0x18

/* Peak detection output */
#define LIS3DSH_PEAK1            0x19   /* SM1 peak value */
#define LIS3DSH_PEAK2            0x1A   /* SM2 peak value */

/* Vector filter coefficients */
#define LIS3DSH_VFC_1            0x1B
#define LIS3DSH_VFC_2            0x1C
#define LIS3DSH_VFC_3            0x1D
#define LIS3DSH_VFC_4            0x1E

#define LIS3DSH_THRS3            0x1F   /* Threshold 3 (shared) */

/* ── Control registers ──────────────────────────────────────────────────── */
#define LIS3DSH_CTRL_REG4        0x20   /* ODR, axes enable, BDU */
#define LIS3DSH_CTRL_REG1        0x21   /* SM1 enable + interrupt routing */
#define LIS3DSH_CTRL_REG2        0x22   /* SM2 enable + interrupt routing */
#define LIS3DSH_CTRL_REG3        0x23   /* INT1/INT2 pin config, polarity */
#define LIS3DSH_CTRL_REG5        0x24   /* Full scale, filter BW, self-test */
#define LIS3DSH_CTRL_REG6        0x25   /* FIFO / boot */

#define LIS3DSH_STATUS           0x27   /* Data ready flags */

/* ── Acceleration output registers (16-bit, little-endian) ─────────────── */
#define LIS3DSH_OUT_X_L          0x28
#define LIS3DSH_OUT_X_H          0x29
#define LIS3DSH_OUT_Y_L          0x2A
#define LIS3DSH_OUT_Y_H          0x2B
#define LIS3DSH_OUT_Z_L          0x2C
#define LIS3DSH_OUT_Z_H          0x2D

/* FIFO */
#define LIS3DSH_FIFO_CTRL        0x2E
#define LIS3DSH_FIFO_SRC         0x2F

/* ── State Machine 1 (SM1) registers ───────────────────────────────────── */
/* Program steps: 16 opcodes, written to ST1_1 ... ST1_16 */
#define LIS3DSH_ST1_1            0x40
#define LIS3DSH_ST1_2            0x41
#define LIS3DSH_ST1_3            0x42
#define LIS3DSH_ST1_4            0x43
#define LIS3DSH_ST1_5            0x44
#define LIS3DSH_ST1_6            0x45
#define LIS3DSH_ST1_7            0x46
#define LIS3DSH_ST1_8            0x47
#define LIS3DSH_ST1_9            0x48
#define LIS3DSH_ST1_10           0x49
#define LIS3DSH_ST1_11           0x4A
#define LIS3DSH_ST1_12           0x4B
#define LIS3DSH_ST1_13           0x4C
#define LIS3DSH_ST1_14           0x4D
#define LIS3DSH_ST1_15           0x4E
#define LIS3DSH_ST1_16           0x4F

/* SM1 timers (TIM4/TIM3 = 8-bit, TIM2/TIM1 = 16-bit) */
#define LIS3DSH_TIM4_1           0x50   /* 8-bit timer 4 */
#define LIS3DSH_TIM3_1           0x51   /* 8-bit timer 3 */
#define LIS3DSH_TIM2_1_L         0x52   /* 16-bit timer 2 low byte */
#define LIS3DSH_TIM2_1_H         0x53   /* 16-bit timer 2 high byte */
#define LIS3DSH_TIM1_1_L         0x54   /* 16-bit timer 1 low byte */
#define LIS3DSH_TIM1_1_H         0x55   /* 16-bit timer 1 high byte */

/* SM1 thresholds */
#define LIS3DSH_THRS2_1          0x56   /* Threshold 2 for SM1 */
#define LIS3DSH_THRS1_1          0x57   /* Threshold 1 for SM1 (tap threshold) */

/* SM1 axis masks
   Bit layout: [7]=+Z [6]=-Z [5]=+Y [4]=-Y [3]=+X [2]=-X [1]=P_V [0]=N_V
   MASK1_B : axes monitored when threshold condition is NOT met (waiting)
   MASK1_A : axes monitored when threshold condition IS met (triggered) */
#define LIS3DSH_MASK1_B          0x59
#define LIS3DSH_MASK1_A          0x5A

/* SM1 settings register */
#define LIS3DSH_SETT1            0x5B

/* SM1 program pointer and timer counter (read-only) */
#define LIS3DSH_PR1              0x5C
#define LIS3DSH_TC1_L            0x5D
#define LIS3DSH_TC1_H            0x5E

/* SM1 output/interrupt flag register
   Reading this register also CLEARS the interrupt latch */
#define LIS3DSH_OUTS1            0x5F

/* ── State Machine 2 (SM2) registers ───────────────────────────────────── */
#define LIS3DSH_ST2_1            0x60
#define LIS3DSH_ST2_2            0x61
#define LIS3DSH_ST2_3            0x62
#define LIS3DSH_ST2_4            0x63
#define LIS3DSH_ST2_5            0x64
#define LIS3DSH_ST2_6            0x65
#define LIS3DSH_ST2_7            0x66
#define LIS3DSH_ST2_8            0x67
#define LIS3DSH_ST2_9            0x68
#define LIS3DSH_ST2_10           0x69
#define LIS3DSH_ST2_11           0x6A
#define LIS3DSH_ST2_12           0x6B
#define LIS3DSH_ST2_13           0x6C
#define LIS3DSH_ST2_14           0x6D
#define LIS3DSH_ST2_15           0x6E
#define LIS3DSH_ST2_16           0x6F

#define LIS3DSH_TIM4_2           0x70
#define LIS3DSH_TIM3_2           0x71
#define LIS3DSH_TIM2_2_L         0x72
#define LIS3DSH_TIM2_2_H         0x73
#define LIS3DSH_TIM1_2_L         0x74
#define LIS3DSH_TIM1_2_H         0x75

#define LIS3DSH_THRS2_2          0x76
#define LIS3DSH_THRS1_2          0x77
#define LIS3DSH_DES2             0x78
#define LIS3DSH_MASK2_B          0x79
#define LIS3DSH_MASK2_A          0x7A
#define LIS3DSH_SETT2            0x7B
#define LIS3DSH_PR2              0x7C
#define LIS3DSH_TC2_L            0x7D
#define LIS3DSH_TC2_H            0x7E
#define LIS3DSH_OUTS2            0x7F

/* =============================================================================
   REGISTER BIT DEFINITIONS
   ============================================================================= */

/* ── Output Data Rate (CTRL_REG4 bits [7:4]) ────────────────────────────── */
#define LIS3DSH_DATARATE_POWERDOWN   ((uint8_t)0x00)
#define LIS3DSH_DATARATE_3_125       ((uint8_t)0x10)
#define LIS3DSH_DATARATE_6_25        ((uint8_t)0x20)
#define LIS3DSH_DATARATE_12_5        ((uint8_t)0x30)
#define LIS3DSH_DATARATE_25          ((uint8_t)0x40)
#define LIS3DSH_DATARATE_50          ((uint8_t)0x50)
#define LIS3DSH_DATARATE_100         ((uint8_t)0x60)
#define LIS3DSH_DATARATE_400         ((uint8_t)0x70)
#define LIS3DSH_DATARATE_800         ((uint8_t)0x80)
#define LIS3DSH_DATARATE_1600        ((uint8_t)0x90)

/* ── Axes enable (CTRL_REG4 bits [2:0]) ─────────────────────────────────── */
#define LIS3DSH_X_ENABLE             ((uint8_t)0x01)
#define LIS3DSH_Y_ENABLE             ((uint8_t)0x02)
#define LIS3DSH_Z_ENABLE             ((uint8_t)0x04)
#define LIS3DSH_XYZ_ENABLE           ((uint8_t)0x07)

/* ── Block Data Update (CTRL_REG4 bit 3) ────────────────────────────────── */
#define LIS3DSH_BDU_CONTINUOUS_UPDATE  ((uint8_t)0x00) /* registers update continuously */
#define LIS3DSH_BDU_BLOCK_UPDATE       ((uint8_t)0x08) /* registers hold until both bytes read */

/* ── Anti-aliasing filter bandwidth (CTRL_REG5 bits [7:6]) ──────────────── */
#define LIS3DSH_FILTER_BW_800HZ      ((uint8_t)0x00)
#define LIS3DSH_FILTER_BW_200HZ      ((uint8_t)0x01)
#define LIS3DSH_FILTER_BW_400HZ      ((uint8_t)0x02)
#define LIS3DSH_FILTER_BW_50HZ       ((uint8_t)0x03)

/* ── Full scale (CTRL_REG5 bits [5:3]) ──────────────────────────────────── */
/*    Sensitivity: ±2g=0.06mg/LSB  ±4g=0.12mg/LSB  ±8g=0.24mg/LSB  ±16g=0.48mg/LSB */
#define LIS3DSH_FULLSCALE_2          ((uint8_t)0x00)
#define LIS3DSH_FULLSCALE_4          ((uint8_t)0x01)
#define LIS3DSH_FULLSCALE_8          ((uint8_t)0x03)
#define LIS3DSH_FULLSCALE_16         ((uint8_t)0x04)

/* ── Self test (CTRL_REG5 bits [2:1]) ───────────────────────────────────── */
#define LIS3DSH_SELF_TEST_NORMAL     ((uint8_t)0x00)
#define LIS3DSH_SELF_TEST_NEGATIVE   ((uint8_t)0x01)
#define LIS3DSH_SELF_TEST_POSITIVE   ((uint8_t)0x02)

/* ── SPI wire mode (CTRL_REG5 bit 0) ────────────────────────────────────── */
#define LIS3DSH_SPI_4WIRE            ((uint8_t)0x00)
#define LIS3DSH_SPI_3WIRE            ((uint8_t)0x01)

/* ── State machine selector ─────────────────────────────────────────────── */
#define SM1                          ((uint8_t)0x01)
#define SM2                          ((uint8_t)0x02)

/* ── Interrupt pin selector ─────────────────────────────────────────────── */
#define LIS3DSH_INT1                 1
#define LIS3DSH_INT2                 2

/* ── CTRL_REG1/REG2 bits ────────────────────────────────────────────────── */
#define LIS3DSH_SM1_EN               (1U << 0)  /* Enable SM1 */
#define LIS3DSH_SM2_EN               (1U << 0)  /* Enable SM2 */
/* bit3=0 routes interrupt to INT1, bit3=1 routes to INT2 */

/* ── CTRL_REG3 bit positions ────────────────────────────────────────────── */
#define LIS3DSH_DR_EN_Pos            7   /* Data ready interrupt enable */
#define LIS3DSH_IEA_Pos              6   /* Interrupt polarity (0=active low, 1=active high) */
#define LIS3DSH_IEL_Pos              5   /* Interrupt mode (0=latched, 1=pulsed) */
#define LIS3DSH_INT2_EN_Pos          4   /* INT2 pin output enable */
#define LIS3DSH_INT1_EN_Pos          3   /* INT1 pin output enable */
#define LIS3DSH_VFILT_Pos            2   /* Vector filter enable */
#define LIS3DSH_STRT_Pos             0   /* Software reset */

/* ── SETT1 / SETT2 bits ─────────────────────────────────────────────────── */
#define LIS3DSH_SETT_SITR            (1U << 0) /* Reset SM to step 1 after CONT fires */
#define LIS3DSH_SETT_R_TAM           (1U << 1) /* Reset SM if ANY condition fails */
#define LIS3DSH_SETT_ABS             (1U << 5) /* Compare against deviation from mean
                                                   (removes gravity DC offset — essential
                                                    for hand-held tap detection) */
#define LIS3DSH_SETT_THR3_SA         (1U << 6) /* Use THRS3 for axis comparison */
#define LIS3DSH_SETT_P_DET           (1U << 7) /* Peak detection enable */

/* ── SM Opcodes ─────────────────────────────────────────────────────────── */
#define LIS3DSH_SM_GNTH1             0x05  /* Go Next if axis > THRS1 (tap leading edge) */
#define LIS3DSH_SM_LNTH1             0x02  /* Loop (stay) if axis > THRS1 (tap peak/release) */
#define LIS3DSH_SM_CONT              0x11  /* Fire interrupt + reset SM to step 1 */

/* ── Axis masks for MASK1_A / MASK1_B ──────────────────────────────────── */
/*    Bit layout: [7]=+Z [6]=-Z [5]=+Y [4]=-Y [3]=+X [2]=-X [1]=P_V [0]=N_V
      P/N = positive/negative direction of that axis
      P_V/N_V = vector magnitude (sqrt(X²+Y²+Z²)) above/below threshold      */
#define LIS3DSH_MASK_ALL             0xFC  /* Monitor all 6 axis directions */
#define LIS3DSH_MASK_Z_ONLY          0xC0  /* Monitor ±Z only */
#define LIS3DSH_MASK_Y_ONLY          0x30  /* Monitor ±Y only */
#define LIS3DSH_MASK_X_ONLY          0x0C  /* Monitor ±X only */

/* ── WHO_AM_I expected value ─────────────────────────────────────────────── */
#define LIS3DSH_WHO_AM_I_VAL         0x3F

/* =============================================================================
   STRUCTS
   ============================================================================= */

/* Sensor initialization parameters */
typedef struct {
    SPI_HandleTypeDef *handle;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    uint8_t output_data_rate;   /* LIS3DSH_DATARATE_xxx */
    uint8_t en_axis;            /* LIS3DSH_XYZ_ENABLE or combination */
    uint8_t filter_bandwidth;   /* LIS3DSH_FILTER_BW_xxx */
    uint8_t full_scale;         /* LIS3DSH_FULLSCALE_xxx */
    uint8_t self_test;          /* LIS3DSH_SELF_TEST_xxx */
    uint8_t spi_selection;      /* LIS3DSH_SPI_4WIRE or 3WIRE */
    uint8_t block_data_update;  /* LIS3DSH_BDU_xxx */
} LIS3DSH_InitTypeDef;

/* State machine configuration */
typedef struct {
    uint8_t enable;         /* SM1 or SM2 */
    uint8_t interrupt_pin;  /* LIS3DSH_INT1 or LIS3DSH_INT2 */
    uint8_t threshold;      /* THRS1 value — see calibration for correct value */
    uint8_t mask;           /* Axis mask — LIS3DSH_MASK_xxx */
    uint8_t timer;          /* TIM4 (8-bit, optional) */
    uint8_t timer2;         /* TIM3 (8-bit, optional) */
    uint8_t program[16];    /* SM opcodes: LIS3DSH_SM_xxx, unused steps = 0x00 */
} LIS3DSH_SM_ConfigTypeDef;

/* INT1/INT2 pin configuration */
typedef struct {
    uint8_t drdy_enable;        /* DR_EN: data ready interrupt on INT1 */
    uint8_t interrupt_polarity; /* IEA: 0=active low, 1=active high */
    uint8_t interrupt_mode;     /* IEL: 0=latched, 1=pulsed */
    uint8_t int1_enable;        /* INT1_EN: enable INT1 pin output */
    uint8_t int2_enable;        /* INT2_EN: enable INT2 pin output */
    uint8_t vector_filter;      /* VFILT: vector magnitude filter */
    uint8_t soft_reset;         /* STRT: software reset */
} LIS3DSH_InterruptConfigTypeDef;

/* =============================================================================
   FUNCTION PROTOTYPES
   ============================================================================= */

/* Core init */
void    LIS3DSH_Init(LIS3DSH_InitTypeDef *dev);
uint8_t LIS3DSH_WhoAmI(LIS3DSH_InitTypeDef *dev);

/* Raw axis reads (16-bit signed) */
int16_t LIS3DSH_GetAxisX(LIS3DSH_InitTypeDef *dev);
int16_t LIS3DSH_GetAxisY(LIS3DSH_InitTypeDef *dev);
int16_t LIS3DSH_GetAxisZ(LIS3DSH_InitTypeDef *dev);

/* Offset correction (applied inside sensor, 8-bit resolution) */
void LIS3DSH_OffsetX(LIS3DSH_InitTypeDef *dev, int16_t offset);
void LIS3DSH_OffsetY(LIS3DSH_InitTypeDef *dev, int16_t offset);
void LIS3DSH_OffsetZ(LIS3DSH_InitTypeDef *dev, int16_t offset);

/* State machine */
void    LIS3DSH_SM_Init(LIS3DSH_InitTypeDef *dev, LIS3DSH_SM_ConfigTypeDef *SMConfig);
uint8_t LIS3DSH_IsTapDetected(LIS3DSH_InitTypeDef *dev, uint8_t sm);

/* Interrupt pin config */
void LIS3DSH_InterruptConfig(LIS3DSH_InitTypeDef *dev, LIS3DSH_InterruptConfigTypeDef *cfg);

/* Low-level register access (public so main.c can use for debug) */
uint8_t LIS3DSH_ReadReg(LIS3DSH_InitTypeDef *dev, uint8_t reg);


int8_t LIS3DSH_GetTemperature(LIS3DSH_InitTypeDef *dev);
#endif /* __LIS3DSH__H__ */