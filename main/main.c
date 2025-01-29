#include "cJSON.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "sensor_co2.h"
#include "tb_client.h"
#include "wifi_component.h"
#include "power_manager.h"

static void attributes_callback(cJSON *attributes)
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

static void mysensor_callback(const cJSON *data_sensor_co2)
{
    tb_client_send_telemetry(data_sensor_co2);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //power_manager_init();

    provision_and_connect();
    tb_client_init(thingsboard_url, CONFIG_TECNOLOGIA_TOPIC, attributes_callback);
    sensor_co2_create(mysensor_callback);
}
