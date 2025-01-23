#include "power_manager.h"
#include "scheduler.h"
#include "esp_sleep.h"
#include "esp_log.h"

static const char *TAG = "POWER_MANAGER";

void power_manager_init(void) {
    ESP_LOGI(TAG, "Configurando gestor de energía...");
    // Inicialización del gestor de energía (si fuese necesario)
}

void power_manager_check_and_sleep(void) {
    if (!scheduler_is_operational()) {
        ESP_LOGI(TAG, "Fuera del horario operativo.");

        if (scheduler_time_configured()) {
            // Si el sistema tiene la hora configurada, calcula el tiempo hasta el próximo periodo operativo.
            uint64_t sleep_duration = scheduler_get_sleep_duration();
            ESP_LOGI(TAG, "Entrando en Deep Sleep por %llu segundos...", sleep_duration / 1000000);
            esp_sleep_enable_timer_wakeup(sleep_duration);
        } else {
            // Si no tiene la hora configurada, entra en Deep Sleep durante 10 horas.
            ESP_LOGI(TAG, "El sistema no tiene hora configurada. Entrando en Deep Sleep por 10 horas...");
            esp_sleep_enable_timer_wakeup(10 * 60 * 60 * 1000000); // 10 horas
        }

        esp_deep_sleep_start();
    } else {
        ESP_LOGI(TAG, "En horario operativo. Continuando funcionamiento...");
    }
}
