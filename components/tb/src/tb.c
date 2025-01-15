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

    if (tb->client)
    {
        esp_mqtt_client_destroy(tb->client);
        tb->client = NULL;
    }

    if (tb->event_loop)
    {
        esp_event_loop_delete(tb->event_loop);
        tb->event_loop = NULL;
    }

    free(tb->host);
    tb->host = NULL;

    tb->port = 0;

    free(tb->telemetry_topic);
    tb->telemetry_topic = NULL;

    free(tb->cert_pem);
    tb->cert_pem = NULL;

    tb->task = NULL;
}

esp_err_t tb_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *host, uint32_t port, const char *telemetry_topic, const char *cert_pem)
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

    if (!telemetry_topic)
    {
        telemetry_topic = TB_DEFAULT_TELEMETRY_TOPIC;
    }

    assert(tb);
    assert(event_loop);
    assert(host);
    assert(port != 0);
    assert(telemetry_topic);

    esp_err_t err = ESP_OK;

    memset(tb, 0, sizeof(*tb));

    tb->client = NULL;

    tb->event_loop = event_loop;

    tb_util_clear_notification(tb);

    tb->host = (char *)malloc(strlen(host) + 1);
    if (!tb->host)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    strcpy(tb->host, host);

    tb->port = port;

    tb->telemetry_topic = (char *)malloc(strlen(telemetry_topic) + 1);
    if (!tb->telemetry_topic)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    strcpy(tb->telemetry_topic, telemetry_topic);

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

cleanup:
    if (err != ESP_OK)
    {
        tb_destroy(tb);
    }

    return err;
}
