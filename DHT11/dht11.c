#include "dht11.h"

/* Add timer handle declaration, defined in main.c */
extern TIM_HandleTypeDef htim7;

/* Private helper functions */
static void delay_us(uint32_t us);
static void set_pin_mode(DHT11_HandleTypeDef *dht, GPIO_pin_mode_t mode);
static void dht11_start_signal(DHT11_HandleTypeDef *dht);
static dht11_status_t dht11_check_response(DHT11_HandleTypeDef *dht);
static uint8_t dht11_read_bit(DHT11_HandleTypeDef *dht);
static uint8_t dht11_read_byte(DHT11_HandleTypeDef *dht);





dht11_status_t dht11_init(DHT11_HandleTypeDef *dht11, GPIO_TypeDef *port, uint16_t pin){
    if (dht11 == NULL){
        return DHT11_ERROR;
    }
    dht11->port = port;
    dht11->pin = pin;
    dht11->temperature_int = 0;
    dht11->temperature_dec = 0;
    dht11->humidity_int = 0;
    dht11->humidity_dec = 0;

    return DHT11_OK;
}




dht11_status_t dht11_read_data(DHT11_HandleTypeDef *dht){
    if (dht == NULL){
        return DHT11_ERROR;
    }

    uint8_t data[5] = {0};
    
    dht11_start_signal(dht);
    
    if(dht11_check_response(dht) != DHT11_OK){
        return DHT11_TIMEOUT;
    
    }

    for (int i = 0; i < 5; i++){
        data[i] = dht11_read_byte(dht);
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)){
        return DHT11_CHECKSUM_ERROR;
    }

    dht->humidity_int    = data[0];
    dht->humidity_dec    = data[1];
    dht->temperature_int = data[2];
    dht->temperature_dec = data[3];

    return DHT11_OK;
}

/**
 * @brief Gets the temperature value from last successful read.
 */
dht11_status_t dht11_get_temperature(DHT11_HandleTypeDef *dht11, float *temperature){
    if (dht11 == NULL || temperature == NULL){
        return DHT11_ERROR;
    }

    *temperature = (float)dht11->temperature_int + ((float)dht11->temperature_dec / 10.0f);

    return DHT11_OK;
}

/**
 * @brief Gets the humidity value from last successful read.
 */
dht11_status_t dht11_get_humidity(DHT11_HandleTypeDef *dht11, uint8_t *humidity){
    if (dht11 == NULL || humidity == NULL){
        return DHT11_ERROR;
    }

    *humidity = dht11->humidity_int;

    return DHT11_OK;
}

/* Private helper functions implementation */

static void set_pin_mode(DHT11_HandleTypeDef *dht, GPIO_pin_mode_t mode){
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = dht->pin;

    if (mode == GPIO_PIN_MODE_OUTPUT){
        gpio.Mode  = GPIO_MODE_OUTPUT_PP;
        gpio.Pull  = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
    }
    else{
        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_PULLUP;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
    }

    HAL_GPIO_Init(dht->port, &gpio);
}



static void delay_us(uint32_t us){
    __HAL_TIM_SET_COUNTER(&htim7, 0);
    while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}



static void dht11_start_signal(DHT11_HandleTypeDef *dht){
    set_pin_mode(dht, GPIO_PIN_MODE_OUTPUT);
    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_RESET);
    HAL_Delay(18);                // ≥18 ms

    HAL_GPIO_WritePin(dht->port, dht->pin, GPIO_PIN_SET);
    delay_us(30);                 // 20–40 µs

    set_pin_mode(dht, GPIO_PIN_MODE_INPUT);
}


static dht11_status_t dht11_check_response(DHT11_HandleTypeDef *dht){
    uint32_t timeout = 1000;

    // Wait LOW (≈80 µs)
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET){
        if (--timeout == 0) return DHT11_TIMEOUT;
        delay_us(1);
    }

    timeout = 1000;
    // Wait HIGH (≈80 µs)
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET){
        if (--timeout == 0) return DHT11_TIMEOUT;
        delay_us(1);
    }

    return DHT11_OK;
}
    

static uint8_t dht11_read_bit(DHT11_HandleTypeDef *dht){
    uint32_t timeout = 1000;

    // Wait LOW
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET){
        if (--timeout == 0) return 0;
        delay_us(1);
    }

    timeout = 1000;
    // Wait HIGH
    while (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_RESET){
        if (--timeout == 0) return 0;
        delay_us(1);
    }

    delay_us(40);   // sample window

    return (HAL_GPIO_ReadPin(dht->port, dht->pin) == GPIO_PIN_SET) ? 1 : 0;
}


static uint8_t dht11_read_byte(DHT11_HandleTypeDef *dht){
    uint8_t value = 0;

    for (int i = 7; i >= 0; i--){
        value |= (dht11_read_bit(dht) << i);
    }

    return value;
}