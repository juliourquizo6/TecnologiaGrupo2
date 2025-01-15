#include <assert.h>
#include "cJSON.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "tb/tb_util.h"
#include "tb/tb_attr.h"

static void tb_attr_subscribe_to_shared_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void tb_attr_request_handler(const char *attribute_key, int32_t attribute_event_id, void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void tb_attr_request_shared_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void tb_attr_request_client_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static esp_err_t tb_attr_unsubscribe_from_request(thingsboard *tb, esp_event_handler_t request_attributes_handler);
static esp_err_t tb_attr_subscribe_to_request(thingsboard *tb, esp_event_handler_t request_attributes_handler);

static void tb_attr_subscribe_to_shared_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_SUBSCRIBED:
    {
        if (!tb_util_is_event_from_topic(event, TB_ATTR_SHARED_SUBSCRIBE_TOPIC))
        {
            break;
        }

        esp_err_t err = ESP_OK;

        tb_util_notify(tb, err);

        break;
    }

    case MQTT_EVENT_DATA:
    {
        if (!tb_util_is_event_from_topic(event, TB_ATTR_SHARED_SUBSCRIBE_TOPIC))
        {
            break;
        }

        esp_err_t err = ESP_OK;

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_SHARED_ATTRIBUTE_EVENT, event->data, event->data_len, TB_UTIL_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup_data;
        }

    cleanup_data:
        break;
    }

    default:
    {
        break;
    }
    }
}

esp_err_t tb_attr_unsubscribe_from_shared(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_unsubscribe(tb->client, TB_ATTR_SHARED_SUBSCRIBE_TOPIC);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_unregister_event(tb->client, MQTT_EVENT_DATA, tb_attr_subscribe_to_shared_handler);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}

esp_err_t tb_attr_subscribe_to_shared(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_register_event(tb->client, MQTT_EVENT_DATA, tb_attr_subscribe_to_shared_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    tb_util_clear_notification(tb);

    if (esp_mqtt_client_subscribe(tb->client, TB_ATTR_SHARED_SUBSCRIBE_TOPIC, 0) < 0)
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
    if (err != ESP_OK)
    {
        tb_attr_unsubscribe_from_shared(tb);
    }

    return err;
}

static void tb_attr_request_handler(const char *attribute_key, int32_t attribute_event_id, void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id)
    {
    case MQTT_EVENT_SUBSCRIBED:
    {
        if (!tb_util_is_event_from_topic(event, TB_ATTR_RESPONSE_TOPIC))
        {
            break;
        }

        esp_err_t err = ESP_OK;

        if (esp_mqtt_client_publish(tb->client, TB_ATTR_REQUEST_TOPIC, TB_ATTR_REQUEST_DATA, 0, 0, 0) < 0)
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
        if (!tb_util_is_event_from_topic(event, TB_ATTR_RESPONSE_TOPIC))
        {
            break;
        }

        cJSON *data_json = NULL;
        char *data = NULL;

        esp_err_t err = ESP_OK;

        data_json = cJSON_Parse(event->data);
        if (!data)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        cJSON *attribute_json = cJSON_GetObjectItem(data_json, attribute_key);
        if (!attribute_json)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        data = cJSON_Print(attribute_json);
        if (!data)
        {
            err = ESP_FAIL;
            goto cleanup_data;
        }

        err = esp_event_post_to(tb->event_loop, TB_EVENTS, attribute_event_id, data, strlen(data), TB_UTIL_TIMEOUT_TICKS);
        if (err != ESP_OK)
        {
            goto cleanup_data;
        }

    cleanup_data:
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

        tb_util_notify(tb, err);

        break;
    }

    default:
    {
        break;
    }
    }
}

static void tb_attr_request_shared_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_attr_request_handler("shared", TB_SHARED_ATTRIBUTE_EVENT, event_handler_arg, event_base, event_id, event_data);
}

static void tb_attr_request_client_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_attr_request_handler("client", TB_CLIENT_ATTRIBUTE_EVENT, event_handler_arg, event_base, event_id, event_data);
}

static esp_err_t tb_attr_unsubscribe_from_request(thingsboard *tb, esp_event_handler_t request_attributes_handler)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_unsubscribe(tb->client, TB_ATTR_RESPONSE_TOPIC);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_unregister_event(tb->client, MQTT_EVENT_DATA, request_attributes_handler);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}

static esp_err_t tb_attr_subscribe_to_request(thingsboard *tb, esp_event_handler_t request_attributes_handler)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = esp_mqtt_client_register_event(tb->client, MQTT_EVENT_DATA, request_attributes_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    tb_util_clear_notification(tb);

    if (esp_mqtt_client_subscribe(tb->client, TB_ATTR_RESPONSE_TOPIC, 0) < 0)
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
    tb_attr_unsubscribe_from_request(tb, request_attributes_handler);

    return err;
}

esp_err_t tb_attr_request_shared(thingsboard *tb)
{
    return tb_attr_subscribe_to_request(tb, tb_attr_request_shared_handler);
}

esp_err_t tb_attr_request_client(thingsboard *tb)
{
    return tb_attr_subscribe_to_request(tb, tb_attr_request_client_handler);
}

esp_err_t tb_attr_subscribe_to_and_request_shared(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    err = tb_attr_subscribe_to_shared(tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = tb_attr_request_shared(tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (err != ESP_OK)
    {
        tb_attr_unsubscribe_from_shared(tb);
    }

    return err;
}
