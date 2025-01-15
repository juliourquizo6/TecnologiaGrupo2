#ifndef TB_TB_H
#define TB_TB_H

#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"

#define TB_DEFAULT_HOST "demo.thingsboard.io"
#define TB_DEFAULT_PORT 1883
#define TB_DEFAULT_ENCRYPTED_PORT 8883
#define TB_DEFAULT_TELEMETRY_TOPIC "v1/devices/me/telemetry"

ESP_EVENT_DECLARE_BASE(TB_EVENTS);

enum
{
    TB_CONNECTED_EVENT,
    TB_DISCONNECTED_EVENT,
    TB_SHARED_ATTRIBUTE_EVENT,
    TB_CLIENT_ATTRIBUTE_EVENT,
};

typedef struct thingsboard thingsboard;

void tb_destroy(thingsboard *tb);
esp_err_t tb_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *host, uint32_t port, const char *telemetry_topic, const char *cert_pem);

struct thingsboard
{
    esp_mqtt_client_handle_t client;
    esp_event_loop_handle_t event_loop;
    TaskHandle_t task;
    char *host;
    uint32_t port;
    char *telemetry_topic;
    char *cert_pem;
};

#endif
