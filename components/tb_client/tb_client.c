#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "tb_client.h"

typedef struct tb_client tb_client_t;

typedef struct tb_client *tb_client_handle_t;

struct tb_client
{
    char *hostname;
    char *telemetry_topic;
    void (*attributes_callback)(cJSON *);
    char *access_token;
    esp_mqtt_client_handle_t mqtt_client_handle;
};

static char *TAG = "tb_client";

static tb_client_handle_t s_tb_client_handle = NULL;

static void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t tb_client_init(const char *hostname, const char *telemetry_topic, void (*attributes_callback)(cJSON *))
{
    if (!hostname)
    {
        hostname = "demo.thingsboard.io";
    }

    if (!telemetry_topic)
    {
        telemetry_topic = "v1/devices/me/telemetry";
    }

    tb_client_handle_t tb_client_handle = NULL;

    esp_err_t err = ESP_OK;

    if (s_tb_client_handle)
    {
        err = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    static tb_client_t s_tb_client = {0};

    tb_client_handle = &s_tb_client;

    tb_client_handle->hostname = strdup(hostname);
    if (!tb_client_handle->hostname)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    tb_client_handle->telemetry_topic = strdup(telemetry_topic);
    if (!tb_client_handle->telemetry_topic)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    tb_client_handle->attributes_callback = attributes_callback;

    tb_client_handle->access_token = NULL;

    esp_mqtt_client_config_t mqtt_client_config = {0};

    tb_client_handle->mqtt_client_handle = esp_mqtt_client_init(&mqtt_client_config);
    if (!tb_client_handle->mqtt_client_handle)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb_client_handle->mqtt_client_handle, MQTT_EVENT_ANY, tb_mqtt_event_handler, tb_client_handle);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_start(tb_client_handle->mqtt_client_handle);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    s_tb_client_handle = tb_client_handle;
    tb_client_handle = NULL;

    ESP_LOGI(TAG, "Initialized ThingsBoard client");

cleanup:
    if (tb_client_handle)
    {
        if (tb_client_handle->mqtt_client_handle)
        {
            esp_mqtt_client_destroy(tb_client_handle->mqtt_client_handle);
            tb_client_handle->mqtt_client_handle = NULL;
        }

        if (tb_client_handle->access_token)
        {
            free(tb_client_handle->access_token);
            tb_client_handle->access_token = NULL;
        }

        if (tb_client_handle->attributes_callback)
        {
            tb_client_handle->attributes_callback = NULL;
        }

        if (tb_client_handle->telemetry_topic)
        {
            free(tb_client_handle->telemetry_topic);
            tb_client_handle->telemetry_topic = NULL;
        }

        if (tb_client_handle->hostname)
        {
            free(tb_client_handle->hostname);
            tb_client_handle->hostname = NULL;
        }

        tb_client_handle = NULL;
    }

    return err;
}

void tb_client_send_telemetry(const cJSON *telemetry)
{
    tb_client_handle_t tb_client_handle = s_tb_client_handle;

    char *telemetry_data = NULL;

    esp_err_t err = ESP_OK;

    if (!tb_client_handle)
    {
        err = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    telemetry_data = cJSON_PrintUnformatted(telemetry);
    if (!telemetry_data)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, telemetry_data, 0, 0, 0);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    ESP_LOGI(TAG, "Sent telemetry: %s", telemetry_data);

cleanup:
    if (telemetry_data)
    {
        cJSON_free(telemetry_data);
        telemetry_data = NULL;
    }
}

void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_client_handle_t tb_client_handle = (tb_client_handle_t)event_handler_arg;
    esp_mqtt_event_handle_t mqtt_event_handle = (esp_mqtt_event_handle_t)event_data;

    nvs_handle_t nvs_handle = 0;
    char *access_token = NULL;
    char *mqtt_client_uri = NULL;
    cJSON *root = NULL;
    char *http_client_url = NULL;
    char *user_event_data = NULL;

    esp_err_t err = ESP_OK;

    switch (event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
    {
        ESP_LOGI(TAG, "Connecting to ThingsBoard");

        err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

        if (!tb_client_handle->access_token)
        {
            err = nvs_open("tb_client", NVS_READONLY, &nvs_handle);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
            {
                goto cleanup;
            }

            if (nvs_handle)
            {
                size_t access_token_len = 0;

                err = nvs_get_str(nvs_handle, "access_token", NULL, &access_token_len);
                if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
                {
                    goto cleanup;
                }

                if (access_token_len > 0)
                {
                    access_token = (char *)malloc(access_token_len);
                    if (!access_token)
                    {
                        err = ESP_ERR_NO_MEM;
                        goto cleanup;
                    }

                    *access_token = '\0';

                    err = nvs_get_str(nvs_handle, "access_token", access_token, &access_token_len);
                    if (err != ESP_OK)
                    {
                        goto cleanup;
                    }

                    tb_client_handle->access_token = access_token;
                    access_token = NULL;

                    ESP_LOGI(TAG, "Restored access token: %s", tb_client_handle->access_token);
                }
            }
        }

        int mqtt_client_uri_len = snprintf(NULL, 0, "mqtt://%s", tb_client_handle->hostname);
        if (mqtt_client_uri_len < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        mqtt_client_uri = (char *)malloc(mqtt_client_uri_len + 1);
        if (!mqtt_client_uri)
        {
            err = ESP_ERR_NO_MEM;
            goto cleanup;
        }

        *mqtt_client_uri = '\0';

        if (snprintf(mqtt_client_uri, mqtt_client_uri_len + 1, "mqtt://%s", tb_client_handle->hostname) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        esp_mqtt_client_config_t mqtt_client_config = {
            .broker.address.uri = mqtt_client_uri,
            .credentials.username = (tb_client_handle->access_token) ? tb_client_handle->access_token : "provision",
        };

        err = esp_mqtt_set_config(tb_client_handle->mqtt_client_handle, &mqtt_client_config);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

        ESP_LOGI(TAG, "Using ThingsBoard hostname %s and username %s", mqtt_client_config.broker.address.uri, mqtt_client_config.credentials.username);

        break;
    }

    case MQTT_EVENT_CONNECTED:
    {
        ESP_LOGI(TAG, "Connected to ThingsBoard");

        if (tb_client_handle->access_token)
        {
            ESP_LOGI(TAG, "Listening for attributes");

            if (esp_mqtt_client_subscribe(tb_client_handle->mqtt_client_handle, "v1/devices/me/attributes", 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (esp_mqtt_client_subscribe(tb_client_handle->mqtt_client_handle, "v1/devices/me/attributes/response/0", 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, "v1/devices/me/attributes/request/0", "{}", 0, 0, 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }
        }
        else
        {
            ESP_LOGI(TAG, "Provisioning device");

            if (esp_mqtt_client_subscribe(tb_client_handle->mqtt_client_handle, "/provision/response", 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, "/provision/request", "{\"deviceName\":\"" CONFIG_TB_DEVICE_NAME "\",\"provisionDeviceKey\":\"" CONFIG_TB_PROVISION_DEVICE_KEY "\",\"provisionDeviceSecret\":\"" CONFIG_TB_PROVISION_DEVICE_SECRET "\"}", 0, 0, 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }
        }

        break;
    }

    case MQTT_EVENT_DATA:
    {
        ESP_LOGI(TAG, "Received data %.*s on topic %.*s", mqtt_event_handle->data_len, mqtt_event_handle->data, mqtt_event_handle->topic_len, mqtt_event_handle->topic);

        if (tb_client_handle->access_token)
        {
            cJSON *attributes = NULL;

            if (strncmp(mqtt_event_handle->topic, "v1/devices/me/attributes", mqtt_event_handle->topic_len) == 0)
            {
                ESP_LOGI(TAG, "Received attribute update");

                root = cJSON_ParseWithLength(mqtt_event_handle->data, mqtt_event_handle->data_len);
                if (!root)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                attributes = root;
            }
            else if (strncmp(mqtt_event_handle->topic, "v1/devices/me/attributes/response/0", mqtt_event_handle->topic_len) == 0)
            {
                ESP_LOGI(TAG, "Received attribute response");

                root = cJSON_ParseWithLength(mqtt_event_handle->data, mqtt_event_handle->data_len);
                if (!root)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                attributes = cJSON_GetObjectItem(root, "shared");
            }
            else
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (!attributes)
            {
                break;
            }

            cJSON *fw_title = cJSON_GetObjectItem(attributes, "fw_title");

            cJSON *fw_version = cJSON_GetObjectItem(attributes, "fw_version");

            if (fw_title && fw_version)
            {
                char *fw_title_data = cJSON_GetStringValue(fw_title);
                if (!fw_title_data)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                char *fw_version_data = cJSON_GetStringValue(fw_version);
                if (!fw_version_data)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                ESP_LOGI(TAG, "Received firmware title %s and version %s", fw_title_data, fw_version_data);

                const esp_app_desc_t *app_desc = esp_app_get_description();

                const char *current_fw_title_data = app_desc->project_name;

                const char *current_fw_version_data = app_desc->version;

                ESP_LOGI(TAG, "Current firmware title %s and version %s", current_fw_title_data, current_fw_version_data);

                int current_fw_data_len = snprintf(NULL, 0, "{\"current_fw_title\":\"%s\",\"current_fw_version\":\"%s\"}", current_fw_title_data, current_fw_version_data);
                if (current_fw_data_len < 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                char *current_fw_data = (char *)malloc(current_fw_data_len + 1);
                if (!current_fw_data)
                {
                    err = ESP_ERR_NO_MEM;
                    goto cleanup;
                }

                *current_fw_data = '\0';

                if (snprintf(current_fw_data, current_fw_data_len + 1, "{\"current_fw_title\":\"%s\",\"current_fw_version\":\"%s\"}", current_fw_title_data, current_fw_version_data) < 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, current_fw_data, 0, 0, 0) < 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (strcmp(fw_title_data, current_fw_title_data) != 0 || strcmp(fw_version_data, current_fw_version_data) != 0)
                {
                    ESP_LOGI(TAG, "Downloading firmware");

                    err = esp_wifi_set_ps(WIFI_PS_NONE);
                    if (err != ESP_OK)
                    {
                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"DOWNLOADING\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    int http_client_url_len = snprintf(NULL, 0, "https://%s/api/v1/%s/firmware?title=%s&version=%s", tb_client_handle->hostname, tb_client_handle->access_token, fw_title_data, fw_version_data);
                    if (http_client_url_len < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    http_client_url = (char *)malloc(http_client_url_len + 1);
                    if (!http_client_url)
                    {
                        err = ESP_ERR_NO_MEM;
                        goto cleanup;
                    }

                    *http_client_url = '\0';

                    if (snprintf(http_client_url, http_client_url_len + 1, "https://%s/api/v1/%s/firmware?title=%s&version=%s", tb_client_handle->hostname, tb_client_handle->access_token, fw_title_data, fw_version_data) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    esp_http_client_config_t http_client_config = {
                        .crt_bundle_attach = esp_crt_bundle_attach,
                        .url = http_client_url,
                    };

                    esp_https_ota_config_t https_ota_config = {
                        .http_config = &http_client_config,
                    };

                    err = esp_https_ota(&https_ota_config);
                    if (err != ESP_OK)
                    {
                        if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"FAILED\"}", 0, 0, 0) < 0)
                        {
                            err = ESP_FAIL;
                            goto cleanup;
                        }

                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"DOWNLOADED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"VERIFIED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"UPDATING\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    ESP_LOGI(TAG, "Restarting in device");

                    vTaskDelay(pdMS_TO_TICKS(1000));

                    esp_restart();
                }
                else
                {
                    ESP_LOGI(TAG, "Firmware is up to date");

                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"UPDATED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }
                }
            }

            if (tb_client_handle->attributes_callback)
            {
                tb_client_handle->attributes_callback(attributes);
            }
        }
        else
        {
            if (strncmp(mqtt_event_handle->topic, "/provision/response", mqtt_event_handle->topic_len) == 0)
            {
                ESP_LOGI(TAG, "Received provisioning response");

                root = cJSON_ParseWithLength(mqtt_event_handle->data, mqtt_event_handle->data_len);
                if (!root)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                cJSON *status = cJSON_GetObjectItem(root, "status");
                if (!status)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                char *status_data = cJSON_GetStringValue(status);
                if (!status_data)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (strcmp(status_data, "SUCCESS") != 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                cJSON *credentials_type = cJSON_GetObjectItem(root, "credentialsType");
                if (!credentials_type)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                char *credentials_type_data = cJSON_GetStringValue(credentials_type);
                if (!credentials_type_data)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (strcmp(credentials_type_data, "ACCESS_TOKEN") != 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                cJSON *credentials_value = cJSON_GetObjectItem(root, "credentialsValue");
                if (!credentials_value)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                char *credentials_value_data = cJSON_GetStringValue(credentials_value);
                if (!credentials_value_data)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                ESP_LOGI(TAG, "Received access token: %s", credentials_value_data);

                err = nvs_open("tb_client", NVS_READWRITE, &nvs_handle);
                if (err != ESP_OK)
                {
                    goto cleanup;
                }

                err = nvs_set_str(nvs_handle, "access_token", credentials_value_data);
                if (err != ESP_OK)
                {
                    goto cleanup;
                }

                err = nvs_commit(nvs_handle);
                if (err != ESP_OK)
                {
                    goto cleanup;
                }

                ESP_LOGI(TAG, "Reconnected to ThingsBoard with access token");

                esp_mqtt_client_config_t mqtt_client_config = {
                    .network.reconnect_timeout_ms = 1,
                };

                err = esp_mqtt_set_config(tb_client_handle->mqtt_client_handle, &mqtt_client_config);
                if (err != ESP_OK)
                {
                    goto cleanup;
                }

                err = esp_mqtt_client_disconnect(tb_client_handle->mqtt_client_handle);
                if (err != ESP_OK)
                {
                    goto cleanup;
                }
            }
            else
            {
                err = ESP_FAIL;
                goto cleanup;
            }
        }

        break;
    }

    default:
    {
        break;
    }
    }

cleanup:
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error %s, disconnecting and reconnecting to ThingsBoard", esp_err_to_name(err));

        esp_mqtt_client_disconnect(tb_client_handle->mqtt_client_handle);
    }

    if (user_event_data)
    {
        cJSON_free(user_event_data);
        user_event_data = NULL;
    }

    if (http_client_url)
    {
        free(http_client_url);
        http_client_url = NULL;
    }

    if (root)
    {
        cJSON_Delete(root);
        root = NULL;
    }

    if (mqtt_client_uri)
    {
        free(mqtt_client_uri);
        mqtt_client_uri = NULL;
    }

    if (access_token)
    {
        free(access_token);
        access_token = NULL;
    }

    if (nvs_handle != 0)
    {
        nvs_close(nvs_handle);
        nvs_handle = 0;
    }
}
