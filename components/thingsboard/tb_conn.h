#ifndef TB_CONN_H
#define TB_CONN_H

#include "tb.h"

static void tb_connect_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err = ESP_OK;

    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        tb_notify_error(tb, err);

        break;
    }

    case MQTT_EVENT_DISCONNECTED:
    {
        err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_DISCONNECTED_EVENT, NULL, 0, TB_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

    cleanup:
        break;
    }

    default:
        break;
    }
}

esp_err_t tb_connect(thingsboard *tb)
{
    esp_err_t err = ESP_OK;

    char token[TB_NVS_TOKEN_MAX_LENGTH + 1] = CONFIG_TB_TOKEN;

    if (*token == '\0')
    {
        err = tb_try_provision_and_read_token_from_nvs(tb, token);
        if (err != ESP_OK)
        {
            goto cleanup;
        }
    }

    esp_mqtt_client_destroy(tb->client);
    tb->client = NULL;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = tb->host,
        .broker.address.port = tb->port,
        .broker.address.transport = tb->cert_pem ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
        .credentials.username = token,
        .broker.verification.certificate = tb->cert_pem,
    };
    tb->client = esp_mqtt_client_init(&mqtt_cfg);
    if (!tb->client)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb->client, ESP_EVENT_ANY_ID, tb_connect_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    tb_reset_notification(tb);

    err = esp_mqtt_client_start(tb->client);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = tb_wait_for_notify_error(tb);
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

#endif
