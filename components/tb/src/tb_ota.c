#include <assert.h>
#include "tb/tb_ota.h"

static void tb_ota_subscribe_to_update(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t tb_ota_update(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

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
    }
}

esp_err_t tb_ota_subscribe_to_update(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

cleanup:
    return err;
}
