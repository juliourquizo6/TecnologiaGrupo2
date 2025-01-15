#include <assert.h>
#include "esp_https_ota.h"
#include "esp_system.h"
#include "tb/tb_ota.h"

static void tb_ota_subscribe_to_update_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t tb_ota_update(const char *url, const char *cert_pem)
{
    assert(cert_pem);

    esp_err_t err = ESP_OK;

    esp_http_client_config_t https_cfg = {
        .url = url,
        .cert_pem = cert_pem,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &https_cfg,
    };
    err = esp_https_ota(&ota_cfg);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    esp_restart();

cleanup:
    return err;
}

static void tb_ota_subscribe_to_update_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_DATA:
    {
    }

    default:
    {
        break;
    }
    }
}

esp_err_t tb_ota_subscribe_to_update(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    return err;
}
