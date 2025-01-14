#ifndef TB_H
#define TB_H

#include <assert.h>
#include <cJSON.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <mqtt_client.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "sdkconfig.h"
#include "tb_conf.h"
#include "tb_attr.h"
#include "tb_conn.h"
#include "tb_nvs.h"
#include "tb_ota.h"
#include "tb_prov.h"
#include "tb_tm.h"
#include "tb_util.h"

ESP_EVENT_DECLARE_BASE(TB_EVENTS);
ESP_EVENT_DEFINE_BASE(TB_EVENTS);

enum
{
    TB_DISCONNECTED_EVENT,
    TB_SHARED_ATTRIBUTE_EVENT,
    TB_CLIENT_ATTRIBUTE_EVENT,
};

#define TB_EVENT_LOOP_QUEUE_SIZE 10
#define TB_EVENT_LOOP_STACK_SIZE 2048
#define TB_TIMEOUT_TICKS pdMS_TO_TICKS(CONFIG_TB_TIMEOUT_MS)
#define TB_DEFAULT_HOST "demo.thingsboard.io"
#define TB_DEFAULT_PORT 1883
#define TB_DEFAULT_ENCRYPTED_PORT 8883
#define TB_DEFAULT_TELEMETRY_TOPIC "v1/devices/me/telemetry"

typedef struct thingsboard thingsboard;

struct thingsboard
{
    TaskHandle_t task;
    esp_mqtt_client_handle_t client;
    esp_event_loop_handle_t event_loop;
    char *host;
    uint32_t port;
    char *telemetry_topic;
    char *cert_pem;
};

esp_err_t thingsboard_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *host, uint32_t port, const char *telemetry_topic, const char *cert_pem)
{
    assert(tb);
    assert(event_loop);

    esp_err_t err = ESP_OK;

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

    tb->task = xTaskGetCurrentTaskHandle();

    tb->client = NULL;

    tb->event_loop = event_loop;

    tb->host = (char *)malloc(strlen(host) + 1);
    if (!tb->host)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    strcpy(tb->host, host);

    tb->port = (char *)malloc(strlen(port) + 1);
    if (!tb->port)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    strcpy(tb->port, port);

    tb->telemetry_topic = (char *)malloc(strlen(telemetry_topic) + 1);
    if (!tb->telemetry_topic)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    strcpy(tb->telemetry_topic, telemetry_topic);

    tb->cert_pem = (char *)malloc(strlen(cert_pem) + 1);
    if (!tb->cert_pem)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    strcpy(tb->cert_pem, cert_pem);

cleanup:
    return err;
}

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

    free(tb->telemetry_topic);
}

#endif
