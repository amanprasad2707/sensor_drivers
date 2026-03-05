/**
  ******************************************************************************
  * @file    2_set_resolution.c
  * @brief   Change sensor resolution then read temperature.
  *          Resolution is saved to sensor EEPROM and survives power off.
  *
  * Options:
  *   DS18B20_RES_9BIT   → 0.5°C    /  94ms
  *   DS18B20_RES_10BIT  → 0.25°C   / 188ms
  *   DS18B20_RES_11BIT  → 0.125°C  / 375ms
  *   DS18B20_RES_12BIT  → 0.0625°C / 750ms  (sensor default)
  *
  * Wiring
  * ──────
  *   UART TX ──┬── 4.7kΩ ── 3.3V
  *             └── DS18B20 DATA
  *                 DS18B20 VCC → 3.3V
  *                 DS18B20 GND → GND
  ******************************************************************************
  */

#include "main.h"
#include "ds18b20.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

DS18B20_HandleTypeDef hds18b20;

static void debug_print(const char *msg){
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void Set_Resolution_Run(void){
    char  msg[48];
    float temperature = 0.0f;

    DS18B20_Init(&hds18b20, &huart1);

    /* Change to 9-bit — fastest conversion, least precise */
    DS18B20_Status_t ret = DS18B20_SetResolution(&hds18b20, DS18B20_RES_9BIT);
    if (ret != DS18B20_OK) {
        snprintf(msg, sizeof(msg), "SetResolution error: %d\r\n", ret);
        debug_print(msg);
        return;
    }

    /*
     * After SetResolution, hds18b20.resolution is updated automatically.
     * GetTemperature will now use the 94ms delay instead of 750ms.
     */

    while (1){
        ret = DS18B20_GetTemperature(&hds18b20, &temperature);

        if (ret == DS18B20_OK) {
            snprintf(msg, sizeof(msg), "Temp: %.2f C\r\n", temperature);
        } else {
            snprintf(msg, sizeof(msg), "Error: %d\r\n", ret);
        }
        debug_print(msg);

        HAL_Delay(2000);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    DS18B20_RxCpltCallback(&hds18b20, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
    DS18B20_ErrorCallback(&hds18b20, huart);
}