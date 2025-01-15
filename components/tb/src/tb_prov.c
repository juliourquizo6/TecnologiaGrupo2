#include <assert.h>
#include <string.h>
#include "cJSON.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "tb/tb_conf.h"
#include "tb/tb_nvs.h"
#include "tb/tb_util.h"
#include "tb/tb_prov.h"

static void tb_prov_provision_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void tb_prov_provision_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        esp_err_t err = ESP_OK;

        if (esp_mqtt_client_subscribe(tb->client, TB_PROV_RESPONSE_TOPIC, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup_connected;
        }

        if (esp_mqtt_client_publish(tb->client, TB_PROV_REQUEST_TOPIC, TB_PROV_REQUEST_DATA, 0, 0, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup_connected;
        }

    cleanup_connected:
        if (err != ESP_OK)
        {
            tb_util_notify(tb, err);
        }

        break;
    }

    case MQTT_EVENT_DATA:
    {
        if (!tb_util_is_event_from_topic(event, TB_PROV_RESPONSE_TOPIC))
        {
            break;
        }

        cJSON *data_json = NULL;

        esp_err_t err = ESP_OK;

        data_json = cJSON_Parse(event->data);
        if (!data_json)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        cJSON *status_json = cJSON_GetObjectItem(data_json, "status");
        if (!status_json)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        if (strcmp(status_json->valuestring, "SUCCESS") != 0)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        cJSON *token_json = cJSON_GetObjectItem(data_json, "credentialsValue");
        if (!token_json)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        err = tb_nvs_set_token(token_json->valuestring);
        if (err != ESP_OK)
        {
            goto cleanup_data;
        }

    cleanup_data:
        if (data_json)
        {
            cJSON_Delete(data_json);
        }

        tb_util_notify(tb, err);

        break;
    }

    default:
    {
        break;
    }
    }
}

esp_err_t tb_prov_provision(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    // TODO
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

    err = esp_mqtt_client_register_event(tb->client, ESP_EVENT_ANY_ID, tb_prov_provision_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (esp_mqtt_client_subscribe(tb->client, TB_PROV_RESPONSE_TOPIC, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    tb_util_clear_notification(tb);

    if (esp_mqtt_client_publish(tb->client, TB_PROV_REQUEST_TOPIC, TB_PROV_REQUEST_DATA, 0, 0, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = tb_util_wait_for_notification(tb);
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

esp_err_t tb_prov_try_to_provision_and_get_token(thingsboard *tb, char *token, size_t *token_length)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    bool has_token = false;
    err = tb_nvs_has_token(&has_token);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (!has_token)
    {
        err = tb_prov_provision(tb);
        if (err != ESP_OK)
        {
            goto cleanup;
        }
    }

    err = tb_nvs_get_token(token, token_length);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}
