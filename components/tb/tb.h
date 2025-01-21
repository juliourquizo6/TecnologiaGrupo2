#include "cJSON.h"
#include "esp_err.h"

esp_err_t tb_client_init(const char *hostname, const char *mqtt_client_certificate, const char *http_client_certificate, const char *telemetry_topic, void (*attributes_callback)(const cJSON *));

esp_err_t tb_client_send(const cJSON *data);
