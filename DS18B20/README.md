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
DS18B20_Init(&hds18b20, &huart1, DS18B20_RES_12BIT);
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
void DS18B20_Init(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart, uint8_t res);
```
Sets up the library. Call this once before anything else.

| Parameter | Description |
|-----------|-------------|
| `hds` | Pointer to your DS18B20 handle |
| `huart` | Pointer to your HAL UART handle (e.g. `&huart1`) |
| `res` | Starting resolution — see table below |

---

### `DS18B20_GetTemperature()` ⭐ Main function
```c
DS18B20_Status_t DS18B20_GetTemperature(DS18B20_HandleTypeDef *hds, float *temperature);
```
Does everything: triggers conversion, waits, reads result. Easiest way to get a temperature reading.

---

### `DS18B20_StartConversion()`
```c
DS18B20_Status_t DS18B20_StartConversion(DS18B20_HandleTypeDef *hds);
```
Tells the sensor to start measuring. **Non-blocking** — you must wait for conversion to finish before reading (see delays below).

---

### `DS18B20_ReadTemperature()`
```c
DS18B20_Status_t DS18B20_ReadTemperature(DS18B20_HandleTypeDef *hds, float *temperature);
```
Reads the scratchpad memory and returns temperature in °C. Call this after `DS18B20_StartConversion()` + delay.

---

### `DS18B20_SetResolution()`
```c
DS18B20_Status_t DS18B20_SetResolution(DS18B20_HandleTypeDef *hds, uint8_t res);
```
Changes sensor resolution and saves it to the sensor's EEPROM (survives power off).

---

### `DS18B20_Reset()`
```c
DS18B20_Status_t DS18B20_Reset(DS18B20_HandleTypeDef *hds);
```
Sends a reset pulse and checks if the sensor is present. Useful for checking if the sensor is connected.

---

### `DS18B20_ReadPowerSupply()`
```c
DS18B20_Status_t DS18B20_ReadPowerSupply(DS18B20_HandleTypeDef *hds, uint8_t *parasitic);
```
Checks how the sensor is being powered.

| `*parasitic` value | Meaning |
|--------------------|---------|
| `0` | Parasitic — sensor powered from data line |
| `1` | External VCC connected |

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

---

## Demo Example File

See [`./examples/demo.c`](./examples/demo.c) for a complete working example.
