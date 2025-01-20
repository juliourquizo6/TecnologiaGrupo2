#include "esp_err.h"

struct data_sensor_co2
{
    uint16_t TVOC;
    uint16_t CO2;
};

typedef void (*sensor_co2_handler)(struct data_sensor_co2);

void sensor_co2_create(sensor_co2_handler handler);
void sensor_co2_set_frecuencia_lectura(int frec_ms);
void sensor_co2_set_frecuencia_envio(int frec_ms);

