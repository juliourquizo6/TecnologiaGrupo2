#include "cJSON.h"
#include "esp_err.h"

esp_err_t tb_client_init(const char *hostname, const char *certificate, const char *telemetry_topic, void (*attributes_callback)(const cJSON *));

esp_err_t tb_client_send(const char *data);
