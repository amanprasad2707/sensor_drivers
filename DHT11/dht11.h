#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"
#include "stm32f407xx.h"

typedef enum {
  DHT11_OK = 0,
  DHT11_ERROR = 1,
  DHT11_TIMEOUT = 2,
  DHT11_CHECKSUM_ERROR = 3
} dht11_status_t;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  uint8_t temperature_int;
  uint8_t temperature_dec;
  uint8_t humidity_int;
  uint8_t humidity_dec;
} DHT11_HandleTypeDef;


typedef enum{
  GPIO_PIN_MODE_INPUT = 0,
  GPIO_PIN_MODE_OUTPUT = 1
}GPIO_pin_mode_t;

/**
 * @brief Initializes the DHT11 sensor handle.
 *
 * @param[in] dht11 Pointer to DHT11 handle structure.
 * @param[in] port GPIO port for the DHT11 data pin.
 * @param[in] pin GPIO pin for the DHT11 data pin.
 * @return dht11_status_t Returns DHT11_OK on success.
 */
dht11_status_t dht11_init(DHT11_HandleTypeDef *dht11, GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief Reads temperature and humidity from DHT11 sensor.
 *
 * @param[in] dht11 Pointer to DHT11 handle structure.
 * @return dht11_status_t Returns DHT11_OK on success, error code on failure.
 */
dht11_status_t dht11_read_data(DHT11_HandleTypeDef *dht11);

/**
 * @brief Gets the temperature value from last successful read.
 *
 * @param[in] dht11 Pointer to DHT11 handle structure.
 * @param[out] temperature Pointer to store temperature in Celsius.
 * @return dht11_status_t Returns DHT11_OK on success.
 */
dht11_status_t dht11_get_temperature(DHT11_HandleTypeDef *dht11, float *temperature);

/**
 * @brief Gets the humidity value from last successful read.
 *
 * @param[in] dht11 Pointer to DHT11 handle structure.
 * @param[out] humidity Pointer to store humidity in percentage.
 * @return dht11_status_t Returns DHT11_OK on success.
 */
dht11_status_t dht11_get_humidity(DHT11_HandleTypeDef *dht11, uint8_t *humidity);

#endif /* INC_DHT11_H_ */