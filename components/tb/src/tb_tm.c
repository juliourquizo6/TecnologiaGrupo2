#include <assert.h>
#include <stdlib.h>
#include "mqtt_client.h"
#include "tb/tb_tm.h"

esp_err_t tb_tm_send(const thingsboard *tb, const char *data)
{
    assert(tb);
    assert(data);

    esp_err_t err = ESP_OK;

    if (esp_mqtt_client_publish(tb->client, tb->topic, data, 0, 0, 0) < 0)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

cleanup:
    return err;
}

esp_err_t tb_tm_send_json(const thingsboard *tb, const cJSON *data_json)
{
    assert(tb);
    assert(data_json);

    char *data = NULL;

    esp_err_t err = ESP_OK;

    data = cJSON_PrintUnformatted(data_json);
    if (!data)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = tb_tm_send(tb, data);
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
