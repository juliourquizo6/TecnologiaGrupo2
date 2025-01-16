#include "scheduler.h"
#include "esp_log.h"

static const char *TAG = "SCHEDULER";

bool scheduler_is_operational(void) {
    // Verifica si el sistema está en horario de funcionamiento
    // Simulación para este ejemplo
    return true;
}

void scheduler_init(void) {
    ESP_LOGI(TAG, "Inicializando scheduler...");
}
