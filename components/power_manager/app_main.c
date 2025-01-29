#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "power_manager.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando el sistema...");

    // Inicializar NVS (requerido por algunos componentes, como WiFi o SNTP)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar el gestor de energía
    power_manager_init();

    ESP_LOGI(TAG, "Gestor de energía inicializado.");
}

