/**
  ******************************************************************************
  * @file    6_read_multi_temp_dynamic.c
  * @brief   Read multiple sensors dynamically — no hardcoded ROM codes.
  *          SearchROM discovers all sensors at startup automatically.
  *
  * Use when sensor count is unknown or sensors can change.
  * If you know the ROM codes and sensors are fixed, use 6_multi_hardcoded.c.
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

#define MAX_SENSORS  10U

static void debug_print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void Multi_Dynamic_Run(void)
{
    char    msg[48];
    uint8_t rom_codes[MAX_SENSORS][8];
    uint8_t found = 0;

    DS18B20_Init(&hds18b20, &huart1);

    /* Discover all sensors once at startup */
    DS18B20_Status_t ret = DS18B20_SearchROM(&hds18b20, rom_codes, MAX_SENSORS, &found);

    if (ret != DS18B20_OK || found == 0) {
        snprintf(msg, sizeof(msg), "No sensors found: %d\r\n", ret);
        debug_print(msg);
        return;
    }

    snprintf(msg, sizeof(msg), "Found: %d sensor(s)\r\n", found);
    debug_print(msg);

    while (1)
    {
        /* Convert all at once */
        ret = DS18B20_StartConversion(&hds18b20);
        if (ret != DS18B20_OK) {
            HAL_Delay(2000);
            continue;
        }

        HAL_Delay(DS18B20_CONV_TIME_12BIT);

        /* Read each sensor by its discovered ROM code */
        for (int i = 0; i < found; i++) {
            float temperature = 0.0f;

            ret = DS18B20_MatchROM(&hds18b20, rom_codes[i]);
            if (ret != DS18B20_OK) continue;

            ret = DS18B20_ReadTemperature(&hds18b20, &temperature);
            if (ret == DS18B20_OK) {
                snprintf(msg, sizeof(msg), "[%d] %.2f C\r\n", i + 1, temperature);
            } else {
                snprintf(msg, sizeof(msg), "[%d] err: %d\r\n", i + 1, ret);
            }
            debug_print(msg);
        }

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