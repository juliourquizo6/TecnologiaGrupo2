#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"

#define ACCESS_TOKEN_LEN_MAX 32

ESP_EVENT_DECLARE_BASE(TB_EVENTS);

enum
{
    TB_EVENT_ATTRIBUTES,
};

typedef struct thingsboard thingsboard;

esp_err_t tb_create(thingsboard *tb, const char *host, const char *topic, char *cert_pem, esp_event_handler_t event_handler, void *event_handler_arg);

esp_err_t tb_delete(thingsboard *tb);

esp_err_t tb_send(const thingsboard *tb, const char *data);

struct thingsboard
{
    char *host;
    char *topic;
    char *cert_pem;
    char access_token[ACCESS_TOKEN_LEN_MAX + 1];
    esp_event_loop_handle_t event_loop;
    esp_mqtt_client_handle_t mqtt_client;
};
