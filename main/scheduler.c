#include "scheduler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "time.h"

static const char *TAG = "SCHEDULER";

// Configuración predeterminada de funcionamiento
static int operational_start_hour = 8;  // Hora de inicio por defecto
static int operational_end_hour = 22;  // Hora de fin por defecto
static bool is_time_configured = false; // Verifica si el tiempo está configurado

void scheduler_init(void) {
    ESP_LOGI(TAG, "Inicializando scheduler...");
    // Aquí se podrían cargar configuraciones desde menuconfig
}

bool scheduler_is_operational(void) {
    if (!is_time_configured) {
        ESP_LOGW(TAG, "El tiempo no está configurado. Funciona en modo alternativo.");
        return true; // Por defecto, siempre operativo si no hay tiempo configurado
    }

    // Obtener la hora actual
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Verifica si está en el rango de funcionamiento
    int current_hour = timeinfo.tm_hour;
    if (current_hour >= operational_start_hour && current_hour < operational_end_hour) {
        ESP_LOGI(TAG, "Hora actual: %d. Dentro del horario operativo.", current_hour);
        return true;
    } else {
        ESP_LOGI(TAG, "Hora actual: %d. Fuera del horario operativo.", current_hour);
        return false;
    }
}

void scheduler_config_operational_hours(int start_hour, int end_hour) {
    operational_start_hour = start_hour;
    operational_end_hour = end_hour;
    ESP_LOGI(TAG, "Horario operativo configurado: %d:00 a %d:00", start_hour, end_hour);
}

void scheduler_set_time_configured(bool configured) {
    is_time_configured = configured;
}

bool scheduler_time_configured(void) {
    // Devuelve si el tiempo está configurado
    return is_time_configured;
}

uint64_t scheduler_get_sleep_duration(void) {
    if (!is_time_configured) {
        // Si no está configurado, el Deep Sleep será de 10 horas (en microsegundos)
        return 10 * 60 * 60 * 1000000ULL;
    }

    // Obtener la hora actual
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int current_hour = timeinfo.tm_hour;

    if (current_hour >= operational_end_hour) {
        // Calcular el tiempo hasta el inicio del próximo periodo operativo (al día siguiente)
        int hours_to_sleep = 24 - current_hour + operational_start_hour;
        return hours_to_sleep * 60 * 60 * 1000000ULL;
    } else if (current_hour < operational_start_hour) {
        // Calcular el tiempo hasta el inicio del periodo operativo (el mismo día)
        int hours_to_sleep = operational_start_hour - current_hour;
        return hours_to_sleep * 60 * 60 * 1000000ULL;
    }

    // Si está dentro del horario operativo, no debería llamarse esta función
    return 0;
}
