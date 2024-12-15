#include <stdio.h>
#include "esp_wifi.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
#include <wifi_component.h>


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

    wifi_register_event_handlers();

    wifi_init();
    
    // 1: Provisionamiento
    
    // 2: Coneccion WiFi
    
    //2.2: Tarea Sensor
    

}
