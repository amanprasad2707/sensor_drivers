# DS18B20 Temperature Sensor Library
### UART Half-Duplex + DMA | STM32 HAL

A simple, reliable library to read temperature from the DS18B20 sensor using **UART in half-duplex mode** on STM32 microcontrollers. No dedicated 1-Wire peripheral needed — just a regular UART.

---

## How It Works (Quick Explanation)

The DS18B20 uses a protocol called **1-Wire**, which means all communication happens over a single wire. This library tricks the UART peripheral into acting as a 1-Wire master:

| Baud Rate | Used For |
|-----------|----------|
| 9600 | Sending reset pulse & detecting sensor |
| 115200 | Reading and writing data bits |

Each bit sent to the sensor is actually one full UART byte (`0xFF` = bit 1, `0x00` = bit 0). When reading, the sensor can pull the line low — so the byte received back tells us what bit the sensor sent.

**DMA** (Direct Memory Access) is used during reads so the CPU doesn't have to wait — the hardware handles the data transfer and signals when it's done.

---

## STM32CubeMX Configuration

> ⚠️ This is the most important step. If UART is not set up correctly, nothing will work.

### UART Settings

1. Open your project in **STM32CubeMX**
2. Select the UART peripheral you want to use (e.g. `USART1`)
3. Set mode to **Single Wire (Half-Duplex)**
4. Configure as follows:

| Parameter | Value |
|-----------|-------|
| Baud Rate | `115200` (initial — library changes it automatically) |
| Word Length | 8 Bits |
| Parity | None |
| Stop Bits | 1 |
| Mode | TX/RX |
| Hardware Flow Control | None |

5. Under **DMA Settings**, add:
   - `USART1_RX` → DMA2 Stream 2 (or whatever is available), Direction: Peripheral→Memory, Mode: Normal
   - `USART1_TX` → DMA2 Stream 7, Direction: Memory→Peripheral, Mode: Normal

6. Under **NVIC Settings**, enable:
   - `USART1 global interrupt`
   - Both DMA stream interrupts

### Wiring

```
STM32 TX pin ──┬── 4.7kΩ resistor ── VCC (3.3V)
               └── DS18B20 DATA pin
                   DS18B20 VCC  ── 3.3V
                   DS18B20 GND  ── GND
```

> The 4.7kΩ pull-up resistor on the data line is **required**. Without it the sensor will not respond.

---

## Files

| File | Description |
|------|-------------|
| `ds18b20.h` | Header — types, constants, function prototypes |
| `ds18b20.c` | Implementation |

Copy both files into your project's `Core/Src` and `Core/Inc` folders (or wherever you keep your source files).

---

## Quick Start

### 1. Include the header

```c
#include "ds18b20.h"
```

### 2. Create a handle

```c
DS18B20_HandleTypeDef hds18b20;
```

### 3. Initialize in `main()`

```c
DS18B20_Init(&hds18b20, &huart1);
```

### 4. Read temperature

```c
float temperature;
DS18B20_Status_t status = DS18B20_GetTemperature(&hds18b20, &temperature);

if (status == DS18B20_OK) {
    // Use temperature value
}
```

### 5. Wire up the DMA callbacks

In your `stm32f4xx_it.c` or `main.c`, add these inside the HAL callback functions:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    DS18B20_RxCpltCallback(&hds18b20, huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    DS18B20_ErrorCallback(&hds18b20, huart);
}
```

> These callbacks tell the library when a DMA read has finished. **Without them, `DS18B20_ReadByte()` will always time out.**

---

## API Reference

### `DS18B20_Init()`
```c
void DS18B20_Init(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart);
```
Sets up the library. Call this once before anything else. Does not communicate with the sensor — no bus activity occurs.

| Parameter | Description |
|-----------|-------------|
| `hds` | Pointer to your DS18B20 handle |
| `huart` | Pointer to your HAL UART handle (e.g. `&huart1`) |

---

### `DS18B20_GetTemperature()` ⭐ Start here
```c
DS18B20_Status_t DS18B20_GetTemperature(DS18B20_HandleTypeDef *hds, float *temperature);
```
Does everything: triggers conversion, waits, reads result. Easiest way to get a temperature reading. Uses Skip ROM — works with one sensor on the bus.

---

### `DS18B20_StartConversion()`
```c
DS18B20_Status_t DS18B20_StartConversion(DS18B20_HandleTypeDef *hds);
```
Tells all sensors on the bus to start measuring simultaneously. **Non-blocking** — you must wait for conversion to finish before reading.

---

### `DS18B20_ReadTemperature()`
```c
DS18B20_Status_t DS18B20_ReadTemperature(DS18B20_HandleTypeDef *hds, float *temperature);
```
Reads the scratchpad and returns temperature in °C. Call this after `DS18B20_StartConversion()` + delay for single sensor, or after `DS18B20_MatchROM()` for multi-sensor reads.

---

### `DS18B20_SetResolution()`
```c
DS18B20_Status_t DS18B20_SetResolution(DS18B20_HandleTypeDef *hds, uint8_t res);
```
Changes sensor resolution and saves it to the sensor's EEPROM (survives power off). Also updates `hds->resolution` so conversion delays in `DS18B20_GetTemperature()` adjust automatically.

> 💡 Resolution is **not** set during `DS18B20_Init()`. The handle defaults to `DS18B20_RES_12BIT` internally which matches the sensor's factory default. Call `DS18B20_SetResolution()` explicitly only if you need a different resolution.

---

### `DS18B20_Reset()`
```c
DS18B20_Status_t DS18B20_Reset(DS18B20_HandleTypeDef *hds);
```
Sends a reset pulse and checks if the sensor is present. Useful for verifying a sensor is connected before starting a sequence.

---

### `DS18B20_ReadPowerSupply()`
```c
DS18B20_Status_t DS18B20_ReadPowerSupply(DS18B20_HandleTypeDef *hds, uint8_t *parasitic);
```
Checks how the sensor is being powered.

| `*parasitic` value | Meaning |
|--------------------|---------|
| `1` | External VCC connected |
| `0` | Parasitic — sensor powered from data line |

> ⚠️ If parasitic mode is detected, normal temperature conversion will fail or return garbage. A strong pull-up GPIO is required during conversion. Recommended: always connect VDD to 3.3V.

---

### `DS18B20_ReadROM()`
```c
DS18B20_Status_t DS18B20_ReadROM(DS18B20_HandleTypeDef *hds, uint8_t *rom);
```
Reads the unique 64-bit ROM code of the sensor. Use this once per sensor to discover its address.

> ⚠️ Only works when **one sensor is on the bus**. Connect sensors one by one, read and note each ROM code, then hardcode them in your project.

---

### `DS18B20_SearchROM()`
```c
DS18B20_Status_t DS18B20_SearchROM(DS18B20_HandleTypeDef *hds, uint8_t rom_codes[][8], uint8_t max, uint8_t *found);
```
Automatically discovers all DS18B20 sensors on the bus. Use when sensor count is unknown or sensors can change at runtime. No need to hardcode ROM codes.

| Parameter | Description |
|-----------|-------------|
| `hds` | Pointer to DS18B20 handle |
| `rom_codes` | Output: 2D array to store found ROM codes `[max][8]` |
| `max` | Size of `rom_codes` array — limits how many sensors to search for |
| `found` | Output: number of sensors actually found |

```c
uint8_t rom_codes[10][8];
uint8_t found = 0;

DS18B20_SearchROM(&hds18b20, rom_codes, 10, &found);
// found now contains the number of sensors discovered
// rom_codes[0..found-1] contain their ROM codes
```

---

### `DS18B20_MatchROM()`
```c
DS18B20_Status_t DS18B20_MatchROM(DS18B20_HandleTypeDef *hds, uint8_t *rom);
```
Addresses one specific sensor by its ROM code, ignoring all others on the bus. Must be followed immediately by `DS18B20_ReadTemperature()`.

---

### `DS18B20_GetTemperatureByROM()` ⭐ Use this for a specific sensor
```c
DS18B20_Status_t DS18B20_GetTemperatureByROM(DS18B20_HandleTypeDef *hds, uint8_t *rom, float *temperature);
```
Triggers conversion on one specific sensor and reads its temperature. Handles the full sequence internally — no need to call `MatchROM` or `ReadTemperature` separately.

| Parameter | Description |
|-----------|-------------|
| `hds` | Pointer to DS18B20 handle |
| `rom` | 8-byte ROM code of the target sensor |
| `temperature` | Output: temperature in °C |

---

## Multi-Sensor Pattern

When reading multiple sensors, trigger conversion once for all then read each individually. This is more efficient than converting each sensor separately.

```c
// Convert all sensors simultaneously (Skip ROM broadcasts to everyone)
DS18B20_StartConversion(&hds18b20);
HAL_Delay(DS18B20_CONV_TIME_12BIT);

// Read each sensor individually by ROM code
DS18B20_MatchROM(&hds18b20, sensor1_rom);
DS18B20_ReadTemperature(&hds18b20, &temp1);

DS18B20_MatchROM(&hds18b20, sensor2_rom);
DS18B20_ReadTemperature(&hds18b20, &temp2);
```

Total wait = 1 × conversion time regardless of sensor count.

---

## Resolution Options

| Constant | Value | Precision | Conversion Time |
|----------|-------|-----------|-----------------|
| `DS18B20_RES_9BIT` | `0x1F` | 0.5 °C | 94 ms |
| `DS18B20_RES_10BIT` | `0x3F` | 0.25 °C | 188 ms |
| `DS18B20_RES_11BIT` | `0x5F` | 0.125 °C | 375 ms |
| `DS18B20_RES_12BIT` | `0x7F` | 0.0625 °C | 750 ms *(default)* |

Higher resolution = more accurate but slower.

---

## Return Codes

| Code | Value | Meaning |
|------|-------|---------|
| `DS18B20_OK` | 0 | Success |
| `DS18B20_ERR_NO_DEVICE` | -1 | Sensor not found / not connected |
| `DS18B20_ERR_TIMEOUT` | -2 | UART didn't respond in time |
| `DS18B20_ERR_CRC` | -3 | Data corrupted during transfer |
| `DS18B20_ERR_DMA` | -4 | DMA transfer failed |
| `DS18B20_ERR_RANGE` | -5 | Temperature outside -55…+125 °C |

Always check the return value — it tells you exactly what went wrong.

---

## Troubleshooting

| Symptom | Likely Cause |
|---------|-------------|
| Always returns `ERR_NO_DEVICE` | Missing 4.7kΩ pull-up, wrong pin, or sensor not powered |
| Always returns `ERR_TIMEOUT` | DMA not enabled in CubeMX, or callbacks not wired up |
| Always returns `ERR_CRC` | Noisy wiring, missing pull-up, or long wire without proper shielding |
| Temperature reads `0.0` | DMA callbacks not implemented — `ReadByte` is timing out silently |
| Works once then stops | UART not re-initializing baud rate correctly — check HAL half-duplex config |
| `SearchROM` returns `ERR_NO_DEVICE` after first round | UART buffer not flushed — leftover bytes from bit walk confuse presence detection |
| Both `bit` and `bit_cmp` read as 1 in `SearchROM` | Delay inside bit loop causes sensor timeout — no delays allowed during 64-bit walk |
| Parasitic mode — temperature garbage or CRC errors | Strong pull-up GPIO required during conversion — connect VDD to 3.3V instead |

---

## Examples

| File | What it covers |
|------|---------------|
| `1_single_sensor.c` | Basic temperature read — start here |
| `2_set_resolution.c` | Change resolution, saved to EEPROM |
| `3_nonblocking.c` | `StartConversion` + `ReadTemperature` separately, CPU free during conversion |
| `4_read_rom.c` | Get ROM code of one sensor, prints C array format ready to paste |
| `5_search_rom.c` | Auto-discover all sensors on bus, print ROM codes |
| `6_multi_hardcoded.c` | Multiple sensors with known ROM codes, efficient convert-all-read-each |
| `7_multi_dynamic.c` | Multiple sensors, `SearchROM` at startup, no hardcoding |
| `8_get_temperature_by_rom.c` | Read one specific sensor by ROM code |
| `9_power_supply.c` | Detect parasitic vs external power |

## Example

See [`./examples`](./examples) for a complete working example.
