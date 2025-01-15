#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "esp_event.h"
#include "mqtt_client.h"
#include "tb/tb_util.h"
#include "tb/tb.h"

ESP_EVENT_DEFINE_BASE(TB_EVENTS);

void tb_destroy(thingsboard *tb)
{
    assert(tb);

    tb->event_loop = NULL;

    free(tb->host);
    tb->host = NULL;

    tb->port = 0;

    free(tb->topic);
    tb->topic = NULL;

    free(tb->cert_pem);
    tb->cert_pem = NULL;

    if (tb->client)
    {
        esp_mqtt_client_destroy(tb->client);
        tb->client = NULL;
    }

    tb->task = NULL;
}

esp_err_t tb_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *host, uint32_t port, const char *topic, const char *cert_pem)
{
    if (!host)
    {
        host = TB_DEFAULT_HOST;
    }

    if (port == 0)
    {
        if (cert_pem)
        {
            port = TB_DEFAULT_ENCRYPTED_PORT;
        }
        else
        {
            port = TB_DEFAULT_PORT;
        }
    }

    if (!topic)
    {
        topic = TB_DEFAULT_TELEMETRY_TOPIC;
    }

    assert(tb);
    assert(event_loop);
    assert(host);
    assert(port != 0);
    assert(topic);

    esp_err_t err = ESP_OK;

    memset(tb, 0, sizeof(*tb));

    tb->event_loop = event_loop;

    tb->host = (char *)malloc(strlen(host) + 1);
    if (!tb->host)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    strcpy(tb->host, host);

    tb->port = port;

    tb->topic = (char *)malloc(strlen(topic) + 1);
    if (!tb->topic)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    strcpy(tb->topic, topic);

    if (cert_pem)
    {
        tb->cert_pem = (char *)malloc(strlen(cert_pem) + 1);
        if (!tb->cert_pem)
        {
            err = ESP_ERR_NO_MEM;
            goto cleanup;
        }
        strcpy(tb->cert_pem, cert_pem);
    }
    else
    {
        tb->cert_pem = NULL;
    }

    esp_mqtt_client_config_t mqtt_cfg = {0};
    tb->client = esp_mqtt_client_init(&mqtt_cfg);

    tb_util_clear_notification(tb);

cleanup:
    if (err != ESP_OK)
    {
        tb_destroy(tb);
    }

    return err;
}
