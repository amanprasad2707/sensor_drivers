/**
  ******************************************************************************
  * @file    5_read_multi_temp_hardcoded.c
  * @brief   Read multiple sensors using hardcoded ROM codes.
  *          Use when sensors are fixed in hardware and ROM codes are known.
  *
  * How to get ROM codes:
  *   Run 4_read_rom.c (one sensor at a time) or 5_search_rom.c (all at once).
  *   Copy the printed C[] lines into the arrays below.
  *
  * Efficient pattern:
  *   StartConversion once → all sensors convert simultaneously
  *   Wait once → read each sensor by ROM code individually
  *   Total wait = 1 × conversion time regardless of sensor count
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
/* Add more:
 * static uint8_t sensor3_rom[8] = { 0x28, ... };
 */

static void debug_print(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

static void read_sensor(const char *label, uint8_t *rom)
{
    char  msg[48];
    float temperature = 0.0f;

    DS18B20_Status_t ret = DS18B20_MatchROM(&hds18b20, rom);
    if (ret != DS18B20_OK) {
        snprintf(msg, sizeof(msg), "%s MatchROM err: %d\r\n", label, ret);
        debug_print(msg);
        return;
    }

    ret = DS18B20_ReadTemperature(&hds18b20, &temperature);
    if (ret == DS18B20_OK) {
        snprintf(msg, sizeof(msg), "%s: %.2f C\r\n", label, temperature);
    } else {
        snprintf(msg, sizeof(msg), "%s err: %d\r\n", label, ret);
    }
    debug_print(msg);
}

void Multi_Hardcoded_Run(void)
{
    DS18B20_Init(&hds18b20, &huart1);

    while (1)
    {
        /* Convert all sensors at once */
        DS18B20_Status_t ret = DS18B20_StartConversion(&hds18b20);
        if (ret != DS18B20_OK) {
            HAL_Delay(2000);
            continue;
        }

        HAL_Delay(DS18B20_CONV_TIME_12BIT);

        /* Read each sensor by ROM */
        read_sensor("S1", sensor1_rom);
        read_sensor("S2", sensor2_rom);
        /* Add: read_sensor("S3", sensor3_rom); */

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