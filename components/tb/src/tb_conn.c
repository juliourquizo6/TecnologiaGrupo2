#include <assert.h>
#include "esp_event.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "tb/tb_prov.h"
#include "tb/tb_nvs.h"
#include "tb/tb_util.h"
#include "tb/tb_conn.h"

static void tb_conn_connect_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void tb_conn_connect_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        esp_err_t err = ESP_OK;

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_CONNECTED_EVENT, NULL, 0, TB_UTIL_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup_connected;
        }

    cleanup_connected:
        tb_util_notify(tb, err);

        break;
    }

    case MQTT_EVENT_DISCONNECTED:
    {
        esp_err_t err = ESP_OK;

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_DISCONNECTED_EVENT, NULL, 0, TB_UTIL_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup_disconnected;
        }

    cleanup_disconnected:
        break;
    }

    default:
    {
        break;
    }
    }
}

esp_err_t tb_conn_connect(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    char token[TB_NVS_TOKEN_LENGTH_MAX] = CONFIG_TB_TOKEN;
    if (*token == '\0')
    {
        size_t token_lenght = sizeof(token);
        err = tb_prov_try_to_provision_and_get_token(tb, token, &token_lenght);
        if (err != ESP_OK)
        {
            goto cleanup;
        }
    }

    tb_conn_disconnect(tb);

    err = tb_set_mqtt_config_with_token(tb, token);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb->client, ESP_EVENT_ANY_ID, tb_conn_connect_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    tb_util_clear_notification(tb);

    err = esp_mqtt_client_start(tb->client);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = tb_util_wait_for_notification(tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (err != ESP_OK && tb->client)
    {
        esp_mqtt_client_destroy(tb->client);
        tb->client = NULL;
    }

    return err;
}

esp_err_t tb_conn_disconnect(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_stop(tb->client);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}
