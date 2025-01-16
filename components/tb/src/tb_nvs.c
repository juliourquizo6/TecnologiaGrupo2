#include <assert.h>
#include <string.h>
#include "nvs_flash.h"
#include "tb/tb_nvs.h"

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

esp_err_t tb_nvs_get_token(char *token, size_t token_length)
{
    assert(token);

    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

    err = nvs_open(TB_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

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

    assert(strlen(token) >= TB_CONN_TOKEN_LENGTH_MIN);
    assert(strlen(token) <= TB_CONN_TOKEN_LENGTH_MAX);

    return err;
}

esp_err_t tb_nvs_set_token(const char *token)
{
    assert(token);
    assert(strlen(token) >= TB_CONN_TOKEN_LENGTH_MIN);
    assert(strlen(token) <= TB_CONN_TOKEN_LENGTH_MAX);

    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

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

    err = nvs_commit(nvs);
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

esp_err_t tb_nvs_erase_token(void)
{
    nvs_handle_t nvs = 0;

    esp_err_t err = ESP_OK;

    err = nvs_open(TB_NVS_NAMESPACE_NAME, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = nvs_erase_key(nvs, TB_NVS_TOKEN_KEY);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = nvs_commit(nvs);
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
