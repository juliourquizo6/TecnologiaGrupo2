#include "cJSON.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "sensor_co2.h"
#include "tb_client.h"

static const char *host = NULL;
static const char *topic = NULL;

static void sgp30_handler(cJSON *data_sensor_co2)
{
    tb_client_send_telemetry(data_sensor_co2);
}

static void attributes_callback(const cJSON *attributes)
{
    cJSON *sampling_rate = cJSON_GetObjectItem(attributes, "samplingRate");

    if (!cJSON_IsNumber(sampling_rate))
    {
        return;
    }

    int hz = sampling_rate->valueint;
    int ms = 1000 / hz;

    sensor_co2_set_frecuencia_lectura(ms);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    tb_client_init(host, topic, attributes_callback);
    sensor_co2_create(sgp30_handler);
}
