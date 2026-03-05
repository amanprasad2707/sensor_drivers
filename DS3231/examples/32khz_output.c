/*
 * Example: DS3231 32.768 kHz Output
 *
 * Demonstrates:
 *  - Enabling the dedicated 32kHz output pin
 *  - Checking oscillator status
 *
 * Hardware:
 *  - Connect DS3231 32kHz pin to oscilloscope or logic analyzer
 *  - This is NOT the INT/SQW pin
 */

#include "ds3231.h"

extern I2C_HandleTypeDef hi2c1;

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();

    /* Initialize DS3231 */
    ds3231_init(&hi2c1, 0x68);

    /* Check if oscillator stopped (power failure detection) */
    if (ds3231_isOscillatorStopped()){
        /* You may want to reconfigure time here */
    }

    /* ---- Enable 32kHz output ---- */
    ds3231_set32kHzOutput(DS3231_ENABLED);

    /* Verify it is enabled */
    if (ds3231_is32kHzOutputEnabled()){
        /* 32kHz output active */
    }

    while (1){
        /* Hardware continuously generates 32.768 kHz */
        HAL_Delay(1000);
    }
}