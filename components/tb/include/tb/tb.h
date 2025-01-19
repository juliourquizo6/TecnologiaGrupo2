#ifndef TB_H
#define TB_H

#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"

#define TB_DEFAULT_HOSTNAME "demo.thingsboard.io"
#define TB_DEFAULT_PORT 1883
#define TB_DEFAULT_PORT_ENCRYPTED 8883
#define TB_DEFAULT_TOPIC "v1/devices/me/telemetry"

ESP_EVENT_DECLARE_BASE(TB_EVENTS);

enum
{
    TB_EVENT_CONNECTED,
    TB_EVENT_DISCONNECTED,
    TB_EVENT_SHARED_ATTRIBUTE,
    TB_EVENT_CLIENT_ATTRIBUTE,
};

typedef struct thingsboard thingsboard;

esp_err_t tb_init(thingsboard *tb, esp_event_loop_handle_t event_loop, const char *hostname, uint32_t port, const char *topic, const char *certificate);
void tb_destroy(thingsboard *tb);

struct thingsboard
{
    esp_event_loop_handle_t event_loop;
    // TODO: must use URI (not hostname and port)
    // Also, dont set transport (TCP, SSL), it is set by the uri
    char *hostname;
    uint32_t port;
    char *topic;
    char *certificate;
    esp_mqtt_client_handle_t client;
    // TODO: better to use event group because task notification may trigger spuriously
    // Don't use direct to task notifications as light weight event group alternative
    // because of conflicts with calling blocking functions on multiple tasks
    TaskHandle_t task;
};

#endif
