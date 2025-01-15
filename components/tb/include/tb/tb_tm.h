#ifndef TB_TM_H
#define TB_TM_H

#include "esp_err.h"
#include "cJSON.h"
#include "tb/tb.h"

#define TB_TM_TOPIC "v1/devices/me/telemetry"

esp_err_t tb_tm_create_telemetry_topic_from_null_terminated_subtopics(char **telemetry_topic, ...);
esp_err_t tb_tm_send_telemetry(thingsboard *tb, const char *data);
esp_err_t tb_tm_send_telemetry_json(thingsboard *tb, const cJSON *data_json);

#endif
