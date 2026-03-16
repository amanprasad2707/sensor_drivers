#include "lis3dsh.h"
#include "stm32f4xx_hal.h"

/* =============================================================================
   LIS3DSH ACCELEROMETER DRIVER — Implementation
   ============================================================================= */

/* -----------------------------------------------------------------------------
   PRIVATE: Low-level SPI read/write

   SPI protocol for LIS3DSH:
     Write: bit7 of address = 0, then data byte
     Read : bit7 of address = 1, then dummy byte — sensor returns data

   We always use TransmitReceive (full-duplex) even for writes.
   This keeps the RX FIFO drained so stale bytes never corrupt reads.
   ----------------------------------------------------------------------------- */

static void LIS3DSH_WriteReg(LIS3DSH_InitTypeDef *dev, uint8_t reg, uint8_t data)
{
  uint8_t tx[2] = {reg & 0x7F, data}; /* bit7=0 = write command */
  uint8_t rx[2] = {0};

  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET); /* CS low = select */
  HAL_SPI_TransmitReceive(dev->handle, tx, rx, 2, 100);
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET); /* CS high = deselect */
}

uint8_t LIS3DSH_ReadReg(LIS3DSH_InitTypeDef *dev, uint8_t reg)
{
  uint8_t tx[2] = {reg | 0x80, 0x00}; /* bit7=1 = read command */
  uint8_t rx[2] = {0};

  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(dev->handle, tx, rx, 2, 100);
  HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

  return rx[1]; /* rx[0] = garbage during address phase, rx[1] = actual data */
}

/* =============================================================================
   PUBLIC API
   ============================================================================= */

/* -----------------------------------------------------------------------------
   LIS3DSH_Init
   Configures CTRL_REG4 (ODR + axes + BDU) and CTRL_REG5 (filter + scale + test)
   Call this first, before SM_Init.
   ----------------------------------------------------------------------------- */
void LIS3DSH_Init(LIS3DSH_InitTypeDef *dev)
{
  uint8_t ctrl_reg4 = dev->output_data_rate | dev->en_axis | dev->block_data_update;

  uint8_t ctrl_reg5 = (dev->filter_bandwidth << 6) | (dev->full_scale << 3) | (dev->self_test << 1) | (dev->spi_selection << 0);

  LIS3DSH_WriteReg(dev, LIS3DSH_CTRL_REG4, ctrl_reg4);
  LIS3DSH_WriteReg(dev, LIS3DSH_CTRL_REG5, ctrl_reg5);
}

/* -----------------------------------------------------------------------------
   LIS3DSH_WhoAmI
   Returns 0x3F if sensor is present and SPI is working.
   Always call this after Init to verify communication before proceeding.
   ----------------------------------------------------------------------------- */
uint8_t LIS3DSH_WhoAmI(LIS3DSH_InitTypeDef *dev)
{
  return LIS3DSH_ReadReg(dev, LIS3DSH_WHO_AM_I);
}

/* -----------------------------------------------------------------------------
   Axis reads — returns signed 16-bit raw value
   At ±8g: divide by ~170 to get g value (4096 LSB = 1g)
   At ±2g: divide by ~682 to get g value (16384 LSB = 1g)
   ----------------------------------------------------------------------------- */
int16_t LIS3DSH_GetAxisX(LIS3DSH_InitTypeDef *dev)
{
  int16_t val = (int16_t)(LIS3DSH_ReadReg(dev, LIS3DSH_OUT_X_H) << 8);
  val |= LIS3DSH_ReadReg(dev, LIS3DSH_OUT_X_L);
  return val;
}

int16_t LIS3DSH_GetAxisY(LIS3DSH_InitTypeDef *dev)
{
  int16_t val = (int16_t)(LIS3DSH_ReadReg(dev, LIS3DSH_OUT_Y_H) << 8);
  val |= LIS3DSH_ReadReg(dev, LIS3DSH_OUT_Y_L);
  return val;
}

int16_t LIS3DSH_GetAxisZ(LIS3DSH_InitTypeDef *dev)
{
  int16_t val = (int16_t)(LIS3DSH_ReadReg(dev, LIS3DSH_OUT_Z_H) << 8);
  val |= LIS3DSH_ReadReg(dev, LIS3DSH_OUT_Z_L);
  return val;
}

/* -----------------------------------------------------------------------------
   Offset correction — applied by sensor hardware before output
   Value is 8-bit so only the lower byte is used.
   Use to zero out small static offsets when sensor is flat and still.
   ----------------------------------------------------------------------------- */
void LIS3DSH_OffsetX(LIS3DSH_InitTypeDef *dev, int16_t offset)
{
  LIS3DSH_WriteReg(dev, LIS3DSH_OFF_X, (uint8_t)offset);
}

void LIS3DSH_OffsetY(LIS3DSH_InitTypeDef *dev, int16_t offset)
{
  LIS3DSH_WriteReg(dev, LIS3DSH_OFF_Y, (uint8_t)offset);
}

void LIS3DSH_OffsetZ(LIS3DSH_InitTypeDef *dev, int16_t offset)
{
  LIS3DSH_WriteReg(dev, LIS3DSH_OFF_Z, (uint8_t)offset);
}

/* -----------------------------------------------------------------------------
   LIS3DSH_SM_Init
   Programs the hardware state machine for tap detection.

   SM1 tap program (3 steps):
     Step 1 — GNTH1 : Go to next step when ANY monitored axis exceeds THRS1
                       This catches the rising edge of a tap impulse.
     Step 2 — LNTH1 : Stay at this step while axis is STILL above THRS1
                       This waits for the tap to finish (release phase).
     Step 3 — CONT  : Fire interrupt and reset SM back to step 1.

   SETT1 flags used:
     SITR (bit0) — After CONT fires, SM resets to step 1 automatically.
                   Without this, SM stops after one detection.
     ABS  (bit5) — Compare against deviation from running mean, not absolute value.
                   This removes the gravity DC component so the sensor works
                   in any orientation without false triggers.

   Threshold notes (at ±8g, 1 THRS unit ≈ 131 raw LSB ≈ 32mg):
     Use LIS3DSH_CalibrateThreshold() in main.c to find the right value.
     Typical range: 20–150 depending on hand tremor and desired sensitivity.
   ----------------------------------------------------------------------------- */
void LIS3DSH_SM_Init(LIS3DSH_InitTypeDef *dev, LIS3DSH_SM_ConfigTypeDef *SMConfig)
{
  uint8_t reg;

  if (SMConfig->enable == SM1)
  {
    /* 1. Write all 16 SM1 program steps.
          Unused steps must be 0x00 (NOP) — write them all to be safe. */
    for (int i = 0; i < 16; i++)
      LIS3DSH_WriteReg(dev, LIS3DSH_ST1_1 + i, SMConfig->program[i]);

    /* 2. Set tap threshold (compared against axis deviation from mean) */
    LIS3DSH_WriteReg(dev, LIS3DSH_THRS1_1, SMConfig->threshold);

    /* 3. Set axis masks — which directions trigger the SM
          MASK1_B = axes checked when condition NOT met (idle/waiting state)
          MASK1_A = axes checked when condition IS met (triggered state)
          Set both to the same value for normal tap detection. */
    LIS3DSH_WriteReg(dev, LIS3DSH_MASK1_B, SMConfig->mask);
    LIS3DSH_WriteReg(dev, LIS3DSH_MASK1_A, SMConfig->mask);

    /* 4. Configure SETT1:
          SITR — auto-reset SM after firing (allows repeated detection)
          ABS  — subtract DC gravity before comparing (essential for hand-held) */
    LIS3DSH_WriteReg(dev, LIS3DSH_SETT1, LIS3DSH_SETT_SITR | LIS3DSH_SETT_ABS);

    /* 5. Clear any stale interrupt BEFORE enabling SM
          (avoids an immediate false trigger on first enable) */
    LIS3DSH_ReadReg(dev, LIS3DSH_OUTS1);

    /* 6. Enable SM1 and route its interrupt to the selected pin.
          bit3 in CTRL_REG1: 0 = INT1, 1 = INT2 */
    reg = LIS3DSH_ReadReg(dev, LIS3DSH_CTRL_REG1);
    reg |= LIS3DSH_SM1_EN;

    if (SMConfig->interrupt_pin == LIS3DSH_INT1)
      reg &= ~(1U << 3);
    else
      reg |= (1U << 3);

    LIS3DSH_WriteReg(dev, LIS3DSH_CTRL_REG1, reg);
  }

  else if (SMConfig->enable == SM2)
  {
    /* Mirror of SM1 but using SM2 registers */
    for (int i = 0; i < 16; i++)
      LIS3DSH_WriteReg(dev, LIS3DSH_ST2_1 + i, SMConfig->program[i]);

    LIS3DSH_WriteReg(dev, LIS3DSH_THRS1_2, SMConfig->threshold);
    LIS3DSH_WriteReg(dev, LIS3DSH_MASK2_B, SMConfig->mask);
    LIS3DSH_WriteReg(dev, LIS3DSH_MASK2_A, SMConfig->mask);
    LIS3DSH_WriteReg(dev, LIS3DSH_SETT2, LIS3DSH_SETT_SITR | LIS3DSH_SETT_ABS);

    LIS3DSH_ReadReg(dev, LIS3DSH_OUTS2);

    reg = LIS3DSH_ReadReg(dev, LIS3DSH_CTRL_REG2);
    reg |= LIS3DSH_SM2_EN;

    if (SMConfig->interrupt_pin == LIS3DSH_INT1)
      reg &= ~(1U << 3);
    else
      reg |= (1U << 3);

    LIS3DSH_WriteReg(dev, LIS3DSH_CTRL_REG2, reg);
  }
}

/* -----------------------------------------------------------------------------
   LIS3DSH_IsTapDetected
   Reads OUTS1 or OUTS2 and returns non-zero if the SM fired.

   IMPORTANT: Reading OUTS1/OUTS2 also CLEARS the interrupt latch.
   So calling this function IS the clear — do not call it twice per event
   or you will miss the second tap.

   OUTS bit layout: [7]=+Z [6]=-Z [5]=+Y [4]=-Y [3]=+X [2]=-X [1]=P_V [0]=N_V
   The set bits tell you WHICH axis caused the tap.
   ----------------------------------------------------------------------------- */
uint8_t LIS3DSH_IsTapDetected(LIS3DSH_InitTypeDef *dev, uint8_t sm)
{
  uint8_t outs;

  if (sm == SM1)
    outs = LIS3DSH_ReadReg(dev, LIS3DSH_OUTS1);
  else
    outs = LIS3DSH_ReadReg(dev, LIS3DSH_OUTS2);

  return (outs != 0x00); /* any bit set = SM reached CONT = tap detected */
}

/* -----------------------------------------------------------------------------
  LIS3DSH_InterruptConfig
  Configures CTRL_REG3 — controls INT1/INT2 pin behavior.
  Call this if you want to use the physical interrupt pins with EXTI.
   ----------------------------------------------------------------------------- */
void LIS3DSH_InterruptConfig(LIS3DSH_InitTypeDef *dev, LIS3DSH_InterruptConfigTypeDef *cfg)
{
  uint8_t reg = 0;

  reg |= (cfg->soft_reset << LIS3DSH_STRT_Pos);
  reg |= (cfg->vector_filter << LIS3DSH_VFILT_Pos);
  reg |= (cfg->drdy_enable << LIS3DSH_DR_EN_Pos);
  reg |= (cfg->interrupt_polarity << LIS3DSH_IEA_Pos);
  reg |= (cfg->interrupt_mode << LIS3DSH_IEL_Pos);
  reg |= (cfg->int2_enable << LIS3DSH_INT2_EN_Pos);
  reg |= (cfg->int1_enable << LIS3DSH_INT1_EN_Pos);

  LIS3DSH_WriteReg(dev, LIS3DSH_CTRL_REG3, reg);
}

/* -----------------------------------------------------------------------------
  LIS3DSH_GetTemperature
  Returns raw temperature as signed 8-bit value.

  Note: This is a RELATIVE temperature, not absolute.
  The sensor does not provide a calibrated absolute reading.
  It reflects internal die temperature change, useful for
  compensation purposes, not for reading room temperature.

  Typical output: value increases as sensor heats up.
  ----------------------------------------------------------------------------- */
int8_t LIS3DSH_GetTemperature(LIS3DSH_InitTypeDef *dev)
{
  return (int8_t)LIS3DSH_ReadReg(dev, LIS3DSH_OUT_T);
}