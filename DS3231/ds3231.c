#include "ds3231.h"

#ifdef __cplusplus
extern "C" {
#endif

I2C_HandleTypeDef *ds3231_i2c;
static uint8_t ds3231_address = DS3231_I2C_ADDRESS;

static void ds3231_write_data(uint8_t reg, uint8_t data){
    //     uint8_t buffer[2] = {reg, data};
    // HAL_I2C_Master_Transmit(ds3231_i2c, ds3231_address  << 1, buffer, 2, DS3231_TIMEOUT);
    HAL_I2C_Mem_Write(ds3231_i2c, ds3231_address  << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, DS3231_TIMEOUT);

}
static uint8_t ds3231_read_data(uint8_t reg){
    uint8_t data = 0;
    /* HAL_I2C_Master_Transmit(ds3231_i2c, ds3231_address  << 1, &reg, 1, DS3231_TIMEOUT);
    HAL_I2C_Master_Receive(ds3231_i2c, ds3231_address  << 1, &data, 1, DS3231_TIMEOUT); */
    if(HAL_I2C_Mem_Read(ds3231_i2c, ds3231_address  << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, DS3231_TIMEOUT) != HAL_OK){
        return 0;
    }
    return data;
}

static uint8_t ds3231_bin2bcd(uint16_t dec){
    return ((dec / 10) << 4) | (dec % 10);
}

static uint16_t ds3231_bcd2bin(uint8_t bcd){
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}


void ds3231_init(I2C_HandleTypeDef *hi2c, uint8_t address){
    ds3231_i2c = hi2c;
    ds3231_address = address;
}

static void ds3231_setSecond(uint8_t sec){
    ds3231_write_data(DS3231_ADDR_SEC, ds3231_bin2bcd(sec));
}

static void ds3231_setMinute(uint8_t min){
    ds3231_write_data(DS3231_ADDR_MIN, ds3231_bin2bcd(min));
}


static void ds3231_setHour(uint8_t hour, RTC_HourFormat_t format, RTC_Meridiem_t meridiem){
    uint8_t reg = 0;
    if(format == HOUR_FORMAT_12){
        assert_param(hour >= 1 && hour <= 12);
        reg |= (1 << 6);   // Set bit 6 → 12h mode

        if(meridiem == PM){
            reg |= (1 << 5);  // Set PM
        }
        else{
            reg &= ~(1 << 5); // set AM
        }
        reg |= (ds3231_bin2bcd(hour) & 0x1F);  // Hour bits (0–4)
    }

    else{   // 24h format
        assert_param(hour <= 23);
        reg &= ~(1 << 6);  // Clear bit 6 → 24h mode
        reg |= ds3231_bin2bcd(hour & 0x3F);  // Hour bits (0–5)
    }

    ds3231_write_data(DS3231_ADDR_HRS, reg);
}


static void ds3231_setDay(RTC_day_t day){
    ds3231_write_data(DS3231_ADDR_DAY, ds3231_bin2bcd(day));
}

static void ds3231_setDateReg(uint8_t date){
    assert_param(date >= 1 && date <= 31);
    ds3231_write_data(DS3231_ADDR_DATE, ds3231_bin2bcd(date));
}

static void ds3231_setMonth(uint8_t month){
    assert_param(month >= 1 && month <= 12);
    ds3231_write_data(DS3231_ADDR_MONTH, ds3231_bin2bcd(month));
}

static void ds3231_setYear(uint16_t year){
    assert_param(year >= 2000 && year <= 2199);

    uint8_t century = (year >= 2100) ? 1 : 0;

    /* Update century bit in month register */
    uint8_t monthReg = ds3231_read_data(DS3231_ADDR_MONTH);

    monthReg &= 0x7F;              // Clear century bit
    monthReg |= (century << 7);    // Set correct century

    ds3231_write_data(DS3231_ADDR_MONTH, monthReg);
    ds3231_write_data(DS3231_ADDR_YEAR, ds3231_bin2bcd(year % 100));
}



void ds3231_setTime(RTC_time_t *time){
    assert_param(time != NULL);
    ds3231_setSecond(time->seconds);
    ds3231_setMinute(time->minutes);
    ds3231_setHour(time->hours, time->hour_format, time->meridiem);
}


void ds3231_getTime(RTC_time_t *time){
    assert_param(time != NULL);
    uint8_t hourReg = ds3231_read_data(DS3231_ADDR_HRS);
    uint8_t minReg  = ds3231_read_data(DS3231_ADDR_MIN);
    uint8_t secReg  = ds3231_read_data(DS3231_ADDR_SEC);

    time->hour_format = (hourReg & (1 << 6)) ? HOUR_FORMAT_12 : HOUR_FORMAT_24;

    if(time->hour_format == HOUR_FORMAT_12){
        time->meridiem = (hourReg & (1 << 5)) ? PM : AM;
        time->hours = ds3231_bcd2bin(hourReg & 0x1F);
    }
    else{
        time->meridiem = AM;  // not meaningful in 24h
        time->hours = ds3231_bcd2bin(hourReg & 0x3F);
    }

    time->minutes = ds3231_bcd2bin(minReg & 0x7F);
    time->seconds = ds3231_bcd2bin(secReg & 0x7F);
}



void ds3231_setDate(RTC_date_t *date){
    assert_param(date != NULL);
    ds3231_setDateReg(date->date);
    ds3231_setMonth(date->month);
    ds3231_setYear(date->year);
    ds3231_setDay(date->day);
}

void ds3231_getDate(RTC_date_t *date){
    assert_param(date != NULL);
    uint8_t dateReg = ds3231_read_data(DS3231_ADDR_DATE);
    uint8_t monthReg = ds3231_read_data(DS3231_ADDR_MONTH);
    uint8_t yearReg = ds3231_read_data(DS3231_ADDR_YEAR);
    uint8_t dayReg = ds3231_read_data(DS3231_ADDR_DAY);

    date->date = ds3231_bcd2bin(dateReg & 0x3F);
    date->month = ds3231_bcd2bin(monthReg & 0x1F);
    date->day = ds3231_bcd2bin(dayReg & 0x07);

    uint16_t year2digits = ds3231_bcd2bin(yearReg);
    uint8_t century = (monthReg & (1 << 7)) ? 1 : 0;
    date->year = 2000 + (century * 100) + year2digits;
}


float ds3231_getTemperature(void){
    int8_t tempMSB = (int8_t)ds3231_read_data(DS3231_ADDR_TEMP_MSB);
    uint8_t tempLSB = ds3231_read_data(DS3231_ADDR_TEMP_LSB);
    int16_t tempRaw = (tempMSB << 2) | (tempLSB >> 6);
    return tempRaw / 4.0f;
}


void ds3231_set32kHzOutput(DS3231_State_t state){
    assert_param(state == DS3231_ENABLED || state == DS3231_DISABLED);
    uint8_t statusReg = ds3231_read_data(DS3231_ADDR_STATUS);
    if(state == DS3231_ENABLED){
        statusReg |= (1 << DS3231_STATUS_EN32KHZ);
    }
    else{
        statusReg &= ~(1 << DS3231_STATUS_EN32KHZ);
    }
    ds3231_write_data(DS3231_ADDR_STATUS, statusReg);
}

uint8_t ds3231_is32kHzOutputEnabled(void){
    uint8_t statusReg = ds3231_read_data(DS3231_ADDR_STATUS);
    return (statusReg & (1 << DS3231_STATUS_EN32KHZ)) ? 1 : 0;
}

uint8_t ds3231_isOscillatorStopped(void){
    uint8_t statusReg = ds3231_read_data(DS3231_ADDR_STATUS);
    return (statusReg & (1 << DS3231_STATUS_OSF)) ? 1 : 0;
}

void ds3231_setOscillator(DS3231_State_t state){
    uint8_t controlReg = ds3231_read_data(DS3231_ADDR_CONTROL);
    if(state == DS3231_ENABLED){
        controlReg &= ~(1 << DS3231_CONTROL_EOSC);
    }
    else{
        controlReg |= (1 << DS3231_CONTROL_EOSC);
    }
    ds3231_write_data(DS3231_ADDR_CONTROL, controlReg);
}

void ds3231_setBatteryBackedSquareWave(DS3231_State_t state){
    uint8_t controlReg = ds3231_read_data(DS3231_ADDR_CONTROL);
    if(state == DS3231_ENABLED){
        controlReg |= (1 << DS3231_CONTROL_BBSQW);
    }
    else{
        controlReg &= ~(1 << DS3231_CONTROL_BBSQW);
    }
    ds3231_write_data(DS3231_ADDR_CONTROL, controlReg);
}



void ds3231_setSquareWaveOutputFreq(DS3231_SquareWaveFreq_t freq){
    uint8_t controlReg = ds3231_read_data(DS3231_ADDR_CONTROL);
    controlReg &= ~(0x03 << DS3231_CONTROL_RS1);
    controlReg |= ((freq & 0x03) << DS3231_CONTROL_RS1);
    ds3231_write_data(DS3231_ADDR_CONTROL, controlReg);

}


void ds3231_setOutputMode(DS3231_OutputMode_t mode){
    uint8_t controlReg = ds3231_read_data(DS3231_ADDR_CONTROL);
    if(mode == DS3231_OUTPUT_SQUARE_WAVE){
        controlReg &= ~(1 << DS3231_CONTROL_INTCN);
    }
    else{
        controlReg |= (1 << DS3231_CONTROL_INTCN);
    }
    ds3231_write_data(DS3231_ADDR_CONTROL, controlReg);
}

void ds3231_setAlarm1Second(uint8_t sec){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_ALM1_SEC);
    reg &= ~(0x7F);
    reg |= ds3231_bin2bcd(sec) & 0x7f;

    ds3231_write_data(DS3231_ADDR_ALM1_SEC, reg);
}

void ds3231_setAlarm1Minute(uint8_t min){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_ALM1_MIN);
    reg &= ~(0x7F);
    reg |= ds3231_bin2bcd(min) & 0x7f;

    ds3231_write_data(DS3231_ADDR_ALM1_MIN, reg);
}

void ds3231_setAlarm1Hour(uint8_t hour, RTC_HourFormat_t format, RTC_Meridiem_t meridiem){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_ALM1_HRS);
    reg &= ~(0x7f);
    if(format == HOUR_FORMAT_12){
        reg |= (1 << 6);   // Set bit 6
        
        if(meridiem == PM){
            reg |= (1 << 5);  // Set PM
        }
        else{
            reg &= ~(1 << 5); // set AM
        }
        reg |= ds3231_bin2bcd(hour) & 0x1f;
    }
    else{   // HOUR_FORMAT_24
        reg &= ~(1 << 6);  // Clear bit 6
        reg |= ds3231_bin2bcd(hour) & 0x3f;
    }
    ds3231_write_data(DS3231_ADDR_ALM1_HRS, reg);
}

void ds3231_setAlarm1Day(RTC_day_t day){
    uint8_t reg = 0;
    reg = ds3231_read_data(DS3231_ADDR_ALM1_DAYDATE);
    reg &= ~(0x3f);
    reg |= (1 << 6);  // Set bit 6 to select day mode
    reg |= ds3231_bin2bcd(day);
    ds3231_write_data(DS3231_ADDR_ALM1_DAYDATE, reg);
}

void ds3231_setAlarm1Date(uint8_t date){
    uint8_t reg = 0;
    reg = ds3231_read_data(DS3231_ADDR_ALM1_DAYDATE);
    reg &= ~(0x3f);
    reg &= ~(1 << 6);  // Clear bit 6 to select date mode
    reg |= ds3231_bin2bcd(date);
    ds3231_write_data(DS3231_ADDR_ALM1_DAYDATE, reg);    
}


void ds3231_setAlarm1Time(RTC_time_t *time){
    assert_param(time != NULL);
    ds3231_setAlarm1Second(time->seconds);
    ds3231_setAlarm1Minute(time->minutes);
    ds3231_setAlarm1Hour(time->hours, time->hour_format, time->meridiem);
}

void ds3231_setAlarm2Minute(uint8_t min){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_ALM2_MIN);
    reg &= ~(0x7F);
    reg |= ds3231_bin2bcd(min) & 0x7f;

    ds3231_write_data(DS3231_ADDR_ALM2_MIN, reg);
}

void ds3231_setAlarm2Hour(uint8_t hour, RTC_HourFormat_t format, RTC_Meridiem_t meridiem){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_ALM2_HRS);
    reg &= ~(0x7f);
    if(format == HOUR_FORMAT_12){
        reg |= (1 << 6);   // Set bit 6
        
        if(meridiem == PM){
            reg |= (1 << 5);  // Set PM
        }
        else{
            reg &= ~(1 << 5); // set AM
        }
        reg |= ds3231_bin2bcd(hour) & 0x1f;
    }
    else{   // HOUR_FORMAT_24
        reg &= ~(1 << 6);  // Clear bit 6
        reg |= ds3231_bin2bcd(hour) & 0x3f;
    }
    ds3231_write_data(DS3231_ADDR_ALM2_HRS, reg);
}

void ds3231_setAlarm2Day(RTC_day_t day){
    uint8_t reg = 0;
    reg = ds3231_read_data(DS3231_ADDR_ALM2_DAYDATE);
    reg &= ~(0x3f);
    reg |= (1 << 6);  // Set bit 6 to select day mode
    reg |= ds3231_bin2bcd(day);
    ds3231_write_data(DS3231_ADDR_ALM2_DAYDATE, reg);
}

void ds3231_setAlarm2Date(uint8_t date){
    uint8_t reg = 0;
    reg = ds3231_read_data(DS3231_ADDR_ALM2_DAYDATE);
    reg &= ~(0x3f);
    reg &= ~(1 << 6);  // Clear bit 6 to select date mode
    reg |= ds3231_bin2bcd(date);
    ds3231_write_data(DS3231_ADDR_ALM2_DAYDATE, reg);    
}

void ds3231_setAlarm2Time(RTC_time_t *time){
    assert_param(time != NULL);
    ds3231_setAlarm2Minute(time->minutes);
    ds3231_setAlarm2Hour(time->hours, time->hour_format, time->meridiem);
}

void ds3231_setAlarm1Mode(DS3231_Alarm1Mode_t mode){
    uint8_t reg = 0;

    reg = ds3231_read_data(DS3231_ADDR_ALM1_SEC) & 0x7f;
    reg |= ((mode >> 0) & 0x01) << DS3231_AXMY;
    ds3231_write_data(DS3231_ADDR_ALM1_SEC, reg);

    reg = ds3231_read_data(DS3231_ADDR_ALM1_MIN) & 0x7f;
    reg |= ((mode >> 1) & 0x01) << DS3231_AXMY;
    ds3231_write_data(DS3231_ADDR_ALM1_MIN, reg);

    reg = ds3231_read_data(DS3231_ADDR_ALM1_HRS) & 0x7f;
    reg |= ((mode >> 2) & 0x01) << DS3231_AXMY;
    ds3231_write_data(DS3231_ADDR_ALM1_HRS, reg);

    reg = ds3231_read_data(DS3231_ADDR_ALM1_DAYDATE) & 0x3f;
    reg |= ( ((mode >> 3) & 0x01) << DS3231_AXMY ) | ( ((mode >> 4) & 0x01) << DS3231_DYDT );
    ds3231_write_data(DS3231_ADDR_ALM1_DAYDATE, reg);
    
}

void ds3231_setAlarm2Mode(DS3231_Alarm2Mode_t mode){
    uint8_t reg = 0;

    reg = ds3231_read_data(DS3231_ADDR_ALM2_MIN) & 0x7f;
    reg |= ((mode >> 0) & 0x01) << DS3231_AXMY;
    ds3231_write_data(DS3231_ADDR_ALM2_MIN, reg);

    reg = ds3231_read_data(DS3231_ADDR_ALM2_HRS) & 0x7f;
    reg |= ((mode >> 1) & 0x01) << DS3231_AXMY;
    ds3231_write_data(DS3231_ADDR_ALM2_HRS, reg);

    reg = ds3231_read_data(DS3231_ADDR_ALM2_DAYDATE) & 0x3f;
    reg |= ( ((mode >> 2) & 0x01) << DS3231_AXMY ) | ( ((mode >> 3) & 0x01) << DS3231_DYDT );
    ds3231_write_data(DS3231_ADDR_ALM2_DAYDATE, reg);
}

void ds3231_enableAlarm1(DS3231_State_t enable){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_CONTROL);
    if(enable == DS3231_ENABLED){
        reg |= (1 << DS3231_CONTROL_A1IE);
    }
    else{
        reg &= ~(1 << DS3231_CONTROL_A1IE);
    }
    ds3231_write_data(DS3231_ADDR_CONTROL, reg);

    ds3231_setOutputMode(DS3231_ALARM_INTERRUPT);
}

void ds3231_enableAlarm2(DS3231_State_t enable){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_CONTROL);
    if(enable == DS3231_ENABLED){
        reg |= (1 << DS3231_CONTROL_A2IE);
    }
    else{
        reg &= ~(1 << DS3231_CONTROL_A2IE);
    }
    ds3231_write_data(DS3231_ADDR_CONTROL, reg);
    ds3231_setOutputMode(DS3231_ALARM_INTERRUPT);
}


void ds3231_clearAlarm1Flag(void){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_STATUS);
    reg &= ~(1 << DS3231_STATUS_A1F);
    ds3231_write_data(DS3231_ADDR_STATUS, reg);
}

void ds3231_clearAlarm2Flag(void){
    uint8_t reg = ds3231_read_data(DS3231_ADDR_STATUS);
    reg &= ~(1 << DS3231_STATUS_A2F);
    ds3231_write_data(DS3231_ADDR_STATUS, reg);
}

uint8_t ds3231_isAlarm1Triggered(void){
    return (ds3231_read_data(DS3231_ADDR_STATUS) >> DS3231_STATUS_A1F) & 0x01;
}

uint8_t ds3231_isAlarm2Triggered(void){
    return (ds3231_read_data(DS3231_ADDR_STATUS) >> DS3231_STATUS_A2F) & 0x01;
}






#ifdef __cplusplus
}
#endif