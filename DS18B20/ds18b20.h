/**
  ******************************************************************************
  * @file    ds18b20.h
  * @brief   DS18B20 1-Wire temperature sensor library (UART half-duplex)
  *
  * Communicates with DS18B20 using UART in half-duplex mode:
  *   - 9600  baud  → reset / presence detect
  *   - 115200 baud → read / write bits
  *
  ******************************************************************************
  */

#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>


/* Exported constants --------------------------------------------------------*/

/** DS18B20 ROM commands */
#define DS18B20_CMD_READ_ROM      0x33
#define DS18B20_CMD_MATCH_ROM     0x55
#define DS18B20_CMD_SKIP_ROM      0xCC
#define DS18B20_CMD_SEARCH_ROM    0xF0
#define DS18B20_CMD_ALARM_SEARCH  0xEC

/** DS18B20 function commands */
#define DS18B20_CMD_WRITE_SCRATCHPAD  0x4E
#define DS18B20_CMD_READ_SCRATCHPAD   0xBE
#define DS18B20_CMD_COPY_SCRATCHPAD   0x48
#define DS18B20_CMD_CONVERT_T         0x44
#define DS18B20_CMD_RECALL_E2         0xB8
#define DS18B20_CMD_READ_POWER        0xB4

/** Resolution configuration register bits */
#define DS18B20_RES_9BIT   0x1F   /* 93.75ms  conversion */
#define DS18B20_RES_10BIT  0x3F   /* 187.5ms  conversion */
#define DS18B20_RES_11BIT  0x5F   /* 375ms    conversion */
#define DS18B20_RES_12BIT  0x7F   /* 750ms    conversion (default) */

/** Conversion timeout per resolution (ms) */
#define DS18B20_CONV_TIME_9BIT   94
#define DS18B20_CONV_TIME_10BIT  188
#define DS18B20_CONV_TIME_11BIT  375
#define DS18B20_CONV_TIME_12BIT  750


typedef enum{
    DS18B20_OK = 0,
    DS18B20_ERR_NO_DEVICE = -1,
    DS18B20_ERR_TIMEOUT = -2,
    DS18B20_ERR_CRC = -3,
    DS18B20_ERR_DMA = -4,
    DS18B20_ERR_RANGE = -5
} DS18B20_Status_t;



/* Exported types ------------------------------------------------------------*/

/**
 * @brief  DS18B20 handle structure.
 *         One instance is enough for any number of sensors on the same wire.
 */
typedef struct {
    UART_HandleTypeDef *huart;          /* Pointer to HAL UART handle (half-duplex) */
    uint8_t             resolution;     /* Resolution register value (DS18B20_RES_*) */
    uint8_t             rxData[8];      /* Internal DMA receive buffer              */
    volatile int8_t     rxDone;         /* DMA RX completion flag (set in callback) */
} DS18B20_HandleTypeDef;



/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief  Initialize the DS18B20 handle with a UART peripheral and resolution.
 * @param  hds    Pointer to DS18B20 handle
 * @param  huart  Pointer to initialized HAL UART handle (half-duplex mode)
 * @param  res    Resolution: DS18B20_RES_9BIT … DS18B20_RES_12BIT
 */
void DS18B20_Init(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart, uint8_t res);



/**
 * @brief  Issue a reset pulse and detect sensor presence.
 * @param  hds  Pointer to DS18B20 handle
 * @retval DS18B20_OK            — sensor detected
 *         DS18B20_ERR_NO_DEVICE — no presence pulse received
 *         DS18B20_ERR_TIMEOUT   — UART RX timed out
 */
DS18B20_Status_t DS18B20_Reset(DS18B20_HandleTypeDef *hds);



/**
 * @brief  Write one byte to the 1-Wire bus.
 * @param  hds   Pointer to DS18B20 handle
 * @param  data  Byte to write
 */
void DS18B20_WriteByte(DS18B20_HandleTypeDef *hds, uint8_t data);



/**
 * @brief  Read one byte from the 1-Wire bus (uses DMA).
 * @param  hds  Pointer to DS18B20 handle
 * @retval Byte read from bus, or 0 on DMA error
 */
uint8_t DS18B20_ReadByte(DS18B20_HandleTypeDef *hds);



/**
 * @brief  Trigger a temperature conversion (non-blocking — caller must wait).
 * @param  hds  Pointer to DS18B20 handle
 * @retval DS18B20_OK or error code
 */
DS18B20_Status_t DS18B20_StartConversion(DS18B20_HandleTypeDef *hds);



/**
 * @brief  Read temperature after conversion is complete.
 *         Reads the full scratchpad and validates CRC.
 * @param  hds          Pointer to DS18B20 handle
 * @param  temperature  Output: temperature in °C
 * @retval DS18B20_OK, DS18B20_ERR_NO_DEVICE, DS18B20_ERR_CRC, DS18B20_ERR_RANGE
 */
DS18B20_Status_t DS18B20_ReadTemperature(DS18B20_HandleTypeDef *hds, float *temperature);



/**
 * @brief  Convenience: trigger conversion, wait, then read temperature.
 * @param  hds          Pointer to DS18B20 handle
 * @param  temperature  Output: temperature in °C
 * @retval DS18B20_OK or error code
 */
DS18B20_Status_t DS18B20_GetTemperature(DS18B20_HandleTypeDef *hds, float *temperature);



/**
 * @brief  Set sensor resolution (writes to scratchpad + copies to EEPROM).
 * @param  hds  Pointer to DS18B20 handle
 * @param  res  DS18B20_RES_9BIT … DS18B20_RES_12BIT
 * @retval DS18B20_OK or error code
 */
DS18B20_Status_t DS18B20_SetResolution(DS18B20_HandleTypeDef *hds, uint8_t res);


/**
 * @brief  Checks if the sensor is parasitic powered or on external VCC.
 * @param  hds       Pointer to DS18B20 handle
 * @param  parasitic Output: 0 = parasitic, 1 = external VCC
 * @retval DS18B20_OK or error code
 */
DS18B20_Status_t DS18B20_ReadPowerSupply(DS18B20_HandleTypeDef *hds, uint8_t *parasitic);

/**
 * @brief  Reads the unique 64-bit ROM code of the sensor on the bus.
 * @note   Only use when exactly ONE sensor is connected. If multiple sensors
 *         are present, responses will collide and data will be corrupted.
 *         Connect sensors one by one, note each ROM code, then hardcode them.
 * @param  hds  Pointer to DS18B20 handle
 * @param  rom  Output: 8-byte array to store the ROM code (index 0 = family code 0x28)
 * @retval DS18B20_OK, DS18B20_ERR_NO_DEVICE, DS18B20_ERR_CRC
 */
DS18B20_Status_t DS18B20_ReadROM(DS18B20_HandleTypeDef *hds, uint8_t *rom);


/**
 * @brief  Addresses one specific sensor by its ROM code, ignoring all others on the bus.
 * @note   Must be followed immediately by a function command (e.g. Convert_T, Read Scratchpad).
 *         Any other command after this will target the matched sensor only.
 * @param  hds  Pointer to DS18B20 handle
 * @param  rom  8-byte ROM code of the sensor to address
 * @retval DS18B20_OK, DS18B20_ERR_NO_DEVICE, DS18B20_ERR_TIMEOUT
 */
DS18B20_Status_t DS18B20_MatchROM(DS18B20_HandleTypeDef *hds, uint8_t *rom);


/**
 * @brief  Triggers conversion and reads temperature from one specific sensor by ROM code.
 * @note   Only the targeted sensor converts. Other sensors on the bus are unaffected.
 *         For reading multiple sensors efficiently, use DS18B20_StartConversion() once
 *         (Skip ROM) then call this for each sensor — avoids repeated conversion delays.
 * @param  hds          Pointer to DS18B20 handle
 * @param  rom          8-byte ROM code of the target sensor
 * @param  temperature  Output: temperature in °C
 * @retval DS18B20_OK, DS18B20_ERR_NO_DEVICE, DS18B20_ERR_CRC, DS18B20_ERR_RANGE
 */
DS18B20_Status_t DS18B20_GetTemperatureByROM(DS18B20_HandleTypeDef *hds, uint8_t *rom, float *temperature);


/**
 * @brief  Must be called from HAL_UART_RxCpltCallback for the correct UART.
 *         Signals DMA RX completion to the library.
 * @param  hds    Pointer to DS18B20 handle
 * @param  huart  UART handle passed to the HAL callback
 */
void DS18B20_RxCpltCallback(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart);



/**
 * @brief  Must be called from HAL_UART_ErrorCallback for the correct UART.
 * @param  hds    Pointer to DS18B20 handle
 * @param  huart  UART handle passed to the HAL callback
 */
void DS18B20_ErrorCallback(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart);




#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */