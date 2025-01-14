#ifndef TB_UTIL_H
#define TB_UTIL_H

#include "tb.h"

#define TB_MIN(a, b) ((a) > (b) ? (b) : (a))
#define TB_MAX(a, b) ((a) > (b) ? (a) : (b))

bool tb_is_event_from_topic(esp_mqtt_event_handle_t event, const char *topic)
{
    assert(event);
    assert(topic);

    return event->topic_len == strlen(topic) && strncmp(event->topic, topic, event->topic_len) == 0;
}

void tb_reset_notification(thingsboard *tb)
{
    assert(tb);

    tb->task = xTaskGetCurrentTaskHandle();

    xTaskNotifyStateClear(tb->task);
}

esp_err_t tb_wait_for_notify_error(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    if (xTaskNotifyWait(0, 0xFFFFFFFF, &err, TB_TIMEOUT_TICKS) == pdFALSE)
    {
        err = ESP_ERR_TIMEOUT;
        goto cleanup;
    }

cleanup:
    return err;
}

void tb_notify_error(thingsboard *tb, esp_err_t err)
{
    assert(tb);

    xTaskNotify(tb->task, (uint32_t)err, eSetValueWithOverwrite);
}

#endif
