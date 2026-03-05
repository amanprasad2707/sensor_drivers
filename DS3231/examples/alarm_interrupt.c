/*
 * Example: DS3231 Alarm1 Interrupt Mode
 *
 * Alarm triggers daily at 10:50:00
 * INT/SQW connected to PA0 (EXTI)
 */

#include "ds3231.h"

extern I2C_HandleTypeDef hi2c1;

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();
    MX_GPIO_Init();   // Make sure EXTI is configured here

    ds3231_init(&hi2c1, 0x68);

    /* ---- Set current time ---- */
    RTC_time_t time = {
        .seconds = 0,
        .minutes = 49,
        .hours = 10,
        .hour_format = HOUR_FORMAT_24
    };

    ds3231_setTime(&time);

    /* ---- Configure Alarm1 for 10:50:00 ---- */
    RTC_time_t alarm = {
        .seconds = 0,
        .minutes = 50,
        .hours = 10,
        .hour_format = HOUR_FORMAT_24
    };

    ds3231_setAlarm1Time(&alarm);
    ds3231_setAlarm1Mode(DS3231_ALM1_MATCH_SEC_MIN_HRS);
    ds3231_setOutputMode(DS3231_ALARM_INTERRUPT);
    ds3231_clearAlarm1Flag();      // Clear old flag
    ds3231_enableAlarm1(DS3231_ENABLED);

    while (1){
        /* Main loop can sleep */
        HAL_Delay(1000);
    }
}



/* interrupt callback */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if (GPIO_Pin == GPIO_PIN_0){   // INT connected to PA0

        if (ds3231_isAlarm1Triggered()){
            ds3231_clearAlarm1Flag();

            /* ---- Alarm Action ---- */
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);  // Example LED
        }
    }
}