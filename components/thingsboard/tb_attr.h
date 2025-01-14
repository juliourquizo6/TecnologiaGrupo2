#ifndef TB_ATTR_H
#define TB_ATTR_H

#include "tb.h"

#define TB_ATTR_SHARED_SUBSCRIBE_TOPIC "v1/devices/me/attributes"
#define TB_ATTR_REQUEST_TOPIC "v1/devices/me/attributes/request/0"
#define TB_ATTR_RESPONSE_TOPIC "v1/devices/me/attributes/response/0"
#define TB_ATTR_REQUEST_DATA "{\"keys\":[]}"

static void tb_subscribe_to_shared_attributes_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err = ESP_OK;

    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_DATA:
    {
        if (!tb_is_event_from_topic(event, TB_ATTR_SHARED_SUBSCRIBE_TOPIC))
        {
            break;
        }

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_SHARED_ATTRIBUTE_EVENT, event->data, event->data_len, TB_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

    cleanup:
        break;
    }
    }
}

esp_err_t tb_subscribe_to_shared_attributes(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_register_event(tb->client, MQTT_EVENT_DATA, tb_subscribe_to_shared_attributes_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (esp_mqtt_client_subscribe(tb->client, TB_ATTR_SHARED_SUBSCRIBE_TOPIC, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

cleanup:
    if (err != ESP_OK)
    {
        esp_mqtt_client_unsubscribe(tb->client, TB_ATTR_SHARED_SUBSCRIBE_TOPIC);
        esp_mqtt_client_unregister_event(tb->client, MQTT_EVENT_DATA, tb_subscribe_to_shared_attributes_handler);
    }

    return err;
}

static void tb_request_attributes_handler(const char *attribute_key, int32_t attribute_event_id, void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err = ESP_OK;

    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_DATA:
    {
        if (!tb_is_event_from_topic(event, TB_ATTR_RESPONSE_TOPIC))
        {
            break;
        }

        cJSON *data_json = NULL;
        char *data = NULL;

        data_json = cJSON_Parse(event->data);
        if (!data)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        cJSON *attribute_json = cJSON_GetObjectItem(data_json, attribute_key);
        if (!attribute_json)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        data = cJSON_Print(attribute_json);
        if (!data)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, attribute_event_id, data, strlen(data), TB_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

    cleanup:
        if (data)
        {
            free(data);
            data = NULL;
        }

        if (data_json)
        {
            cJSON_Delete(data_json);
            data_json = NULL;
        }

        tb_notify_error(tb, err);

        break;
    }
    }
}

static void tb_request_shared_attributes_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_request_attributes_handler("shared", TB_SHARED_ATTRIBUTE_EVENT, event_handler_arg, event_base, event_id, event_data);
}

static void tb_request_client_attributes_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_request_attributes_handler("client", TB_CLIENT_ATTRIBUTE_EVENT, event_handler_arg, event_base, event_id, event_data);
}

static esp_err_t tb_request_attributes(thingsboard *tb, esp_event_handler_t request_attributes_handler)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_register_event(tb->client, MQTT_EVENT_DATA, request_attributes_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    if (esp_mqtt_client_subscribe(tb->client, TB_ATTR_RESPONSE_TOPIC, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    tb_reset_notification(tb);

    if (esp_mqtt_client_publish(tb->client, TB_ATTR_REQUEST_TOPIC, TB_ATTR_REQUEST_DATA, 0, 0, 0) < 0)
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
    esp_mqtt_client_unsubscribe(tb->client, TB_ATTR_RESPONSE_TOPIC);
    esp_mqtt_client_unregister_event(tb->client, MQTT_EVENT_DATA, tb_request_shared_attributes_handler);

    return err;
}

esp_err_t tb_request_shared_attributes(thingsboard *tb)
{
    return tb_request_attributes(tb, tb_request_shared_attributes_handler);
}

esp_err_t tb_request_client_attributes(thingsboard *tb)
{
    return tb_request_attributes(tb, tb_request_client_attributes_handler);
}

esp_err_t tb_subscribe_to_and_request_shared_attributes(thingsboard *tb, bool *requested_shared_attributes)
{
    assert(tb);
    assert(requested_shared_attributes);

    esp_err_t err = ESP_OK;

    err = tb_subscribe_to_shared_attributes(tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = tb_request_shared_attributes(tb);

    if (err == ESP_OK)
    {
        *requested_shared_attributes = true;
    }
    else
    {
        *requested_shared_attributes = false;
    }

cleanup:
    return err;
}

#endif
