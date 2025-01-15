#ifndef TB_NVS_H
#define TB_NVS_H

#include <stdbool.h>
#include "esp_err.h"
#include "tb.h"

#define TB_NVS_NAMESPACE_NAME "tb"
#define TB_NVS_TOKEN_KEY "token"
#define TB_NVS_TOKEN_MIN_LENGTH 2
#define TB_NVS_TOKEN_MAX_LENGTH 33

#include <assert.h>
#include "nvs_flash.h"

esp_err_t tb_nvs_get_token(char *token, size_t *token_length);
{
    assert(token);
    assert(token_length);

    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

    if (token_length < TB_NVS_TOKEN_MIN_LENGTH || token_length > TB_NVS_TOKEN_MAX_LENGTH)
    {
        err = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }

    err = nvs_open(TB_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = nvs_get_str(nvs, TB_NVS_TOKEN_KEY, token, token_length);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (nvs != 0)
    {
        nvs_close(nvs);
        nvs = 0;
    }

    return err;
}

esp_err_t tb_nvs_set_token(const char *token)
{
    assert(token);

    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

    size_t token_length = strlen(token) + 1;
    if (token_length < TB_NVS_TOKEN_MIN_LENGTH || token_length > TB_NVS_TOKEN_MAX_LENGTH)
    {
        err = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }

    err = nvs_open(TB_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = nvs_set_str(nvs, TB_NVS_TOKEN_KEY, token);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (nvs != 0)
    {
        nvs_close(nvs);
        nvs = 0;
    }

    return err;
}

esp_err_t tb_nvs_has_token(bool *has_token)
{
    assert(has_token);

    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

    err = nvs_open(TB_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    size_t token_length = 0;
    err = nvs_get_str(nvs, TB_NVS_TOKEN_KEY, NULL, &token_length);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        goto cleanup;
    }

    if (err == ESP_OK)
    {

        if (token_length < TB_NVS_TOKEN_MIN_LENGTH || token_length > TB_NVS_TOKEN_MAX_LENGTH)
        {
            err = ESP_ERR_INVALID_SIZE;
            goto cleanup;
        }

        *has_token = true;
    }
    else
    {
        *has_token = false;
    }

cleanup:
    if (nvs != 0)
    {
        nvs_close(nvs);
        nvs = 0;
    }

    return err;
}

#endif
