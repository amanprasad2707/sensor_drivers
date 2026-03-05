/**
  ******************************************************************************
  * @file    7_get_temperature_by_rom.c
  * @brief   Use DS18B20_GetTemperatureByROM() — convenience function that
  *          handles the full sequence for one specific sensor:
  *          MatchROM → Convert → wait → MatchROM → Read
  *
  * Difference from 6_multi_hardcoded.c:
  *   6_multi_hardcoded — StartConversion (all sensors at once) → read each
  *                        efficient for multiple sensors, one shared wait
  *
  *   This file — GetTemperatureByROM converts only the targeted sensor
  *               simpler code, but each sensor waits separately
  *               use when you only need one specific sensor
  *
  * Wiring
  * ──────
  *   UART TX ──┬── 4.7kΩ ── 3.3V
  *             ├── DS18B20 #1 DATA
  *             └── DS18B20 #2 DATA
  *                 All VCC → 3.3V
  *                 All GND → GND
  ******************************************************************************
  */

#include "main.h"
#include "ds18b20.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

DS18B20_HandleTypeDef hds18b20;

/* ── Paste your ROM codes here ───────────────────────────────────────────── */
static uint8_t sensor1_rom[8] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t sensor2_rom[8] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static void debug_print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void GetTemperatureByROM_Run(void)
{
    char  msg[48];
    float temp1 = 0.0f;
    float temp2 = 0.0f;

    DS18B20_Init(&hds18b20, &huart1);

    while (1)
    {
        /* Each call does: MatchROM → Convert → wait → MatchROM → Read */
        DS18B20_Status_t ret = DS18B20_GetTemperatureByROM(&hds18b20, sensor1_rom, &temp1);
        if (ret == DS18B20_OK) {
            snprintf(msg, sizeof(msg), "S1: %.2f C\r\n", temp1);
        } else {
            snprintf(msg, sizeof(msg), "S1 err: %d\r\n", ret);
        }
        debug_print(msg);

        ret = DS18B20_GetTemperatureByROM(&hds18b20, sensor2_rom, &temp2);
        if (ret == DS18B20_OK) {
            snprintf(msg, sizeof(msg), "S2: %.2f C\r\n", temp2);
        } else {
            snprintf(msg, sizeof(msg), "S2 err: %d\r\n", ret);
        }
        debug_print(msg);

        HAL_Delay(2000);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    DS18B20_RxCpltCallback(&hds18b20, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    DS18B20_ErrorCallback(&hds18b20, huart);
}