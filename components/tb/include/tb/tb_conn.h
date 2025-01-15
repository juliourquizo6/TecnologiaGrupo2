#ifndef TB_CONN_H
#define TB_CONN_H

#include "esp_err.h"
#include "tb/tb.h"

esp_err_t tb_conn_connect(thingsboard *tb);
esp_err_t tb_conn_disconnect(thingsboard *tb);

#endif
