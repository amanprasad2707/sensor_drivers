/**
  ******************************************************************************
  * @file    1_single_sensor.c
  * @brief   Read temperature from a single DS18B20 every 2 seconds.
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

extern UART_HandleTypeDef huart1;   /* DS18B20 — half-duplex, DMA */
extern UART_HandleTypeDef huart2;   /* Debug print                */

DS18B20_HandleTypeDef hds18b20;

static void debug_print(const char *msg){
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void Single_Sensor_Run(void){
    char  msg[48];
    float temperature = 0.0f;

    DS18B20_Init(&hds18b20, &huart1);

    while (1){
        DS18B20_Status_t ret = DS18B20_GetTemperature(&hds18b20, &temperature);

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