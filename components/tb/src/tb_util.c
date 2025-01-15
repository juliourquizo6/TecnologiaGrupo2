#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "tb/tb_util.h"

esp_err_t tb_set_mqtt_config_with_token(thingsboard *tb, const char *token)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = tb->host,
        .broker.address.port = tb->port,
        .broker.address.transport = tb->cert_pem ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
        .credentials.username = token,
        .broker.verification.certificate = tb->cert_pem,
    };
    err = esp_mqtt_set_config(tb->client, &mqtt_cfg);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    return err;
}

esp_err_t tb_util_topic_from_topic_levels(const char *topics_levels[], size_t topics_levels_length, char **topic)
{
    assert(topics_levels);
    assert(topic);

    esp_err_t err = ESP_OK;

    size_t topic_length = 0;
    for (size_t i = 0; i < topics_levels_length - 1; ++i)
    {
        topic_length += strlen(topics_levels[i]);
        topic_length += strlen("/");
    }
    topic_length += strlen(topics_levels[topics_levels_length - 1]);

    *topic = (char *)malloc(topic_length + 1);
    if (!*topic)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    for (size_t i = 0; i < topics_levels_length - 1; ++i)
    {
        strcat(*topic, topics_levels[i]);
        strcat(*topic, "/");
    }
    strcat(*topic, topics_levels[topics_levels_length - 1]);

cleanup:
    if (err != ESP_OK && *topic)
    {
        free(*topic);
    }

    return err;
}

bool tb_util_is_event_from_topic(esp_mqtt_event_handle_t event, const char *topic)
{
    assert(event);
    assert(topic);

    return event->topic_len == strlen(topic) && strncmp(event->topic, topic, event->topic_len) == 0;
}

void tb_util_clear_notification(thingsboard *tb)
{
    assert(tb);

    tb->task = xTaskGetCurrentTaskHandle();

    xTaskNotifyStateClear(tb->task);
}

esp_err_t tb_util_wait_for_notification(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    if (xTaskNotifyWait(0, 0xFFFFFFFF, (uint32_t *)&err, TB_UTIL_TIMEOUT_TICKS) == pdFALSE)
    {
        err = ESP_ERR_TIMEOUT;
        goto cleanup;
    }

cleanup:
    return err;
}

void tb_util_notify(thingsboard *tb, esp_err_t err)
{
    assert(tb);

    xTaskNotify(tb->task, (uint32_t)err, eSetValueWithOverwrite);
}
