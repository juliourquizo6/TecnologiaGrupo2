#ifndef TB_CONN_H
#define TB_CONN_H

#include "esp_err.h"
#include "tb/tb.h"

#define TB_CONN_TOKEN_LENGTH_MIN 1
#define TB_CONN_TOKEN_LENGTH_MAX 32

esp_err_t tb_conn_connect(thingsboard *tb);
esp_err_t tb_conn_reconnect(thingsboard *tb);
esp_err_t tb_conn_disconnect(thingsboard *tb);

#endif
