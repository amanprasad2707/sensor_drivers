/**
  ******************************************************************************
  * @file    4_search_rom.c
  * @brief   Discover all DS18B20 sensors on the bus using Search ROM.
  *          Prints ROM codes then does a quick temperature read from each.
  *
  * Run this once to find all ROM codes.
  * Copy the printed C[] lines into 6_multi_hardcoded.c.
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

static void debug_print(const char *msg){
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void SearchROM_Run(void){
    char    msg[96];
    uint8_t rom_codes[MAX_SENSORS][8];
    uint8_t found = 0;

    DS18B20_Init(&hds18b20, &huart1);

    /* Discover all sensors */
    DS18B20_Status_t ret = DS18B20_SearchROM(&hds18b20, rom_codes, MAX_SENSORS, &found);

    if (ret != DS18B20_OK || found == 0) {
        snprintf(msg, sizeof(msg), "SearchROM error: %d  found: %d\r\n", ret, found);
        debug_print(msg);
        return;
    }

    /* Print ROM codes */
    snprintf(msg, sizeof(msg), "Found: %d sensor(s)\r\n", found);
    debug_print(msg);

    for (int i = 0; i < found; i++) {
        snprintf(msg, sizeof(msg),
            "[%d] ROM : %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
            i + 1,
            rom_codes[i][0], rom_codes[i][1],
            rom_codes[i][2], rom_codes[i][3],
            rom_codes[i][4], rom_codes[i][5],
            rom_codes[i][6], rom_codes[i][7]);
        debug_print(msg);

        snprintf(msg, sizeof(msg),
            "[%d] C[] : {0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X}\r\n",
            i + 1,
            rom_codes[i][0], rom_codes[i][1],
            rom_codes[i][2], rom_codes[i][3],
            rom_codes[i][4], rom_codes[i][5],
            rom_codes[i][6], rom_codes[i][7]);
        debug_print(msg);
    }

    /* Quick temperature read from each sensor */
    ret = DS18B20_StartConversion(&hds18b20);
    if (ret != DS18B20_OK) return;

    HAL_Delay(DS18B20_CONV_TIME_12BIT);

    for (int i = 0; i < found; i++) {
        float temperature = 0.0f;

        ret = DS18B20_MatchROM(&hds18b20, rom_codes[i]);
        if (ret != DS18B20_OK) continue;

        ret = DS18B20_ReadTemperature(&hds18b20, &temperature);
        if (ret == DS18B20_OK) {
            snprintf(msg, sizeof(msg), "[%d] Temp: %.2f C\r\n", i + 1, temperature);
        } else {
            snprintf(msg, sizeof(msg), "[%d] Read error: %d\r\n", i + 1, ret);
        }
        debug_print(msg);
    }

    /* Done — reset board to re-scan */
    while (1) { HAL_Delay(1000); }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    DS18B20_RxCpltCallback(&hds18b20, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
    DS18B20_ErrorCallback(&hds18b20, huart);
}