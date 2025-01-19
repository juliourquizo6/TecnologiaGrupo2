#ifndef TB_TM_H
#define TB_TM_H

#include "esp_err.h"
#include "cJSON.h"
#include "tb/tb.h"

#define TB_TM_TOPIC "v1/devices/me/telemetry"

// TODO: accept topic to send because it may be in shared attributes (after tb_init)
esp_err_t tb_tm_send(const thingsboard *tb, const char *data);
esp_err_t tb_tm_send_json(const thingsboard *tb, const cJSON *data_json);

#endif
