#ifndef TB_UTIL_H
#define TB_UTIL_H

#include "esp_err.h"
#include "mqtt_client.h"
#include "tb.h"

#define TB_UTIL_TIMEOUT_TICKS pdMS_TO_TICKS(CONFIG_TB_TIMEOUT_MS)

#include <assert.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

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

    if (xTaskNotifyWait(0, 0xFFFFFFFF, &err, TB_UTIL_TIMEOUT_TICKS) == pdFALSE)
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

#endif
