#ifndef TB_TM_H
#define TB_TM_H

#include "cJSON.h"
#include "esp_err.h"
#include "tb.h"

#define TB_TM_TOPIC "v1/devices/me/telemetry"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client.h"

esp_err_t tb_tm_create_telemetry_topic_from_null_terminated_subtopics(char **telemetry_topic, ...)
{
    assert(telemetry_topic);

    esp_err_t err = ESP_OK;

    va_list args;
    va_start(args, telemetry_topic);

    int telemetry_topic_length = strlen(TB_TM_TOPIC);
    while (true)
    {
        const char *arg = va_arg(args, const char *);
        if (!arg)
        {
            break;
        }

        telemetry_topic_length += strlen("/");
        telemetry_topic_length += strlen(arg);
    }

    va_end(args);

    telemetry_topic = (char *)malloc(telemetry_topic_length + 1);
    if (!telemetry_topic)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    va_start(args, out_telemetry_topic);

    strcpy(telemetry_topic, TB_TM_TOPIC);
    while (true)
    {
        const char *arg = va_arg(args, const char *);
        if (!arg)
        {
            break;
        }

        strcat(telemetry_topic, "/");
        strcat(telemetry_topic, arg);
    }

    va_end(args);

cleanup:
    if (err != ESP_OK && telemetry_topic)
    {
        free(telemetry_topic);
    }

    return err;
}

esp_err_t tb_tm_send_telemetry(thingsboard *tb, const char *data)
{
    assert(tb);
    assert(data);

    esp_err_t err = ESP_OK;

    if (esp_mqtt_client_publish(tb->client, tb->telemetry_topic, data, 0, 0, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

cleanup:
    return err;
}

esp_err_t tb_tm_send_telemetry_json(thingsboard *tb, const cJSON *data_json)
{
    assert(tb);
    assert(data_json);

    char *data = NULL;

    esp_err_t err = ESP_OK;

    data = cJSON_Print(data_json);
    if (!data)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = tb_send_telemetry(tb, data);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (data)
    {
        free(data);
    }

    return err;
}

#endif
