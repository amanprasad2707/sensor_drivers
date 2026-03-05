# DS3231 STM32 HAL Driver

A lightweight and fully featured **STM32 HAL driver** for the **DS3231 Real-Time Clock (RTC)**.

This driver provides:
- Time & Date configuration
- Alarm1 and Alarm2 support (all mask modes)
- Interrupt mode (INT/SQW pin)
- Square wave frequency generation
- 32.768 kHz output control
- Temperature reading
- Oscillator and status monitoring

Designed for clean abstraction and easy integration with STM32CubeMX projects.

---

## 📦 Features

- ✔ Time and Date management (12h / 24h support)
- ✔ Alarm1 (seconds supported)
- ✔ Alarm2 (minutes precision)
- ✔ Alarm interrupt mode
- ✔ Square wave output (1Hz – 8.192kHz)
- ✔ Dedicated 32.768kHz output
- ✔ Oscillator stop detection
- ✔ Temperature sensor access
- ✔ Clean bit-level abstraction
- ✔ Polling or interrupt-based usage

---

## 🔌 Hardware Connections

### Minimal Setup

| DS3231 Pin | STM32                          |
|------------|--------------------------------|
| VCC        | 3.3V                           |
| GND        | GND                            |
| SDA        | I2C SDA                        |
| SCL        | I2C SCL                        |
| INT/SQW    | EXTI GPIO (optional)           |
| 32kHz      | Optional clock measurement     |

> ⚠️ **INT/SQW is open-drain** → use a pull-up resistor or enable internal pull-up.

---

## ⚙️ STM32CubeMX Configuration

1. Enable **I2C** (e.g., I2C1)
2. Enable **USART** (optional, for debugging)
3. Configure external interrupt pin (e.g., PA0):
   - Mode: `GPIO_EXTI`
   - Trigger: Falling Edge
   - Pull-up: Enabled
4. Enable the corresponding EXTI interrupt in NVIC
5. Generate code

---

## 🚀 Quick Start

### 1️⃣ Include Driver

```c
#include "ds3231.h"
```

### 2️⃣ Initialize

```c
ds3231_init(&hi2c1, 0x68);
```

### 3️⃣ Set Time

```c
RTC_time_t time = {
    .seconds     = 0,
    .minutes     = 30,
    .hours       = 10,
    .hour_format = HOUR_FORMAT_24
};

ds3231_setTime(&time);
```

### 4️⃣ Set Date

```c
RTC_date_t date = {
    .date  = 21,
    .month = 9,
    .year  = 2025,
    .day   = SUNDAY
};

ds3231_setDate(&date);
```

---

## 🔔 Alarm Usage

### Alarm1 – Daily at 10:50:00

```c
RTC_time_t alarm = {
    .seconds     = 0,
    .minutes     = 50,
    .hours       = 10,
    .hour_format = HOUR_FORMAT_24
};

ds3231_setAlarm1Time(&alarm);
ds3231_setAlarm1Mode(DS3231_ALM1_MATCH_SEC_MIN_HRS);

ds3231_clearAlarm1Flag();
ds3231_setOutputMode(DS3231_ALARM_INTERRUPT);
ds3231_enableAlarm1(DS3231_ENABLED);
```

### Interrupt Callback

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        if (ds3231_isAlarm1Triggered())
        {
            ds3231_clearAlarm1Flag();
            // Alarm action here
        }
    }
}
```

---

## 🔔 Alarm Modes

### Alarm1 Modes

| Mode | Description |
|------|-------------|
| `DS3231_ALM1_EVERY_SEC` | Trigger every second |
| `DS3231_ALM1_MATCH_SEC` | Match seconds |
| `DS3231_ALM1_MATCH_SEC_MIN` | Match seconds + minutes |
| `DS3231_ALM1_MATCH_SEC_MIN_HRS` | Match time daily |
| `DS3231_ALM1_MATCH_SEC_MIN_HRS_DATE` | Match specific date |
| `DS3231_ALM1_MATCH_SEC_MIN_HRS_DAY` | Match specific weekday |

### Alarm2 Modes

| Mode | Description |
|------|-------------|
| `DS3231_ALM2_EVERY_MIN` | Every minute |
| `DS3231_ALM2_MATCH_MIN` | Match minute |
| `DS3231_ALM2_MATCH_MIN_HRS` | Match time daily |
| `DS3231_ALM2_MATCH_MIN_HRS_DATE` | Match specific date |
| `DS3231_ALM2_MATCH_MIN_HRS_DAY` | Match specific weekday |

---

## 🔄 Square Wave Output

```c
ds3231_setOutputMode(DS3231_OUTPUT_SQUARE_WAVE);
ds3231_setSquareWaveOutputFreq(DS3231_FREQ_1HZ);
```

Available frequencies:

| Constant | Frequency |
|----------|-----------|
| `DS3231_FREQ_1HZ` | 1 Hz |
| `DS3231_FREQ_1024HZ` | 1.024 kHz |
| `DS3231_FREQ_4096HZ` | 4.096 kHz |
| `DS3231_FREQ_8192HZ` | 8.192 kHz |

---

## 🕒 32.768 kHz Output

```c
ds3231_set32kHzOutput(DS3231_ENABLED);
```

Dedicated 32kHz pin provides a stable reference clock.

---

## 🌡️ Temperature Reading

```c
float temperature = ds3231_getTemperature();
```

> Resolution: 0.25°C

---

## ⚠️ Important Notes

- INT pin is open-drain → requires pull-up
- Always clear alarm flags after trigger
- Do **NOT** use `HAL_Delay()` inside ISR
- Alarm2 does not support seconds
- Clear flags **before** enabling alarms

---

## 🗂️ Example Projects

See `/examples` folder for:

- Basic time/date demo
- Alarm polling example
- Alarm interrupt example
- Square wave output demo
- 32kHz output demo

---

## 📚 Reference

- [DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)

