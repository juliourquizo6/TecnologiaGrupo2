#ifndef TB_NVS_H
#define TB_NVS_H

#include "tb.h"

#define TB_NVS_NAMESPACE "tb"
#define TB_NVS_TOKEN_KEY "token"
#define TB_NVS_TOKEN_MIN_LENGTH 1
#define TB_NVS_TOKEN_MAX_LENGTH 32

esp_err_t tb_read_token_from_nvs(char *token)
{
    assert(token);

    esp_err_t err = ESP_OK;

    nvs_handle_t nvs = 0;

    err = nvs_open(TB_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    size_t token_length = 0;
    err = nvs_get_str(nvs, TB_NVS_TOKEN_KEY, token, &token_length);
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

esp_err_t tb_write_token_to_nvs(const char *token)
{
    assert(token);

    esp_err_t err = ESP_OK;

    nvs_handle_t nvs = 0;

    size_t token_length = strlen(token);
    if (token_length < TB_NVS_TOKEN_MIN_LENGTH || token_length > TB_NVS_TOKEN_MAX_LENGTH)
    {
        err = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }

    err = nvs_open(TB_NVS_NAMESPACE, NVS_READWRITE, &nvs);
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

esp_err_t tb_is_device_provisioned(bool *device_provisioned)
{
    assert(device_provisioned);

    esp_err_t err = ESP_OK;

    nvs_handle_t nvs = 0;

    err = nvs_open(TB_NVS_NAMESPACE, NVS_READWRITE, &nvs);
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
        *device_provisioned = true;
    }
    else
    {
        *device_provisioned = false;
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
