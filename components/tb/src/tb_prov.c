#include <assert.h>
#include <string.h>
#include "cJSON.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "tb/tb_conn.h"
#include "tb/tb_nvs.h"
#include "tb/tb_util.h"
#include "tb/tb_prov.h"

static void tb_prov_provision_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void tb_prov_provision_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const thingsboard *tb = (const thingsboard *)event_handler_arg;
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

    cleanup_connected:
        if (err != ESP_OK)
        {
            tb_util_notify(tb, err);
        }

        break;
    }

    case MQTT_EVENT_SUBSCRIBED:
    {
        if (!tb_util_is_event_from_topic(event, TB_PROV_RESPONSE_TOPIC))
        {
            break;
        }

        esp_err_t err = ESP_OK;

        if (esp_mqtt_client_publish(tb->client, TB_PROV_REQUEST_TOPIC, TB_PROV_REQUEST_DATA, 0, 0, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup_subscribed;
        }

    cleanup_subscribed:
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

        cJSON *credentials_type_json = cJSON_GetObjectItem(data_json, "credentialsType");
        if (!credentials_type_json)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        if (strcmp(credentials_type_json->valuestring, "ACCESS_TOKEN") != 0)
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
            data_json = NULL;
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

    err = tb_set_mqtt_config_with_token(tb, "provision");
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb->client, ESP_EVENT_ANY_ID, tb_prov_provision_handler, (void *)tb);
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
        tb_conn_disconnect(tb);
        goto cleanup;
    }

cleanup:
    return err;
}

esp_err_t tb_prov_try_to_provision_and_get_token(thingsboard *tb, char *token, size_t token_length)
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
