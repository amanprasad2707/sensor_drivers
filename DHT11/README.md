# DHT11 Temperature & Humidity Sensor Library
### GPIO Bit-Bang + Timer | STM32 HAL

A simple library to read temperature and humidity from the DHT11 sensor using a single GPIO pin on STM32 microcontrollers.

---

## How It Works

The DHT11 uses a single-wire protocol. Communication is done by bit-banging a GPIO pin — the library switches the pin between output and input mode as needed.

A hardware timer (TIM7) is used to generate accurate microsecond delays required by the protocol timing.

**Read sequence:**
1. Master pulls line LOW for ≥18ms (start signal)
2. Master releases line, sensor responds with LOW then HIGH (~80µs each)
3. Sensor sends 40 bits — 5 bytes of data
4. Library validates checksum and stores result in handle

---

## STM32CubeMX Configuration

### Timer — TIM7 (required for µs delays)

1. Enable **TIM7** under Timers
2. Set **Clock Source** to Internal Clock
3. Set **Prescaler** to `(SystemCoreClock / 1000000) - 1`
   - For 168MHz: `167`
   - For 84MHz: `83`
   - For 72MHz: `71`
4. Set **Period** to `65535`
5. No interrupt needed

### GPIO — DHT11 data pin

1. Configure the data pin as **GPIO_Output**
2. Output level: High
3. No pull — external 10kΩ pull-up resistor is used instead

---

## Wiring

```
DHT11 VCC  ── 3.3V
DHT11 GND  ── GND
DHT11 DATA ──┬── 10kΩ ── 3.3V
             └── STM32 GPIO pin (e.g. PA0)
```

> The **10kΩ pull-up resistor** is required. Without it the signal line will float and reads will fail.

---

## Files

| File | Description |
|------|-------------|
| `dht11.h` | Header — types, constants, function prototypes |
| `dht11.c` | Implementation |

---

## Quick Start

### 1. Include the header

```c
#include "dht11.h"
```

### 2. Create a handle

```c
DHT11_HandleTypeDef hdht11;
```

### 3. Start the timer and initialize

```c
HAL_TIM_Base_Start(&htim7);
dht11_init(&hdht11, GPIOA, GPIO_PIN_0);
```

### 4. Read and get values

```c
float   temperature = 0.0f;
uint8_t humidity    = 0;

dht11_status_t ret = dht11_read_data(&hdht11);

if (ret == DHT11_OK) {
    dht11_get_temperature(&hdht11, &temperature);
    dht11_get_humidity(&hdht11, &humidity);
}
```

> ⚠️ Always call `dht11_read_data()` first. `dht11_get_temperature()` and `dht11_get_humidity()` only return values from the last successful read — they do not trigger a new read.

---

## API Reference

### `dht11_init()`
```c
dht11_status_t dht11_init(DHT11_HandleTypeDef *dht11, GPIO_TypeDef *port, uint16_t pin);
```
Initializes the handle. Does not communicate with the sensor.

| Parameter | Description |
|-----------|-------------|
| `dht11` | Pointer to DHT11 handle |
| `port` | GPIO port (e.g. `GPIOA`) |
| `pin` | GPIO pin (e.g. `GPIO_PIN_0`) |

---

### `dht11_read_data()`
```c
dht11_status_t dht11_read_data(DHT11_HandleTypeDef *dht11);
```
Triggers a full read from the sensor. Sends start signal, reads 40 bits, validates checksum, stores result in handle. Call this before `get_temperature` or `get_humidity`.

> ⚠️ DHT11 minimum sampling interval is **2 seconds**. Calling this faster will return errors or stale data.

---

### `dht11_get_temperature()`
```c
dht11_status_t dht11_get_temperature(DHT11_HandleTypeDef *dht11, float *temperature);
```
Returns temperature in °C from the last successful `dht11_read_data()` call.

---

### `dht11_get_humidity()`
```c
dht11_status_t dht11_get_humidity(DHT11_HandleTypeDef *dht11, uint8_t *humidity);
```
Returns relative humidity in % from the last successful `dht11_read_data()` call.

---

## Return Codes

| Code | Value | Meaning |
|------|-------|---------|
| `DHT11_OK` | 0 | Success |
| `DHT11_ERROR` | 1 | Null pointer passed |
| `DHT11_TIMEOUT` | 2 | Sensor did not respond in time |
| `DHT11_CHECKSUM_ERROR` | 3 | Data corrupted — checksum mismatch |

---

## Sensor Specifications

| Parameter | Value |
|-----------|-------|
| Temperature range | 0 – 50 °C |
| Temperature accuracy | ±2 °C |
| Humidity range | 20 – 90 % RH |
| Humidity accuracy | ±5 % RH |
| Sampling interval | ≥ 2 seconds |
| Supply voltage | 3.3V or 5V |

---

## Troubleshooting

| Symptom | Likely Cause |
|---------|-------------|
| Always returns `DHT11_TIMEOUT` | Missing pull-up resistor, wrong pin, or sensor not powered |
| Always returns `DHT11_CHECKSUM_ERROR` | Noisy line, missing pull-up, or reading too fast (< 2s interval) |
| Temperature or humidity reads `0` | `dht11_read_data()` returned an error — check return value |
| Inconsistent reads | Timer prescaler wrong — verify 1µs tick at your clock speed |
| Works sometimes, fails randomly | Wire too long — keep data line under 20cm, add decoupling cap near VCC |