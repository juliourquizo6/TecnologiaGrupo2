#include "cJSON.h"
#include "esp_err.h"

esp_err_t tb_client_init(const char *host, const char *topic, void (*callback)(const cJSON *));

esp_err_t tb_client_send(const cJSON *data);
