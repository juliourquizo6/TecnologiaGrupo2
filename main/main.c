#include "cJSON.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "sensor_co2.h"
#include "tb_client.h"
#include "wifi_component.h"

static void attributes_callback(const cJSON *attributes)
{
    cJSON *sampling_rate = cJSON_GetObjectItem(attributes, "samplingRate");
    if (!sampling_rate)
    {
        return;
    }

    if (!cJSON_IsNumber(sampling_rate))
    {
        return;
    }

    int sampling_rate_hz = sampling_rate->valueint;

    int sampling_rate_ms = 1000 / sampling_rate_hz;

    sensor_co2_set_frecuencia_lectura(sampling_rate_ms);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    provision_and_connect();
    // TODO
    const char *topic = NULL;
    tb_client_init(thingsboard_url, topic, attributes_callback);
    sensor_co2_create(tb_client_send_telemetry);
}
