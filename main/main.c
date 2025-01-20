#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "esp_event.h"
#include "esp_task.h"
#include "esp_console.h"
#include "esp_err.h"
#include "sdkconfig.h"

#include "sgp30.h"

ESP_EVENT_DECLARE_BASE(SGP30_EVENT);
ESP_EVENT_DEFINE_BASE(SGP30_EVENT);

// Variable necesaria para definir un event_loop
static esp_event_loop_handle_t event_loop_handle;

// LOG
const static char *TAG = "MAIN";

enum
{
    SGP30_EVENT_NEWSAMPLE
};

// Gestor de eventos
static void event_handle(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // EVENTOS...
    if (event_base == SGP30_EVENT && event_id == SGP30_EVENT_NEWSAMPLE) {
        // indice???
        uint16_t datos[];
        datos = sgp30_datos();
        
    }

}


void app_main(void) {
    // Se crea la configuracion del event_loop
    esp_event_loop_args_t event_loop_args = {
        .queue_size = CONFIG_ESP_SYSTEM_EVENT_QUEUE_SIZE,
        .task_name = "esp_event_loop_run_task",
        .task_priority = ESP_TASKD_EVENT_PRIO,
        .task_stack_size = ESP_TASKD_EVENT_STACK,
        .task_core_id = tskNO_AFFINITY,
    };
    // Se crea el event_loop con la configuracion y con el manejador
    esp_event_loop_create(&event_loop_args, &event_loop_handle);

    // Se registran todos los manejadores en el manejador del bucle de eventos.
    esp_event_handler_instance_register_with(event_loop_handle, SGP30_EVENT, SGP30_EVENT_NEWSAMPLE, event_handle, NULL, NULL);

    // SGP30
    sgp30_co2_init();
    
    // Timers
    timers_init();
    set_timers();

}
