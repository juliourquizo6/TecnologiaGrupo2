#include "power_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h" // Para las configuraciones de menuconfig

static const char *TAG = "APP_MAIN"; // Solo un TAG global

// Función principal
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema de monitorización de calidad del aire...");

    power_manager_init();

    // Bucle principal
    while (1) {

        // Fuera del horario operativo
        ESP_LOGW(TAG, "Fuera del horario operativo. Entrando en modo de bajo consumo...");

        // Esperar según la frecuencia configurada en menuconfig
        vTaskDelay(pdMS_TO_TICKS(CONFIG_SAMPLE_FREQUENCY));
    }
}
