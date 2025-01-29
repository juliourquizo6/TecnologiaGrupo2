#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "time.h"
#include "stdlib.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_log.h"

static const char *TAG = "POWER_MANAGER";

// Configuración del horario operativo
#define OPERATIVE_HOUR_START 8   // Hora de inicio (8:00)
#define OPERATIVE_HOUR_END 22   // Hora de fin (22:00)

// Tiempo inicial si no se tiene hora sincronizada
#define TIMER_PRUEBA 14 * 60 * 60 * 1000000ULL
#define SLEEP_TIMER 10 * 60 * 60 * 1000000ULL
#define TIME_ZONE "UTC+1"

static esp_timer_handle_t timer;

// Función para calcular el tiempo hasta el próximo evento
static int64_t calculate_time_to_next_event(bool start_of_day) {
    time_t now = time(NULL); // Hora actual en segundos desde la época Unix
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    struct tm event_time = timeinfo;

    if (start_of_day) {
        // Configurar la hora de inicio del día operativo
        event_time.tm_hour = OPERATIVE_HOUR_START;
        event_time.tm_min = 0;
        event_time.tm_sec = 0;
    } else {
        // Configurar la hora de fin del día operativo
        event_time.tm_hour = OPERATIVE_HOUR_END;
        event_time.tm_min = 0;
        event_time.tm_sec = 0;
    }

    // Convertir el evento a tiempo en segundos desde la época Unix
    time_t event_timestamp = mktime(&event_time);

    // Si el tiempo calculado ya pasó hoy, moverlo al día siguiente
    if (event_timestamp < now) {
        event_timestamp += 24 * 60 * 60; // Agregar un día en segundos
    }

    // Calcular la diferencia en microsegundos
    return (event_timestamp - now) * 1000000ULL;
}

// Callback del temporizador
void timer_cb(void *arg) {
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_hour >= OPERATIVE_HOUR_START && timeinfo.tm_hour < OPERATIVE_HOUR_END) {
        // Dentro del horario operativo
        ESP_LOGI(TAG, "Dentro del horario operativo. Calculando tiempo hasta el fin del día operativo...");
        int64_t time_to_sleep = calculate_time_to_next_event(false); // Tiempo hasta el fin del día
        ESP_ERROR_CHECK(esp_timer_start_once(timer, time_to_sleep));
    } else {
        // Fuera del horario operativo, entrar en Deep Sleep
        ESP_LOGI(TAG, "El sistema está entrando en Deep Sleep por 10 horas...");
        esp_sleep_enable_timer_wakeup(SLEEP_TIMER);
        esp_deep_sleep_start();
    }
}

// Sincronización horaria
void sync_hour(struct timeval *tv) {
    ESP_LOGI(TAG, "Sincronización horaria completada.");
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int64_t time_to_event = 0;

    // Determinar si estamos dentro o fuera del horario operativo
    if (timeinfo.tm_hour >= OPERATIVE_HOUR_START && timeinfo.tm_hour < OPERATIVE_HOUR_END) {
        time_to_event = calculate_time_to_next_event(false); // Fin del horario operativo
    } else {
        time_to_event = calculate_time_to_next_event(true); // Inicio del próximo horario operativo
    }

    // Reiniciar el temporizador
    ESP_ERROR_CHECK(esp_timer_restart(timer, time_to_event));
}

// Inicialización del gestor de energía
void power_manager_init(void) {
    setenv("TZ", TIME_ZONE, 1);
    tzset();

    // Crear el temporizador
    esp_timer_create_args_t timer_config = {
        .name = "manager_timer",
        .callback = timer_cb,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &timer));

    // Configurar el temporizador inicial
    ESP_ERROR_CHECK(esp_timer_start_once(timer, TIMER_PRUEBA));

    // Inicializar la sincronización NTP
    esp_sntp_config_t config = {
        .start = true,
        .sync_cb = sync_hour,
        .server_from_dhcp = true,
    };
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
}