#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "tb/tb.h"

ESP_EVENT_DEFINE_BASE(TB_EVENTS);

esp_err_t tb_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *hostname, uint32_t port, const char *topic, const char *certificate)
{
    assert(tb);
    assert(event_loop);

    if (!hostname)
    {
        hostname = TB_DEFAULT_HOSTNAME;
    }

    if (port == 0)
    {
        if (certificate)
        {
            port = TB_DEFAULT_PORT_ENCRYPTED;
        }
        else
        {
            port = TB_DEFAULT_PORT;
        }
    }

    if (!topic)
    {
        topic = TB_DEFAULT_TOPIC;
    }

    esp_err_t err = ESP_OK;

    memset(tb, 0, sizeof(*tb));

    tb->event_loop = event_loop;

    tb->hostname = strdup(hostname);

    tb->port = port;

    tb->topic = strdup(topic);

    tb->certificate = certificate;

    esp_mqtt_client_config_t mqtt_config = {0};
    tb->client = esp_mqtt_client_init(&mqtt_config);
    if (!tb->client)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    tb->task = xTaskGetCurrentTaskHandle();

cleanup:
    if (err != ESP_OK)
    {
        tb_destroy(tb);
    }

    return err;
}

void tb_destroy(thingsboard *tb)
{
    assert(tb);

    tb->event_loop = NULL;

    free(tb->hostname);
    tb->hostname = NULL;

    tb->port = 0;

    free(tb->topic);
    tb->topic = NULL;

    tb->certificate = NULL;

    if (tb->client)
    {
        esp_mqtt_client_destroy(tb->client);
        tb->client = NULL;
    }

    tb->task = NULL;
}
