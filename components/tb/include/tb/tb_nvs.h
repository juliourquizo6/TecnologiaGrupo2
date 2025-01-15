#ifndef TB_NVS_H
#define TB_NVS_H

#include <stdbool.h>
#include "esp_err.h"

#define TB_NVS_NAMESPACE_NAME "tb"
#define TB_NVS_TOKEN_KEY "token"
#define TB_NVS_TOKEN_LENGTH_MIN 2
#define TB_NVS_TOKEN_LENGTH_MAX 33

esp_err_t tb_nvs_get_token(char *token, size_t *token_length);
esp_err_t tb_nvs_set_token(const char *token);
esp_err_t tb_nvs_has_token(bool *has_token);

#endif
