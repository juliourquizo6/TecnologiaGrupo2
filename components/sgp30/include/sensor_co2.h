#include "esp_err.h"
#include "cJSON.h"

typedef void (*sensor_co2_handler)(const cJSON *data_sensor_co2);

void sensor_co2_create(sensor_co2_handler handler);
void sensor_co2_set_frecuencia_lectura(int frec_ms);
void sensor_co2_set_frecuencia_envio(int frec_ms);
