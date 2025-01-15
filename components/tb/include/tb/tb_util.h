#ifndef TB_UTIL_H
#define TB_UTIL_H

#include "esp_err.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "tb/tb.h"

#define TB_UTIL_TIMEOUT_TICKS pdMS_TO_TICKS(CONFIG_TB_TIMEOUT_MS)

esp_err_t tb_set_mqtt_config_with_token(thingsboard *tb, const char *token);
esp_err_t tb_util_topic_from_topic_levels(const char *topics_levels[], size_t topics_levels_length, char **topic);
bool tb_util_is_event_from_topic(esp_mqtt_event_handle_t event, const char *topic);
void tb_util_clear_notification(thingsboard *tb);
esp_err_t tb_util_wait_for_notification(thingsboard *tb);
void tb_util_notify(thingsboard *tb, esp_err_t err);

#endif
