#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_app_desc.h"
#include "esp_https_ota.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "tb.h"

typedef struct tb_client tb_client_t;

typedef struct tb_client *tb_client_handle_t;

struct tb_client
{
    char *hostname;
    const char *certificate;
    char *telemetry_topic;
    void (*attributes_callback)(const cJSON *);
    char *access_token;
    esp_mqtt_client_handle_t mqtt_client_handle;
};

static tb_client_t s_tb_client = {0};
static tb_client_handle_t s_tb_client_handle = &s_tb_client;

static void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t tb_client_init(const char *hostname, const char *certificate, const char *telemetry_topic, void (*attributes_callback)(const cJSON *))
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

    tb_client_t tb_client = {0};
    tb_client_handle = &tb_client;

    tb_client_handle->hostname = strdup(hostname);

    tb_client_handle->certificate = certificate;

    tb_client_handle->telemetry_topic = strdup(telemetry_topic);

    tb_client_handle->attributes_callback = attributes_callback;

    tb_client_handle->access_token = NULL;

    esp_mqtt_client_config_t mqtt_client_config = {0};

    tb_client_handle->mqtt_client_handle = esp_mqtt_client_init(&mqtt_client_config);
    if (!tb_client_handle->mqtt_client_handle)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb_client_handle->mqtt_client_handle, MQTT_EVENT_ANY, tb_mqtt_event_handler, NULL);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_start(tb_client_handle->mqtt_client_handle);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    *s_tb_client_handle = *tb_client_handle;
    tb_client_handle = NULL;

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

        if (tb_client_handle->certificate)
        {
            tb_client_handle->certificate = NULL;
        }

        if (tb_client_handle->hostname)
        {
            free(tb_client_handle->hostname);
            tb_client_handle->hostname = NULL;
        }
    }

    return err;
}

esp_err_t tb_client_send(const char *data)
{
    tb_client_handle_t tb_client_handle = s_tb_client_handle;

    char *mqtt_event_data = NULL;

    esp_err_t err = ESP_OK;

    if (!data)
    {
        err = ESP_ERR_INVALID_ARG;
        goto cleanup;
    }

    if (!tb_client_handle)
    {
        err = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    mqtt_event_data = (char *)malloc(strlen(data) + 1);
    if (!mqtt_event_data)
    {
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    *mqtt_event_data = '\0';

    strcpy(mqtt_event_data, data);

    esp_mqtt_event_t mqtt_event = {
        .data = mqtt_event_data,
    };

    err = esp_mqtt_dispatch_custom_event(tb_client_handle->mqtt_client_handle, &mqtt_event);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    mqtt_event_data = NULL;

cleanup:
    if (mqtt_event_data)
    {
        free(mqtt_event_data);
        mqtt_event_data = NULL;
    }

    return err;
}

void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    tb_client_handle_t tb_client_handle = s_tb_client_handle;
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
                }
            }
        }

        int mqtt_client_uri_len = snprintf(NULL, 0, "%s://%s", (tb_client_handle->certificate) ? "mqtts" : "mqtt", tb_client_handle->hostname);
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

        if (snprintf(mqtt_client_uri, mqtt_client_uri_len + 1, "%s://%s", (tb_client_handle->certificate) ? "mqtts" : "mqtt", tb_client_handle->hostname) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        esp_mqtt_client_config_t mqtt_client_config = {
            .broker.address.uri = mqtt_client_uri,
            .broker.verification.certificate = tb_client_handle->certificate,
            .credentials.username = (tb_client_handle->access_token) ? tb_client_handle->access_token : "provision",
        };

        err = esp_mqtt_set_config(tb_client_handle->mqtt_client_handle, &mqtt_client_config);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

        break;
    }

    case MQTT_EVENT_CONNECTED:
    {
        if (tb_client_handle->access_token)
        {
            if (esp_mqtt_client_subscribe(tb_client_handle->mqtt_client_handle, "v1/devices/me/attribute", 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (esp_mqtt_client_subscribe(tb_client_handle->mqtt_client_handle, "v1/devices/me/attributes/response/0", 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, "v1/devices/me/attributes/response/0", "{}", 0, 0, 0) < 0)
            {
                err = ESP_FAIL;
                goto cleanup;
            }
        }
        else
        {
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
        if (tb_client_handle->access_token)
        {
            cJSON *attributes = NULL;

            if (strncmp(mqtt_event_handle->topic, "v1/devices/me/attributes", mqtt_event_handle->topic_len) == 0)
            {
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
                root = cJSON_ParseWithLength(mqtt_event_handle->data, mqtt_event_handle->data_len);
                if (!root)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                attributes = cJSON_GetObjectItem(root, "shared");
                if (!attributes)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }
            }
            else
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            if (tb_client_handle->attributes_callback)
            {
                tb_client_handle->attributes_callback(attributes);
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

                const esp_app_desc_t *app_desc = esp_app_get_description();

                const char *current_fw_title_data = app_desc->project_name;

                const char *current_fw_version_data = app_desc->version;

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

                memset(current_fw_data, 0, current_fw_data_len + 1);

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
                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"DOWNLOADING\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    int http_client_url_len = snprintf(NULL, 0, "%s://%s/api/v1/%s/firmware?title=%s&version=%s", (tb_client_handle->certificate) ? "https" : "http", tb_client_handle->hostname, tb_client_handle->access_token, fw_title_data, fw_version_data);
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

                    memset(http_client_url, 0, http_client_url_len + 1);

                    if (snprintf(http_client_url, http_client_url_len + 1, "%s://%s/api/v1/%s/firmware?title=%s&version=%s", (tb_client_handle->certificate) ? "https" : "http", tb_client_handle->hostname, tb_client_handle->access_token, fw_title_data, fw_version_data) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    esp_http_client_config_t http_client_config = {
                        .url = http_client_url,
                        .cert_pem = tb_client_handle->certificate,
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

                    esp_restart();
                }
                else
                {
                    if (esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, "{\"fw_state\":\"UPDATED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }
                }
            }
        }
        else
        {
            if (strncmp(mqtt_event_handle->topic, "/provision/response", mqtt_event_handle->topic_len) == 0)
            {
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

    case MQTT_USER_EVENT:
    {
        user_event_data = mqtt_event_handle->data;

        if (tb_client_handle->access_token)
        {
            esp_mqtt_client_publish(tb_client_handle->mqtt_client_handle, tb_client_handle->telemetry_topic, user_event_data, 0, 0, 0);
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
        esp_mqtt_client_disconnect(tb_client_handle->mqtt_client_handle);
    }

    if (user_event_data)
    {
        free(user_event_data);
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
