#ifndef _DS3231_H
#define _DS3231_H


#include <stdint.h>
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS3231_TIMEOUT            1000
#define DS3231_I2C_ADDRESS        0x68


/******* Register map ********/
#define DS3231_ADDR_SEC           0X00
#define DS3231_ADDR_MIN           0X01
#define DS3231_ADDR_HRS           0X02
#define DS3231_ADDR_DAY           0X03
#define DS3231_ADDR_DATE          0X04
#define DS3231_ADDR_MONTH         0X05
#define DS3231_ADDR_YEAR          0X06

#define DS3231_ADDR_ALM1_SEC      0X07
#define DS3231_ADDR_ALM1_MIN      0X08
#define DS3231_ADDR_ALM1_HRS      0X09
#define DS3231_ADDR_ALM1_DAYDATE  0x0A

#define DS3231_ADDR_ALM2_MIN	  0X0B
#define DS3231_ADDR_ALM2_HRS	  0X0C
#define DS3231_ADDR_ALM2_DAYDATE  0x0D


#define DS3231_ADDR_CONTROL       0X0E
#define DS3231_ADDR_STATUS        0X0F
#define DS3231_ADDR_AGING         0X10
#define DS3231_ADDR_TEMP_MSB      0X11
#define DS3231_ADDR_TEMP_LSB      0X12


/******* Status register bits ********/
#define DS3231_STATUS_A1F         0
#define DS3231_STATUS_A2F         1
#define DS3231_STATUS_BSY         2
#define DS3231_STATUS_EN32KHZ     3
#define DS3231_STATUS_OSF         7



/******* Control register bits ********/
#define DS3231_CONTROL_A1IE       0
#define DS3231_CONTROL_A2IE       1
#define DS3231_CONTROL_INTCN      2
#define DS3231_CONTROL_RS1        3
#define DS3231_CONTROL_RS2        4
#define DS3231_CONTROL_CONV       5
#define DS3231_CONTROL_BBSQW      6
#define DS3231_CONTROL_EOSC       7


/******* Alarm bit positions ********/
#define DS3231_AXMY			      7
#define DS3231_DYDT			      6




typedef enum{
    SUNDAY = 1,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY
}RTC_day_t;


typedef enum{
    HOUR_FORMAT_24 = 0,
    HOUR_FORMAT_12
} RTC_HourFormat_t;


typedef enum{
    AM = 0,
    PM
} RTC_Meridiem_t;



typedef struct{
	uint8_t date;
	uint8_t month;
	uint16_t year;
	uint8_t day;
}RTC_date_t;


typedef struct{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	RTC_HourFormat_t hour_format;
    RTC_Meridiem_t meridiem;    // not meaningful in 24h
}RTC_time_t;

typedef enum{
	DS3231_DISABLED = 0,
    DS3231_ENABLED
}DS3231_State_t;


typedef enum{
    DS3231_FREQ_1HZ = 0,
    DS3231_FREQ_1024HZ = 1,
    DS3231_FREQ_4096HZ = 2,
    DS3231_FREQ_8192HZ = 3
}DS3231_SquareWaveFreq_t;


typedef enum {
    DS3231_OUTPUT_SQUARE_WAVE = 0,
    DS3231_ALARM_INTERRUPT   = 1
} DS3231_OutputMode_t;

typedef enum{
    DS3231_ALM1_EVERY_SEC              = 0x0F,
    DS3231_ALM1_MATCH_SEC              = 0x0E,
    DS3231_ALM1_MATCH_SEC_MIN          = 0x0C,
    DS3231_ALM1_MATCH_SEC_MIN_HRS      = 0x08,
    DS3231_ALM1_MATCH_SEC_MIN_HRS_DATE = 0x00,
    DS3231_ALM1_MATCH_SEC_MIN_HRS_DAY  = 0x10,
} DS3231_Alarm1Mode_t;

typedef enum{
    DS3231_ALM2_EVERY_MIN          = 0x07,
    DS3231_ALM2_MATCH_MIN          = 0x06,
    DS3231_ALM2_MATCH_MIN_HRS      = 0x04,
    DS3231_ALM2_MATCH_MIN_HRS_DATE = 0x00,
    DS3231_ALM2_MATCH_MIN_HRS_DAY  = 0x08,
} DS3231_Alarm2Mode_t;




/**
 * @brief Initialize DS3231 driver
 * 
 * @param hi2c     Pointer to HAL I2C handle
 * @param address  7-bit I2C address of DS3231 (usually 0x68)
 */
void ds3231_init(I2C_HandleTypeDef *hi2c, uint8_t address);


/**
 * @brief Set current time in DS3231 RTC.
 *
 * This function writes hours, minutes, and seconds into the RTC.
 * It supports both 12-hour and 24-hour formats.
 *
 * @param time Pointer to RTC_time_t structure containing:
 *        - seconds (0–59)
 *        - minutes (0–59)
 *        - hours   (1–12 in 12h mode, 0–23 in 24h mode)
 *        - hour_format (HOUR_FORMAT_12 or HOUR_FORMAT_24)
 *        - meridiem (AM or PM; ignored in 24h mode)
 *
 * @note The structure pointer must not be NULL.
 *
 * @example
 * RTC_time_t t = {
 *     .seconds = 30,
 *     .minutes = 45,
 *     .hours = 10,
 *     .hour_format = HOUR_FORMAT_12,
 *     .meridiem = AM
 * };
 *
 * DS3231_setTime(&t);
 */
void ds3231_setTime(RTC_time_t *time);

/**
 * @brief Read current time from DS3231 RTC.
 *
 * This function reads the current time from the RTC registers
 * and fills the provided RTC_time_t structure.
 *
 * @param time Pointer to RTC_time_t structure where time will be stored.
 *
 * @note The structure pointer must not be NULL.
 *
 * @example
 * RTC_time_t currentTime;
 * DS3231_getTime(&currentTime);
 *
 * Now currentTime.hours, minutes, seconds are updated
 */
void ds3231_getTime(RTC_time_t *time);


/**
 * @brief Set full calendar date in DS3231 RTC.
 *
 * This function writes date, month, year, and weekday
 * into the RTC calendar registers.
 *
 * @param date Pointer to RTC_date_t structure containing:
 *        - date  (1–31)
 *        - month (1–12)
 *        - year  (e.g., 2025)
 *        - day   (SUNDAY–SATURDAY)
 *
 * @note The structure pointer must not be NULL.
 *
 * @example
 * RTC_date_t d = {
 *     .date = 21,
 *     .month = 9,
 *     .year = 2025,
 *     .day = SUNDAY
 * };
 *
 * DS3231_setDate(&d);
 */
void ds3231_setDate(RTC_date_t *date);

/**
 * @brief Read full calendar date from DS3231 RTC.
 *
 * This function reads the current date, month, year,
 * and weekday from the RTC.
 *
 * @param date Pointer to RTC_date_t structure where date will be stored.
 *
 * @note The structure pointer must not be NULL.
 *
 * @example
 * RTC_date_t today;
 * DS3231_getDate(&today);
 *
 * Access:
 * today.date
 * today.month
 * today.year
 * today.day
 */
void ds3231_getDate(RTC_date_t *date);

/**
 * @brief Read internal temperature sensor value (°C)
 */
float ds3231_getTemperature(void);


/**
 * @brief Enable or disable the 32kHz output pin.
 *
 * The DS3231 can output a 32.768 kHz signal on its 32kHz pin.
 * This function enables or disables that output.
 *
 * @param state DS3231_ENABLED  → Enable 32kHz output
 *              DS3231_DISABLED → Disable 32kHz output
 *
 * @example
 * ds3231_set32kHzOutput(DS3231_ENABLED);
 */
void ds3231_set32kHzOutput(DS3231_State_t state);

/**
 * @brief Check whether the 32kHz output is enabled.
 *
 * Reads the EN32kHz bit from the status register.
 *
 * @return 1 if 32kHz output is enabled
 * @return 0 if 32kHz output is disabled
 *
 * @example
 * if(ds3231_is32kHzOutputEnabled())
 * {
 *     // 32kHz signal is active
 * }
 */
uint8_t ds3231_is32kHzOutputEnabled(void);


/**
 * @brief Check if oscillator stop flag (OSF) is set.
 *
 * The OSF bit is set when the oscillator has stopped,
 * typically due to power failure or backup battery removal.
 *
 * @return 1 if oscillator stop flag is set
 * @return 0 if oscillator is running normally
 *
 * @note After detecting this flag, software should reinitialize
 *       the time and then clear the OSF bit.
 *
 * @example
 * if(ds3231_isOscillatorStopped())
 * {
 *     // Reconfigure time and clear OSF flag
 * }
 */
uint8_t ds3231_isOscillatorStopped(void);

/**
 * @brief Enable or disable the main oscillator.
 *
 * Controls the EOSC (Enable Oscillator) bit in the control register.
 *
 * @param state DS3231_ENABLED  → Oscillator runs normally
 *              DS3231_DISABLED → Oscillator is stopped (battery mode only)
 *
 * @warning Disabling the oscillator will stop timekeeping.
 *
 * @example
 * ds3231_setOscillator(DS3231_ENABLED);
 */
void ds3231_setOscillator(DS3231_State_t state);


/**
 * @brief Enable or disable battery-backed square wave output.
 *
 * Controls the BBSQW bit in the control register.
 * When enabled, the square wave output remains active even
 * when the device switches to battery power.
 *
 * @param state DS3231_ENABLED  → Square wave available in battery mode
 *              DS3231_DISABLED → Square wave disabled in battery mode
 *
 * @note This only affects behavior during battery backup.
 *       It has no effect while running on VCC.
 *
 * @example
 * ds3231_setBatteryBackedSquareWave(DS3231_ENABLED);
 */
void ds3231_setBatteryBackedSquareWave(DS3231_State_t state);


/**
 * @brief Configure square wave output frequency.
 *
 * Selects the frequency of the square wave available
 * on the INT/SQW pin when square wave mode is active.
 *
 * Available frequencies:
 *   - DS3231_FREQ_1HZ
 *   - DS3231_FREQ_1024Hz
 *   - DS3231_FREQ_4096Hz
 *   - DS3231_FREQ_8192Hz
 *
 * @param freq Desired square wave frequency.
 *
 * @note This function modifies RS1 and RS2 bits in the control register.
 *       Square wave output must be enabled using ds3231_setOutputMode().
 *
 * @example
 * ds3231_setSquareWaveOutputFreq(DS3231_FREQ_1HZ);
 */
void ds3231_setSquareWaveOutputFreq(DS3231_SquareWaveFreq_t freq);

/**
 * @brief Configure INT/SQW pin operating mode.
 *
 * Selects whether the INT/SQW pin behaves as:
 *   - Square wave output
 *   - Alarm interrupt output
 *
 * @param mode DS3231_OUTPUT_SQUARE_WAVE → Pin outputs square wave
 *             DS3231_ALARM_INTERRUPT    → Pin outputs alarm interrupt
 *
 * @note This modifies the INTCN bit in the control register.
 *       Alarm interrupts require Alarm1 or Alarm2 to be enabled.
 *
 * @example
 * ds3231_setOutputMode(DS3231_OUTPUT_SQUARE_WAVE);
 */
void ds3231_setOutputMode(DS3231_OutputMode_t mode);


/**
 * @brief Set Alarm1 time (seconds, minutes, hours).
 *
 * Alarm1 supports comparison of:
 *   - Seconds
 *   - Minutes
 *   - Hours
 *   - Date or Day
 *
 * This function sets the time portion of Alarm1.
 *
 * @param time Pointer to RTC_time_t containing:
 *        - seconds (0–59)
 *        - minutes (0–59)
 *        - hours   (1–12 or 0–23 depending on format)
 *        - hour_format
 *        - meridiem (used only in 12h mode)
 *
 * @note Alarm1 also requires date/day and mode configuration.
 *
 * @example
 * RTC_time_t alarmTime = {
 *     .seconds = 0,
 *     .minutes = 30,
 *     .hours = 7,
 *     .hour_format = HOUR_FORMAT_24
 * };
 *
 * ds3231_setAlarm1Time(&alarmTime);
 */
void ds3231_setAlarm1Time(RTC_time_t *time);



/**
 * @brief Set Alarm1 date (1–31).
 *
 * Configures Alarm1 to trigger based on calendar date.
 *
 * @param date Day of month (1–31)
 *
 * @note Must use DS3231_ALM1_MATCH_SEC_MIN_HRS_DATE mode
 *       if you want date-based matching.
 */
void ds3231_setAlarm1Date(uint8_t date);



/**
 * @brief Set Alarm1 weekday (Sunday–Saturday).
 *
 * Configures Alarm1 to trigger based on weekday.
 *
 * @param day Day of week (SUNDAY–SATURDAY)
 *
 * @note Must use DS3231_ALM1_MATCH_SEC_MIN_HRS_DAY mode
 *       to enable weekday comparison.
 */
void ds3231_setAlarm1Day(RTC_day_t day);


/**
 * @brief Configure Alarm1 match mode.
 *
 * Determines which fields must match for Alarm1 to trigger.
 *
 * Available modes:
 *   - DS3231_ALM1_EVERY_SEC
 *   - DS3231_ALM1_MATCH_SEC
 *   - DS3231_ALM1_MATCH_SEC_MIN
 *   - DS3231_ALM1_MATCH_SEC_MIN_HRS
 *   - DS3231_ALM1_MATCH_SEC_MIN_HRS_DATE
 *   - DS3231_ALM1_MATCH_SEC_MIN_HRS_DAY
 *
 * @param mode Alarm1 comparison mode.
 *
 * @note Mode must match your selected date/day configuration.
 *
 * @example
 * ds3231_setAlarm1Mode(DS3231_ALM1_MATCH_SEC_MIN_HRS_DATE);
 */
void ds3231_setAlarm1Mode(DS3231_Alarm1Mode_t mode);


/**
 * @brief Set Alarm2 time (minutes, hours).
 *
 * Alarm2 supports comparison of:
 *   - Minutes
 *   - Hours
 *   - Date or Day
 *
 * @note Alarm2 does NOT support seconds.
 *
 * @param time Pointer to RTC_time_t containing:
 *        - minutes
 *        - hours
 *        - hour_format
 *        - meridiem (used only in 12h mode)
 */
void ds3231_setAlarm2Time(RTC_time_t *time);


/**
 * @brief Set Alarm2 date (1–31).
 *
 * Configures Alarm2 to trigger based on calendar date.
 *
 * @param date Day of month (1–31)
 *
 * @note Must use DS3231_ALM2_MATCH_MIN_HRS_DATE mode.
 */
void ds3231_setAlarm2Date(uint8_t date);


/**
 * @brief Set Alarm2 weekday (Sunday–Saturday).
 *
 * Configures Alarm2 to trigger based on weekday.
 *
 * @param day Day of week (SUNDAY–SATURDAY)
 *
 * @note Must use DS3231_ALM2_MATCH_MIN_HRS_DAY mode.
 */
void ds3231_setAlarm2Day(RTC_day_t day);


/**
 * @brief Configure Alarm2 match mode.
 *
 * Determines which fields must match for Alarm2 to trigger.
 *
 * Available modes:
 *   - DS3231_ALM2_EVERY_MIN
 *   - DS3231_ALM2_MATCH_MIN
 *   - DS3231_ALM2_MATCH_MIN_HRS
 *   - DS3231_ALM2_MATCH_MIN_HRS_DATE
 *   - DS3231_ALM2_MATCH_MIN_HRS_DAY
 *
 * @param mode Alarm2 comparison mode.
 */
void ds3231_setAlarm2Mode(DS3231_Alarm2Mode_t mode);



/**
 * @brief Enable or disable Alarm1 interrupt.
 *
 * Controls the A1IE bit in the control register.
 * When enabled, Alarm1 can generate an interrupt on the INT/SQW pin
 * if interrupt mode is selected.
 *
 * @param enable DS3231_ENABLED  → Enable Alarm1 interrupt
 *               DS3231_DISABLED → Disable Alarm1 interrupt
 *
 * @note Alarm interrupt output requires:
 *       1. Alarm mode configured
 *       2. Alarm enabled
 *       3. INT/SQW pin set to interrupt mode
 *
 * @example
 * ds3231_enableAlarm1(DS3231_ENABLED);
 */
void ds3231_enableAlarm1(DS3231_State_t enable);


/**
 * @brief Enable or disable Alarm2 interrupt.
 *
 * Controls the A2IE bit in the control register.
 *
 * @param enable DS3231_ENABLED  → Enable Alarm2 interrupt
 *               DS3231_DISABLED → Disable Alarm2 interrupt
 *
 * @example
 * ds3231_enableAlarm2(DS3231_ENABLED);
 */
void ds3231_enableAlarm2(DS3231_State_t enable);


/**
 * @brief Check if Alarm1 has triggered.
 *
 * Reads the A1F flag from the status register.
 *
 * @return 1 if Alarm1 flag is set
 * @return 0 if Alarm1 has not triggered
 *
 * @note The flag remains set until manually cleared.
 *
 * @example
 * if(ds3231_isAlarm1Triggered())
 * {
 *     // Alarm1 event occurred
 * }
 */
uint8_t ds3231_isAlarm1Triggered(void);


/**
 * @brief Check if Alarm2 has triggered.
 *
 * Reads the A2F flag from the status register.
 *
 * @return 1 if Alarm2 flag is set
 * @return 0 if Alarm2 has not triggered
 */
uint8_t ds3231_isAlarm2Triggered(void);

/**
 * @brief Clear Alarm1 interrupt flag.
 *
 * Clears the A1F bit in the status register.
 * Must be called after Alarm1 triggers to allow
 * future alarm events.
 *
 * @warning If not cleared, interrupt may remain active.
 *
 * @example
 * ds3231_clearAlarm1Flag();
 */
void ds3231_clearAlarm1Flag(void);


/**
 * @brief Clear Alarm2 interrupt flag.
 *
 * Clears the A2F bit in the status register.
 *
 * @note Required after Alarm2 trigger.
 */
void ds3231_clearAlarm2Flag(void);



#ifdef __cplusplus
}
#endif

#endif // _DS3231_H
