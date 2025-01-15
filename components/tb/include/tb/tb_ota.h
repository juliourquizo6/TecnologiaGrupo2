#ifndef TB_OTA_H
#define TB_OTA_H

#include "esp_err.h"
#include "tb/tb.h"

esp_err_t tb_ota_update(thingsboard *tb);
esp_err_t tb_ota_subscribe_to_update(thingsboard *tb);

#endif
