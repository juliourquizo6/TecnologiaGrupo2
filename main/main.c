<<<<<<< HEAD
#include <stdio.h>
#include "esp_wifi.h"
#include <esp_log.h>
#include <string.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
#include <wifi_component.h>

const char *TAG1 = "MAIN";

static esp_event_loop_handle_t event_loop_handle;

void app_main(void)
{

    esp_event_loop_args_t event_loop_args = {
        .queue_size = 10,
        .task_name = "esp_event_loop_run_task",
        .task_priority = 5,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY,
    };

    esp_event_loop_create(&event_loop_args, &event_loop_handle);

    provision_and_connect();
    ESP_LOGI(TAG1, "%s", (char *)thingsboard_data);
    
    // 1: Provisionamiento
    
    // 2: Coneccion WiFi
    
    //2.2: Tarea Sensor
    

=======
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "sgp30.h"
#include "tb_client.h"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
>>>>>>> main
}
