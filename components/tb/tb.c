#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "tb.h"

ESP_EVENT_DEFINE_BASE(TB_EVENTS);

static void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t tb_create(thingsboard *tb, const char *hostname, const char *telemetry_topic, const char *certificate, esp_event_handler_t event_handler, void *event_handler_arg)
{
    assert(tb);

    if (!hostname)
    {
        hostname = "demo.thingsboard.io";
    }

    if (!telemetry_topic)
    {
        telemetry_topic = "v1/devices/me/telemetry";
    }

    *tb = (thingsboard){
        .hostname = NULL,
        .telemetry_topic = NULL,
        .certificate = NULL,
        .event_loop = NULL,
        .mqtt_client = NULL,
        .access_token = "",
    };

    esp_err_t err = ESP_OK;

    tb->hostname = strdup(hostname);

    tb->telemetry_topic = strdup(telemetry_topic);

    tb->certificate = certificate;

    tb->access_token[0] = '\0';

    esp_event_loop_args_t event_loop_args = {
        .queue_size = 1,
        .task_name = NULL,
    };

    err = esp_event_loop_create(&event_loop_args, &tb->event_loop);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_event_handler_register_with(tb->event_loop, TB_EVENTS, ESP_EVENT_ANY_ID, event_handler, event_handler_arg);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    esp_mqtt_client_config_t mqtt_config = {0};

    tb->mqtt_client = esp_mqtt_client_init(&mqtt_config);
    if (!tb->mqtt_client)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    err = esp_mqtt_client_register_event(tb->mqtt_client, MQTT_EVENT_ANY, tb_mqtt_event_handler, (void *)tb);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    err = esp_mqtt_client_start(tb->mqtt_client);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

cleanup:
    if (err != ESP_OK)
    {
        tb_delete(tb);
    }

    return err;
}

esp_err_t tb_delete(thingsboard *tb)
{
    assert(tb);

    esp_err_t err = ESP_OK;

    if (tb->mqtt_client)
    {
        err = esp_mqtt_client_destroy(tb->mqtt_client);
        tb->mqtt_client = NULL;
    }

    if (tb->event_loop)
    {
        esp_event_loop_delete(tb->event_loop);
        tb->event_loop = NULL;
    }

    if (tb->access_token[0] != '\0')
    {
        tb->access_token[0] = '\0';
    }

    if (tb->certificate)
    {
        tb->certificate = NULL;
    }

    if (tb->telemetry_topic)
    {
        free(tb->telemetry_topic);
        tb->telemetry_topic = NULL;
    }

    if (tb->hostname)
    {
        free(tb->hostname);
        tb->hostname = NULL;
    }

    return err;
}

esp_err_t tb_send(const thingsboard *tb, const char *data)
{
    assert(tb);
    assert(data);

    char *event_data = NULL;

    esp_err_t err = ESP_OK;

    event_data = (char *)malloc(strlen(data) + 1);
    if (!event_data)
    {
        err = ESP_FAIL;
        goto cleanup;
    }

    memset(event_data, 0, strlen(data) + 1);

    esp_mqtt_event_t mqtt_event = {
        .event_id = MQTT_USER_EVENT,
        .client = tb->mqtt_client,
        .data = event_data,
    };

    err = esp_mqtt_dispatch_custom_event(tb->mqtt_client, &mqtt_event);
    if (err != ESP_OK)
    {
        goto cleanup;
    }

    event_data = NULL;

cleanup:
    if (event_data)
    {
        free(event_data);
        event_data = NULL;
    }

    return err;
}

void tb_mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    thingsboard *tb = (thingsboard *)event_handler_arg;
    esp_mqtt_event_handle_t mqtt_event = (esp_mqtt_event_handle_t)event_data;

    assert(tb);
    assert(mqtt_event);

    nvs_handle_t nvs = 0;
    char *mqtt_uri = NULL;
    char *http_url = NULL;
    cJSON *root = NULL;
    char *root_data = NULL;
    char *user_data = NULL;

    esp_err_t err = ESP_OK;

    switch (mqtt_event->event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
    {
        char access_token[ACCESS_TOKEN_LEN_MAX + 1] = "";

        err = nvs_open("tb", NVS_READWRITE, &nvs);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        {
            goto cleanup;
        }

        size_t access_token_len = sizeof(access_token);

        err = nvs_get_str(nvs, "access_token", access_token, &access_token_len);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        {
            goto cleanup;
        }

        int mqtt_uri_len = snprintf(NULL, 0, "%s://%s", (tb->certificate) ? "mqtts" : "mqtt", tb->hostname);
        if (mqtt_uri_len < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        mqtt_uri = (char *)malloc(mqtt_uri_len + 1);
        if (!mqtt_uri)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        memset(mqtt_uri, 0, mqtt_uri_len + 1);

        if (snprintf(mqtt_uri, mqtt_uri_len + 1, "%s://%s", (tb->certificate) ? "mqtts" : "mqtt", tb->hostname) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        esp_mqtt_client_config_t mqtt_config = {
            .broker.address.uri = mqtt_uri,
            .broker.verification.certificate = tb->certificate,
            .credentials.username = (access_token[0] == '\0') ? "provision" : access_token,
        };

        err = esp_mqtt_set_config(mqtt_event->client, &mqtt_config);
        if (err != ESP_OK)
        {
            goto cleanup;
        }

        memcpy(tb->access_token, access_token, sizeof(access_token));

        break;
    }

    case MQTT_EVENT_CONNECTED:
    {
        if (esp_mqtt_client_subscribe(mqtt_event->client, "/provision/response", 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (esp_mqtt_client_publish(mqtt_event->client, "/provision/request", "{\"deviceName\":\"" CONFIG_TB_DEVICE_NAME "\",\"provisionDeviceKey\":\"" CONFIG_TB_PROVISION_DEVICE_KEY "\",\"provisionDeviceSecret\":\"" CONFIG_TB_PROVISION_DEVICE_SECRET "\"}", 0, 0, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (esp_mqtt_client_subscribe(mqtt_event->client, "v1/devices/me/attribute", 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (esp_mqtt_client_subscribe(mqtt_event->client, "v1/devices/me/attributes/response/0", 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        if (esp_mqtt_client_publish(mqtt_event->client, "v1/devices/me/attributes/response/0", "{}", 0, 0, 0) < 0)
        {
            err = ESP_FAIL;
            goto cleanup;
        }

        break;
    }

    case MQTT_EVENT_DATA:
    {
        cJSON *attributes = NULL;

        if (strncmp(mqtt_event->topic, "/provision/response", mqtt_event->topic_len) == 0)
        {
            root = cJSON_ParseWithLength(mqtt_event->data, mqtt_event->data_len);
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

            err = nvs_open("tb", NVS_READWRITE, &nvs);
            if (err != ESP_OK)
            {
                goto cleanup;
            }

            err = nvs_set_str(nvs, "access_token", credentials_value_data);
            if (err != ESP_OK)
            {
                goto cleanup;
            }

            err = nvs_commit(nvs);
            if (err != ESP_OK)
            {
                goto cleanup;
            }

            esp_mqtt_client_config_t mqtt_config = {
                .network.reconnect_timeout_ms = 1,
            };

            err = esp_mqtt_set_config(mqtt_event->client, &mqtt_config);
            if (err != ESP_OK)
            {
                goto cleanup;
            }

            err = esp_mqtt_client_disconnect(mqtt_event->client);
            if (err != ESP_OK)
            {
                goto cleanup;
            }
        }
        else if (strncmp(mqtt_event->topic, "v1/devices/me/attributes", mqtt_event->topic_len) == 0)
        {
            root = cJSON_ParseWithLength(mqtt_event->data, mqtt_event->data_len);
            if (!root)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            attributes = root;
        }
        else if (strncmp(mqtt_event->topic, "v1/devices/me/attributes/response/0", mqtt_event->topic_len) == 0)
        {
            root = cJSON_ParseWithLength(mqtt_event->data, mqtt_event->data_len);
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

        if (attributes)
        {
            root_data = cJSON_PrintUnformatted(attributes);
            if (!root_data)
            {
                err = ESP_FAIL;
                goto cleanup;
            }

            err = esp_event_post_to(tb->event_loop, TB_EVENTS, TB_EVENT_ATTRIBUTES, (void *)root_data, strlen(root_data) + 1, portMAX_DELAY);
            if (err != ESP_OK)
            {
                goto cleanup;
            }

            err = esp_event_loop_run(tb->event_loop, 0);
            if (err != ESP_OK)
            {
                goto cleanup;
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
                    err = ESP_FAIL;
                    goto cleanup;
                }

                memset(current_fw_data, 0, current_fw_data_len + 1);

                if (snprintf(current_fw_data, current_fw_data_len + 1, "{\"current_fw_title\":\"%s\",\"current_fw_version\":\"%s\"}", current_fw_title_data, current_fw_version_data) < 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, current_fw_data, 0, 0, 0) < 0)
                {
                    err = ESP_FAIL;
                    goto cleanup;
                }

                if (strcmp(fw_title_data, current_fw_title_data) != 0 || strcmp(fw_version_data, current_fw_version_data) != 0)
                {
                    if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"DOWNLOADING\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    int http_url_len = snprintf(NULL, 0, "%s://%s/api/v1/%s/firmware?title=%s&version=%s", (tb->certificate) ? "https" : "http", tb->hostname, tb->access_token, fw_title_data, fw_version_data);
                    if (http_url_len < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    http_url = (char *)malloc(http_url_len + 1);
                    if (!http_url)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    memset(http_url, 0, http_url_len + 1);

                    if (snprintf(http_url, http_url_len + 1, "%s://%s/api/v1/%s/firmware?title=%s&version=%s", (tb->certificate) ? "https" : "http", tb->hostname, tb->access_token, fw_title_data, fw_version_data) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    esp_http_client_config_t http_config = {
                        .url = http_url,
                        .cert_pem = tb->certificate,
                    };

                    esp_https_ota_config_t ota_config = {
                        .http_config = &http_config,
                    };

                    err = esp_https_ota(&ota_config);
                    if (err != ESP_OK)
                    {
                        if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"FAILED\"}", 0, 0, 0) < 0)
                        {
                            err = ESP_FAIL;
                            goto cleanup;
                        }

                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"DOWNLOADED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"VERIFIED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"UPDATING\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }

                    esp_restart();
                }
                else
                {
                    if (esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, "{\"fw_state\":\"UPDATED\"}", 0, 0, 0) < 0)
                    {
                        err = ESP_FAIL;
                        goto cleanup;
                    }
                }
            }
        }

        break;
    }

    case MQTT_USER_EVENT:
    {
        user_data = mqtt_event->data;

        esp_mqtt_client_publish(mqtt_event->client, tb->telemetry_topic, user_data, 0, 0, 0);

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
        esp_mqtt_client_disconnect(mqtt_event->client);
    }

    if (user_data)
    {
        free(user_data);
        user_data = NULL;
    }

    if (root_data)
    {
        cJSON_free(root_data);
        root_data = NULL;
    }

    if (root)
    {
        cJSON_Delete(root);
        root = NULL;
    }

    if (http_url)
    {
        free(http_url);
        http_url = NULL;
    }

    if (mqtt_uri)
    {
        free(mqtt_uri);
        mqtt_uri = NULL;
    }

    if (nvs != 0)
    {
        nvs_close(nvs);
        nvs = 0;
    }
}
