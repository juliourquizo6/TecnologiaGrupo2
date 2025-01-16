#ifndef SGP30_SENSOR_H
#define SGP30_SENSOR_H

#include <stdint.h>
#include "esp_err.h"

// Inicializa el sensor SGP30
void sgp30_init(void);

// Lee los valores de CO2 y TVOC del sensor
esp_err_t sgp30_read(uint16_t *co2, uint16_t *tvoc);

#endif // SGP30_SENSOR_H
