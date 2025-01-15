/**
 * @file main.c
 */

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "cJSON.h"

#include "main.h"
#include "wifi.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "mqtt_client.h"

#include "nvs.h"
#include "nvs_flash.h"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

/*! Saves bit values used in application */
static EventGroupHandle_t event_group;

/*! Saves OTA config received from ThingsBoard*/
static struct shared_keys
{
    char targetFwServerUrl[256];
    char targetFwVer[128];
} shared_attributes;

/*! Buffer to save a received MQTT message */
static char mqtt_msg[512];

static esp_mqtt_client_handle_t mqtt_client;

static void parse_ota_config(const cJSON *object)
{
    if (object != NULL)
    {
        cJSON *server_url_response = cJSON_GetObjectItem(object, TB_SHARED_ATTR_FIELD_TARGET_FW_URL);
        if (cJSON_IsString(server_url_response) && (server_url_response->valuestring != NULL) && strlen(server_url_response->valuestring) < sizeof(shared_attributes.targetFwServerUrl))
        {
            memcpy(shared_attributes.targetFwServerUrl, server_url_response->valuestring, strlen(server_url_response->valuestring));
            shared_attributes.targetFwServerUrl[sizeof(shared_attributes.targetFwServerUrl) - 1] = 0;
            ESP_LOGI(TAG, "Received firmware URL: %s", shared_attributes.targetFwServerUrl);
        }

        cJSON *target_fw_ver_response = cJSON_GetObjectItem(object, TB_SHARED_ATTR_FIELD_TARGET_FW_VER);
        if (cJSON_IsString(target_fw_ver_response) && (target_fw_ver_response->valuestring != NULL) && strlen(target_fw_ver_response->valuestring) < sizeof(shared_attributes.targetFwVer))
        {
            memcpy(shared_attributes.targetFwVer, target_fw_ver_response->valuestring, strlen(target_fw_ver_response->valuestring));
            shared_attributes.targetFwVer[sizeof(shared_attributes.targetFwVer) - 1] = 0;
            ESP_LOGI(TAG, "Received firmware version: %s", shared_attributes.targetFwVer);
        }
    }
}

static void start_ota(const char *current_ver, struct shared_keys ota_config)
{
    assert(current_ver != NULL);

    if (!fw_versions_are_equal(current_ver, ota_config.targetFwVer) && ota_params_are_specified(ota_config))
    {
        ESP_LOGW(TAG, "Starting OTA, firmware versions are different - current: %s, target: %s", current_ver, ota_config.targetFwVer);
        ESP_LOGI(TAG, "Target firmware version: %s", ota_config.targetFwVer);
        ESP_LOGI(TAG, "Firmware URL: %s", ota_config.targetFwServerUrl);
        esp_http_client_config_t config = {
            .url = ota_config.targetFwServerUrl,
            .cert_pem = (char *)server_cert_pem_start,
            .event_handler = _http_event_handler,
        };
        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK)
        {
            esp_restart();
        }
        else
        {
            ESP_LOGE(TAG, "Firmware Upgrades Failed");
        }
    }
}

static enum state connection_state(BaseType_t actual_event, const char *current_state_name)
{
    assert(current_state_name != NULL);

    if (actual_event & WIFI_DISCONNECTED_EVENT)
    {
        ESP_LOGE(TAG, "%s state, Wi-Fi not connected, wait for the connect", current_state_name);
        return STATE_WAIT_WIFI;
    }

    if (actual_event & MQTT_DISCONNECTED_EVENT)
    {
        ESP_LOGW(TAG, "%s state, MQTT not connected, wait for the connect", current_state_name);
        return STATE_WAIT_MQTT;
    }

    return STATE_CONNECTION_IS_OK;
}

static void ota_task(void *pvParameters)
{
    enum state current_connection_state = STATE_CONNECTION_IS_OK;
    enum state state = STATE_INITIAL;
    BaseType_t ota_events;
    BaseType_t actual_event = 0x00;
    char running_partition_label[sizeof(((esp_partition_t *)0)->label)];

    while (1)
    {
        if (state != STATE_INITIAL && state != STATE_APP_LOOP)
        {
            if (state != STATE_APP_LOOP)
            {
                xEventGroupClearBits(event_group, OTA_TASK_IN_NORMAL_STATE_EVENT);
            }

            actual_event = xEventGroupWaitBits(event_group,
                                               WIFI_CONNECTED_EVENT | WIFI_DISCONNECTED_EVENT | MQTT_CONNECTED_EVENT | MQTT_DISCONNECTED_EVENT | OTA_CONFIG_FETCHED_EVENT,
                                               false, false, portMAX_DELAY);
        }

        switch (state)
        {
        case STATE_INITIAL:
        {
            // Initialize NVS.
            esp_err_t err = nvs_flash_init();
            if (err == ESP_ERR_NVS_NO_FREE_PAGES)
            {
                // OTA app partition table has a smaller NVS partition size than the non-OTA
                // partition table. This size mismatch may cause NVS initialization to fail.
                // If this happens, we erase NVS partition and initialize NVS again.
                APP_ABORT_ON_ERROR(nvs_flash_erase());
                err = nvs_flash_init();
            }
            APP_ABORT_ON_ERROR(err);

            const esp_partition_t *running_partition = esp_ota_get_running_partition();
            strncpy(running_partition_label, running_partition->label, sizeof(running_partition_label));
            ESP_LOGI(TAG, "Running partition: %s", running_partition_label);

            initialise_wifi(running_partition_label);
            state = STATE_WAIT_WIFI;
            break;
        }
        case STATE_WAIT_WIFI:
        {
            if (actual_event & WIFI_DISCONNECTED_EVENT)
            {
                ESP_LOGW(TAG, "WAIT_WIFI state, Wi-Fi not connected, wait for the connect");
                state = STATE_WAIT_WIFI;
                break;
            }

            if (actual_event & WIFI_CONNECTED_EVENT)
            {
                mqtt_app_start(running_partition_label);
                state = STATE_WAIT_MQTT;
                break;
            }

            ESP_LOGE(TAG, "WAIT_WIFI state, unexpected event received: %d", actual_event);
            state = STATE_INITIAL;
            break;
        }
        case STATE_WAIT_MQTT:
        {
            current_connection_state = connection_state(actual_event, "WAIT_MQTT");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }

            if (actual_event & (WIFI_CONNECTED_EVENT | MQTT_CONNECTED_EVENT))
            {
                ESP_LOGI(TAG, "Connected to MQTT broker %s, on port %d", CONFIG_MQTT_BROKER_URL, CONFIG_MQTT_BROKER_PORT);

                // Send the current firmware version to ThingsBoard
                cJSON *current_fw = cJSON_CreateObject();
                cJSON_AddStringToObject(current_fw, TB_CLIENT_ATTR_FIELD_CURRENT_FW, FIRMWARE_VERSION);
                char *current_fw_attribute = cJSON_PrintUnformatted(current_fw);
                cJSON_Delete(current_fw);
                esp_mqtt_client_publish(mqtt_client, TB_ATTRIBUTES_TOPIC, current_fw_attribute, 0, 1, 0);
                // Free is intentional, it's client responsibility to free the result of cJSON_Print
                free(current_fw_attribute);

                // Send the shared attributes keys to receive their values
                esp_mqtt_client_subscribe(mqtt_client, TB_ATTRIBUTES_SUBSCRIBE_TO_RESPONSE_TOPIC, 1);
                esp_mqtt_client_publish(mqtt_client, TB_ATTRIBUTES_REQUEST_TOPIC, TB_SHARED_ATTR_KEYS_REQUEST, 0, 1, 0);
                ESP_LOGI(TAG, "Waiting for shared attributes response");

                state = STATE_WAIT_OTA_CONFIG_FETCHED;
                break;
            }

            ESP_LOGE(TAG, "WAIT_MQTT state, unexpected event received: %d", actual_event);
            state = STATE_INITIAL;
            break;
        }
        case STATE_WAIT_OTA_CONFIG_FETCHED:
        {
            current_connection_state = connection_state(actual_event, "WAIT_OTA_CONFIG_FETCHED");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }

            if (actual_event & (WIFI_CONNECTED_EVENT | MQTT_CONNECTED_EVENT))
            {
                if (actual_event & OTA_CONFIG_FETCHED_EVENT)
                {
                    ESP_LOGI(TAG, "Shared attributes were fetched from ThingsBoard");
                    xEventGroupClearBits(event_group, OTA_CONFIG_FETCHED_EVENT);
                    state = STATE_OTA_CONFIG_FETCHED;
                    break;
                }

                state = STATE_WAIT_OTA_CONFIG_FETCHED;
                break;
            }

            ESP_LOGE(TAG, "WAIT_OTA_CONFIG_FETCHED state, unexpected event received: %d", actual_event);
            state = STATE_INITIAL;
            break;
        }
        case STATE_OTA_CONFIG_FETCHED:
        {
            current_connection_state = connection_state(actual_event, "OTA_CONFIG_FETCHED");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }

            if (actual_event & (WIFI_CONNECTED_EVENT | MQTT_CONNECTED_EVENT))
            {

                start_ota(FIRMWARE_VERSION, shared_attributes);
                esp_mqtt_client_subscribe(mqtt_client, TB_ATTRIBUTES_TOPIC, 1);
                ESP_LOGI(TAG, "Subscribed to shared attributes updates");
                state = STATE_APP_LOOP;
                break;
            }
            ESP_LOGE(TAG, "OTA_CONFIG_FETCHED state, unexpected event received: %d", actual_event);
            state = STATE_INITIAL;
            break;
        }
        case STATE_APP_LOOP:
        {
            current_connection_state = connection_state(actual_event, "APP_LOOP");
            if (current_connection_state != STATE_CONNECTION_IS_OK)
            {
                state = current_connection_state;
                break;
            }

            if (actual_event & (WIFI_CONNECTED_EVENT | MQTT_CONNECTED_EVENT))
            {
                ota_events = xEventGroupWaitBits(event_group, OTA_CONFIG_UPDATED_EVENT, false, true, 0);
                if ((ota_events & OTA_CONFIG_UPDATED_EVENT))
                {
                    start_ota(FIRMWARE_VERSION, shared_attributes);
                }
                xEventGroupClearBits(event_group, OTA_CONFIG_UPDATED_EVENT);
                xEventGroupSetBits(event_group, OTA_TASK_IN_NORMAL_STATE_EVENT);
                state = STATE_APP_LOOP;
                break;
            }

            ESP_LOGE(TAG, "APP_LOOP state, unexpected event received: %d", actual_event);
            state = STATE_INITIAL;
            break;
        }
        default:
        {
            ESP_LOGE(TAG, "Unexpected state");
            state = STATE_INITIAL;
            break;
        }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
