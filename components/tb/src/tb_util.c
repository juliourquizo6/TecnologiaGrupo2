#include <assert.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "tb/tb_util.h"

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
