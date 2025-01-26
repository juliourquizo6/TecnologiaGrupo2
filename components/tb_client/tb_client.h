#ifndef TB_CLIENT_H
#define TB_CLIENT_H

#include "cJSON.h"
#include "esp_err.h"

esp_err_t tb_client_init(const char *hostname, const char *telemetry_topic, void (*attributes_callback)(cJSON *));

void tb_client_send_telemetry(const cJSON *telemetry);

#endif
