/**
  ******************************************************************************
  * @file    3_read_rom.c
  * @brief   Read and print the ROM code of a single DS18B20 sensor.
  *          Use this to get the ROM code to paste into 6_multi_hardcoded.c.
  *
  * ⚠️  Connect only ONE sensor when running this.
  *
  * ROM code structure:
  *   [0]   = 0x28 (DS18B20 family code, always)
  *   [1-6] = unique serial number
  *   [7]   = CRC
  *
  * Wiring
  * ──────
  *   UART TX ──┬── 4.7kΩ ── 3.3V
  *             └── DS18B20 DATA  (one sensor only)
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

void ReadROM_Run(void){
    char    msg[96];
    uint8_t rom[8];

    DS18B20_Init(&hds18b20, &huart1);

    while (1){
        DS18B20_Status_t ret = DS18B20_ReadROM(&hds18b20, rom);

        if (ret == DS18B20_OK) {
            /* Spaced hex — easy to read */
            snprintf(msg, sizeof(msg),
                "ROM : %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                rom[0], rom[1], rom[2], rom[3],
                rom[4], rom[5], rom[6], rom[7]);
            debug_print(msg);

            /* C array — paste directly into 6_multi_hardcoded.c */
            snprintf(msg, sizeof(msg),
                "C[] : {0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X}\r\n",
                rom[0], rom[1], rom[2], rom[3],
                rom[4], rom[5], rom[6], rom[7]);
            debug_print(msg);
        } else {
            snprintf(msg, sizeof(msg), "Error: %d\r\n", ret);
            debug_print(msg);
        }

        HAL_Delay(3000);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    DS18B20_RxCpltCallback(&hds18b20, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
    DS18B20_ErrorCallback(&hds18b20, huart);
}