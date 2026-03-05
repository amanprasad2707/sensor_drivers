/**
  ******************************************************************************
  * @file    dht11_example.c
  * @brief   Read temperature and humidity from DHT11 sensor every 2 seconds.
  *
  * STM32CubeMX setup
  * ─────────────────
  *  TIM7 → Internal Clock, Prescaler = (SystemClock / 1000000) - 1 → 1µs tick
  *         Period = 65535, No interrupt needed
  *  GPIO → DHT11 data pin as GPIO_Output initially (library switches direction)
  *
  * Wiring
  * ──────
  *   DHT11 VCC  → 3.3V
  *   DHT11 GND  → GND
  *   DHT11 DATA → any GPIO pin (e.g. PA0)
  *                ┗── 10kΩ pull-up ── 3.3V
  ******************************************************************************
  */

#include "main.h"
#include "dht11.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart2;   /* Debug print */
extern TIM_HandleTypeDef  htim7;    /* 1µs tick timer — required by library */

DHT11_HandleTypeDef hdht11;

static void debug_print(const char *msg){
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 1000);
}

void DHT11_Example_Run(void){
    char    msg[64];
    float   temperature = 0.0f;
    uint8_t humidity    = 0;

    /* Init sensor on PA0 — change port/pin to match your wiring */
    dht11_init(&hdht11, GPIOA, GPIO_PIN_0);

    /* Start the microsecond timer */
    HAL_TIM_Base_Start(&htim7);

    while (1){
        dht11_status_t ret = dht11_read_data(&hdht11);

        if (ret == DHT11_OK) {
            dht11_get_temperature(&hdht11, &temperature);
            dht11_get_humidity(&hdht11, &humidity);

            snprintf(msg, sizeof(msg), "Temp: %.1f C  Humidity: %d%%\r\n",
                     temperature, humidity);
            debug_print(msg);
        } else {
            snprintf(msg, sizeof(msg), "Error: %d\r\n", ret);
            debug_print(msg);
        }

        /* DHT11 minimum sampling period is 2 seconds */
        HAL_Delay(2000);
    }
}