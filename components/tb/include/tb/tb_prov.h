#ifndef TB_PROV_H
#define TB_PROV_H

#include "esp_err.h"
#include "sdkconfig.h"
#include "tb/tb.h"

#define TB_PROV_REQUEST_TOPIC "/provision/request"
#define TB_PROV_RESPONSE_TOPIC "/provision/response"
#define TB_PROV_REQUEST_DATA "{\"provisionDeviceKey\":\"" CONFIG_TB_DEVICE_KEY "\",\"provisionDeviceSecret\":\"" CONFIG_TB_DEVICE_SECRET "\"}"

esp_err_t tb_prov_provision(thingsboard *tb);
esp_err_t tb_prov_try_to_provision_and_get_token(thingsboard *tb, char *token, size_t token_length);

#endif
