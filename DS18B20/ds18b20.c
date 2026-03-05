/**
  ******************************************************************************
  * @file    ds18b20.c
  * @brief   DS18B20 1-Wire temperature sensor library (UART half-duplex)
  ******************************************************************************
  */

#include "ds18b20.h"
#include <string.h>

/* Private constants ---------------------------------------------------------*/
#define BAUD_RESET   9600U
#define BAUD_DATA    115200U

#define RESET_BYTE   0xF0U   /* Sent at 9600 baud to generate reset pulse   */
#define BIT_1        0xFFU   /* Sent at 115200 baud to write/read a '1'     */
#define BIT_0        0x00U   /* Sent at 115200 baud to write a '0'          */

#define RX_TIMEOUT_MS   1000U
#define DMA_TIMEOUT_MS  500U

/* Private helpers -----------------------------------------------------------*/

/**
 * @brief  Re-initialize UART at the requested baud rate (half-duplex).
 */
static void uart_set_baud(DS18B20_HandleTypeDef *hds, uint32_t baud){
    hds->huart->Init.BaudRate = baud;
    HAL_HalfDuplex_Init(hds->huart);   /* Re-init; keeps all other settings */
}

/**
 * @brief  Compute Dallas/Maxim 1-Wire CRC-8 over a byte array.
 *         Polynomial: X^8 + X^5 + X^4 + 1  (0x31 reflected = 0x8C)
 */
static uint8_t onewire_crc8(const uint8_t *data, uint8_t len){
    uint8_t crc = 0;

    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {

            uint8_t incoming = byte & 1;
            uint8_t remainder = crc & 1;

            crc >>= 1;

            if (incoming ^ remainder) {
                crc ^= 0x8C;
            }

            byte >>= 1;
        }
    }
    return crc;
}


/**
 * @brief  Read a single bit from the 1-Wire bus (used for Search ROM).
 */
static uint8_t DS18B20_ReadBit(DS18B20_HandleTypeDef *hds) {
    uint8_t tx_data = BIT_1;
    uint8_t rx_data = 0;
    HAL_UART_Transmit(hds->huart, &tx_data, 1, 10);
    HAL_UART_Receive(hds->huart, &rx_data, 1, 10);
    return (rx_data == 0xFF) ? 1 : 0;
}

/**
 * @brief  Write a single bit to the 1-Wire bus (used for Search ROM).
 */
static void DS18B20_WriteBit(DS18B20_HandleTypeDef *hds, uint8_t bit) {
    uint8_t tx_data = (bit ? BIT_1 : BIT_0);
    uint8_t rx_data = 0;
    HAL_UART_Transmit(hds->huart, &tx_data, 1, 10);
    HAL_UART_Receive(hds->huart, &rx_data, 1, 10); /* Flush echo */
}


/* Public API ----------------------------------------------------------------*/

void DS18B20_Init(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart){
    hds->huart      = huart;
    hds->resolution = DS18B20_RES_12BIT;
    hds->rxDone     = 0;
}


/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_Reset(DS18B20_HandleTypeDef *hds){
    uint8_t data = RESET_BYTE;

    uart_set_baud(hds, BAUD_RESET);

    /* Send reset pulse */
    HAL_UART_Transmit(hds->huart, &data, 1, 100);

    /* Read presence pulse */
    HAL_StatusTypeDef status = HAL_UART_Receive(hds->huart, &data, 1, RX_TIMEOUT_MS);
    uart_set_baud(hds, BAUD_DATA);

    if (status != HAL_OK) {
        return DS18B20_ERR_TIMEOUT;
    }
    if (data == RESET_BYTE) {
        return DS18B20_ERR_NO_DEVICE;   /* Line stayed high — no device */
    }
    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
void DS18B20_WriteByte(DS18B20_HandleTypeDef *hds, uint8_t data){
    uint8_t buffer[8];

    for (int i = 0; i < 8; i++) {
        /* LSB first: each bit becomes a full UART byte */
        buffer[i] = ((data >> i) & 0x01) ? BIT_1 : BIT_0;
    }
    HAL_UART_Transmit(hds->huart, buffer, 8, 1000);
}

/* -------------------------------------------------------------------------- */
uint8_t DS18B20_ReadByte(DS18B20_HandleTypeDef *hds){
    uint8_t tx_buf[8];
    uint8_t value = 0;

    /* Fill TX with 0xFF to release the bus so the sensor can drive it */
    memset(tx_buf, BIT_1, sizeof(tx_buf));

    hds->rxDone = 0;

    /* Start DMA TX then DMA RX — in half-duplex the peripheral echoes
     * what was actually on the bus (sensor can pull it low to send a 0) */
    if (HAL_UART_Receive_DMA(hds->huart, hds->rxData, 8) != HAL_OK) return 0;
    if (HAL_UART_Transmit_DMA(hds->huart, tx_buf, 8) != HAL_OK) {
        HAL_UART_AbortReceive(hds->huart);
        return 0;
    }

    /* Wait for RX complete (set by callback) with timeout */
    uint32_t tickstart = HAL_GetTick();
    while (hds->rxDone == 0) {
        if ((HAL_GetTick() - tickstart) > DMA_TIMEOUT_MS) {
            HAL_UART_AbortReceive(hds->huart);
            HAL_UART_AbortTransmit(hds->huart);
            return 0;
        }
    }
    if (hds->rxDone < 0) return 0;   /* DMA error */

    /* Decode: 0xFF on bus = bit 1, anything else = bit 0 */
    for (int i = 0; i < 8; i++) {
        if (hds->rxData[i] == 0xFF) {
            value |= (1 << i);
        }
    }
    return value;
}

/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_StartConversion(DS18B20_HandleTypeDef *hds){
    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(hds, DS18B20_CMD_CONVERT_T);
    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_ReadTemperature(DS18B20_HandleTypeDef *hds, float *temperature){
    uint8_t scratchpad[9];

    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(hds, DS18B20_CMD_READ_SCRATCHPAD);

    /* Read all 9 scratchpad bytes (byte 8 is CRC) */
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = DS18B20_ReadByte(hds);
    }

    /* Validate CRC */
    if (onewire_crc8(scratchpad, 8) != scratchpad[8]) {
        return DS18B20_ERR_CRC;
    }

    /* Combine LSB (byte 0) and MSB (byte 1) */
    int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);

    /* DS18B20: 1 LSB = 0.0625°C */
    float temp_c = (float)raw / 16.0f;

    /* Sanity check: valid range is -55 to +125 °C */
    if (temp_c < -55.0f || temp_c > 125.0f) {
        return DS18B20_ERR_RANGE;
    }

    *temperature = temp_c;
    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_GetTemperature(DS18B20_HandleTypeDef *hds, float *temperature){
    DS18B20_Status_t ret;

    /* Step 1: trigger conversion */
    ret = DS18B20_StartConversion(hds);
    if (ret != DS18B20_OK) return ret;

    /* Step 2: wait for conversion to finish based on current resolution */
    switch (hds->resolution) {
        case DS18B20_RES_9BIT:  HAL_Delay(DS18B20_CONV_TIME_9BIT);  break;
        case DS18B20_RES_10BIT: HAL_Delay(DS18B20_CONV_TIME_10BIT); break;
        case DS18B20_RES_11BIT: HAL_Delay(DS18B20_CONV_TIME_11BIT); break;
        default:                HAL_Delay(DS18B20_CONV_TIME_12BIT); break;
    }

    /* Step 3: read result */
    ret = DS18B20_ReadTemperature(hds, temperature);
    return ret;
}


/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_SetResolution(DS18B20_HandleTypeDef *hds, uint8_t res){
    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(hds, DS18B20_CMD_WRITE_SCRATCHPAD);
    DS18B20_WriteByte(hds, 0x00);  /* TH register (alarm high — unused) */
    DS18B20_WriteByte(hds, 0x00);  /* TL register (alarm low  — unused) */
    DS18B20_WriteByte(hds, res);   /* Configuration register            */

    /* Copy scratchpad to EEPROM so setting survives power-off */
    ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(hds, DS18B20_CMD_COPY_SCRATCHPAD);
    HAL_Delay(10);   /* EEPROM write time ~10ms */

    hds->resolution = res;
    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_ReadPowerSupply(DS18B20_HandleTypeDef *hds, uint8_t *parasitic) {
    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(hds, DS18B20_CMD_READ_POWER);

    // Sensor pulls line LOW = parasitic powered (0), releases = external VCC (1)
    *parasitic = (DS18B20_ReadByte(hds) == 0x00) ? 0 : 1;
    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_ReadROM(DS18B20_HandleTypeDef *hds, uint8_t *rom){
    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_READ_ROM);  // 0x33

    for (int i = 0; i < 8; i++) {
        rom[i] = DS18B20_ReadByte(hds);
    }

    // Validate CRC
    if (onewire_crc8(rom, 7) != rom[7]) {
        return DS18B20_ERR_CRC;
    }

    return DS18B20_OK;
}


/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_MatchROM(DS18B20_HandleTypeDef *hds, uint8_t *rom){
    DS18B20_Status_t ret = DS18B20_Reset(hds);
    if (ret != DS18B20_OK) return ret;
    
    DS18B20_WriteByte(hds, DS18B20_CMD_MATCH_ROM);  // 0x55
    
    // Send all 8 bytes of the ROM code
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteByte(hds, rom[i]);
    }
    
    return DS18B20_OK;
}


/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_GetTemperatureByROM(DS18B20_HandleTypeDef *hds, uint8_t *rom, float *temperature){
    // Target specific sensor for conversion
    DS18B20_Status_t ret = DS18B20_MatchROM(hds, rom);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_CONVERT_T);

    switch (hds->resolution) {
        case DS18B20_RES_9BIT:  HAL_Delay(DS18B20_CONV_TIME_9BIT);  break;
        case DS18B20_RES_10BIT: HAL_Delay(DS18B20_CONV_TIME_10BIT); break;
        case DS18B20_RES_11BIT: HAL_Delay(DS18B20_CONV_TIME_11BIT); break;
        default:                HAL_Delay(DS18B20_CONV_TIME_12BIT); break;
    }

    // Target same sensor again for reading
    ret = DS18B20_MatchROM(hds, rom);
    if (ret != DS18B20_OK) return ret;

    DS18B20_WriteByte(hds, DS18B20_CMD_READ_SCRATCHPAD);

    uint8_t scratchpad[9];
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = DS18B20_ReadByte(hds);
    }

    if (onewire_crc8(scratchpad, 8) != scratchpad[8]) {
        return DS18B20_ERR_CRC;
    }

    int16_t raw = (int16_t)((scratchpad[1] << 8) | scratchpad[0]);
    *temperature = (float)raw / 16.0f;
    return DS18B20_OK;
}


/* -------------------------------------------------------------------------- */
DS18B20_Status_t DS18B20_SearchROM(DS18B20_HandleTypeDef *hds, uint8_t rom_codes[][8], uint8_t max, uint8_t *found){
    *found = 0;
    uint8_t last_discrepancy = 0;    // remembers where we branched last time
    uint8_t rom_buf[8];
    uint8_t search_done = 0;

    memset(rom_buf, 0, sizeof(rom_buf));

    while (!search_done && *found < max){
        DS18B20_Status_t ret = DS18B20_Reset(hds);
        if (ret != DS18B20_OK) return ret;

        DS18B20_WriteByte(hds, DS18B20_CMD_SEARCH_ROM);   /* 0xF0 */
        
        /* Flush RX buffer to remove echoes from WriteByte */
        uint8_t flush;
        while(HAL_UART_Receive(hds->huart, &flush, 1, 1) == HAL_OK);

        uint8_t new_discrepancy = 0;

        /* Walk all 64 bit positions */
        for (int bit_pos = 1; bit_pos <= 64; bit_pos++){
            /* Read two bits: the bit and its complement from all devices */
            uint8_t bit     = DS18B20_ReadBit(hds);
            uint8_t bit_cmp = DS18B20_ReadBit(hds);

            if (bit == 1 && bit_cmp == 1) {
                /* Both 1 — no devices responded, bus error */
                return DS18B20_ERR_NO_DEVICE;
            }

            uint8_t chosen;

            if (bit != bit_cmp) {
                /* All devices agree on this bit — no choice needed */
                chosen = bit;
            } else {
                /* Discrepancy — devices have both 0 and 1 at this position */
                if (bit_pos < last_discrepancy) {
                    /* Follow same path as last run */
                    chosen = (rom_buf[(bit_pos - 1) / 8] >> ((bit_pos - 1) % 8)) & 0x01;
                } else if (bit_pos == last_discrepancy) {
                    chosen = 1;   /* At last branch: now take the 1 path */
                } else {
                    chosen = 0;   /* Beyond last branch: always take 0 first */
                    new_discrepancy = bit_pos;
                }
            }

            /* Write chosen bit into rom_buf */
            if (chosen) {
                rom_buf[(bit_pos - 1) / 8] |=  (1 << ((bit_pos - 1) % 8));
            } else {
                rom_buf[(bit_pos - 1) / 8] &= ~(1 << ((bit_pos - 1) % 8));
            }

            /* Send chosen bit to all devices — non-chosen devices drop off */
            DS18B20_WriteBit(hds, chosen);
        }

        /* Validate CRC of discovered ROM */
        if (onewire_crc8(rom_buf, 7) != rom_buf[7]) {
            return DS18B20_ERR_CRC;
        }

        /* Store discovered ROM code */
        memcpy(rom_codes[*found], rom_buf, 8);
        (*found)++;

        last_discrepancy = new_discrepancy;

        /* No new discrepancy means this was the last device */
        if (last_discrepancy == 0) {
            search_done = 1;
        }
    }

    return DS18B20_OK;
}

/* -------------------------------------------------------------------------- */
void DS18B20_RxCpltCallback(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart){
    if (huart->Instance == hds->huart->Instance) {
        hds->rxDone = 1;
    }
}

/* -------------------------------------------------------------------------- */
void DS18B20_ErrorCallback(DS18B20_HandleTypeDef *hds, UART_HandleTypeDef *huart){
    if (huart->Instance == hds->huart->Instance) {
        hds->rxDone = -1;
    }
}
