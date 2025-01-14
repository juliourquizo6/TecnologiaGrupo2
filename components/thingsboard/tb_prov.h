#ifndef TB_PROV_H
#define TB_PROV_H

#include "tb.h"

#define TB_PROV_REQUEST_TOPIC "/provision/request"
#define TB_PROV_RESPONSE_TOPIC "/provision/response"
#define TB_PROV_REQUEST_DATA "{\"provisionDeviceKey\":\"" CONFIG_TB_PROV_DEVICE_KEY "\",\"provisionDeviceSecret\":\"" CONFIG_TB_PROV_DEVICE_SECRET "\"}"

static void tb_provision_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err = ESP_OK;

    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        if (esp_mqtt_client_subscribe(tb->client, TB_PROV_RESPONSE_TOPIC, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (esp_mqtt_client_publish(tb->client, TB_PROV_REQUEST_TOPIC, TB_PROV_REQUEST_DATA, 0, 0, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

    cleanup:
        if (err != ESP_OK)
        {
            tb_notify_error(tb, err);
        }

        break;
    }

    case MQTT_EVENT_DATA:
    {
        if (!tb_is_event_from_topic(event, TB_PROV_RESPONSE_TOPIC))
        {
            break;
        }

        cJSON *data_json = NULL;

        data_json = cJSON_Parse(event->data);
        if (!data_json)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        cJSON *status_json = cJSON_GetObjectItem(data_json, "status");
        if (!status_json)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (strcmp(status_json->valuestring, "SUCCESS") != 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        cJSON *token_json = cJSON_GetObjectItem(data_json, "credentialsValue");
        if (!token_json)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        err = tb_write_token_to_nvs(token_json->valuestring);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

    cleanup:
        if (data_json)
        {
            cJSON_Delete(data_json);
        }

        tb_notify_error(tb, err);

        break;
    }

    default:
    {
        break;
    }
    }
}

esp_err_t tb_provision(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    esp_mqtt_client_destroy(tb->client);
    tb->client = NULL;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = tb->host,
        .broker.address.port = tb->port,
        .broker.address.transport = tb->cert_pem ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
        .credentials.username = "provision",
        .broker.verification.certificate = tb->cert_pem,
    };
    tb->client = esp_mqtt_client_init(&mqtt_cfg);
    if (!tb->client)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb->client, ESP_EVENT_ANY_ID, tb_provision_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (esp_mqtt_client_subscribe(tb->client, TB_PROV_RESPONSE_TOPIC, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    tb_reset_notification(tb);

    if (esp_mqtt_client_publish(tb->client, TB_PROV_REQUEST_TOPIC, TB_PROV_REQUEST_DATA, 0, 0, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = tb_wait_for_notify_error(tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (tb->client)
    {
        esp_mqtt_client_destroy(tb->client);
        tb->client = NULL;
    }

    return err;
}

esp_err_t tb_try_provision_and_read_token_from_nvs(thingsboard *tb, char *token)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    bool provisioned = false;
    err = tb_is_device_provisioned(&provisioned);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (!provisioned)
    {
        err = tb_provision(tb);
        if (err != ESP_OK)
        {
            goto cleanup;
        }
    }

    err = tb_read_token_from_nvs(token);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}

#endif
