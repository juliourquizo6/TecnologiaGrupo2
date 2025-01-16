#ifndef TB_ATTR_H
#define TB_ATTR_H

#include "esp_err.h"
#include "tb/tb.h"

#define TB_ATTR_SHARED_SUBSCRIBE_TOPIC "v1/devices/me/attributes"
#define TB_ATTR_REQUEST_TOPIC "v1/devices/me/attributes/request/0"
#define TB_ATTR_RESPONSE_TOPIC "v1/devices/me/attributes/response/0"
#define TB_ATTR_REQUEST_DATA "{\"keys\":[]}"

esp_err_t tb_attr_subscribe_to_shared(thingsboard *tb);
esp_err_t tb_attr_unsubscribe_from_shared(const thingsboard *tb);
esp_err_t tb_attr_request_shared(thingsboard *tb);
esp_err_t tb_attr_request_client(thingsboard *tb);
esp_err_t tb_attr_subscribe_to_and_request_shared(thingsboard *tb);

#endif
