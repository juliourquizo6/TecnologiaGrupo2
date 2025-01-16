#include "power_manager.h"
#include "esp_sleep.h"
#include "esp_log.h"

static const char *TAG = "POWER_MANAGER";

void power_manager_init(void) {
    ESP_LOGI(TAG, "Configurando gestor de energía...");
    // Configuración inicial del gestor de energía
}

void power_manager_enter_deep_sleep(void) {
    ESP_LOGI(TAG, "Entrando en Deep Sleep...");
    esp_sleep_enable_timer_wakeup(10 * 60 * 1000000); // 10 minutos
    esp_deep_sleep_start();
}
