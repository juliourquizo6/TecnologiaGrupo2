#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "time.h"
#include "stdlib.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_log.h"        // Para ESP_LOGI
#include "esp_sleep.h"      // Para funciones de Deep Sleep

static const char *TAG = "POWER_MANAGER";

// Configuración del horario operativo
#define OPERATIVE_HOUR_START 0   // Inicio simulado (0:00)
#define OPERATIVE_HOUR_END 0     // Fin simulado (0:08, después lo calcularemos)
#define TIMER_PRUEBA 5 * 1000000ULL   // Tiempo inicial simulado (5 segundos)
#define SLEEP_TIMER 10 * 1000000ULL   // Tiempo de Deep Sleep simulado (10 segundos)



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
        ESP_LOGI(TAG, "Evento activo: El sistema está en horario operativo. Simulando duración...");
        
        // Simular el tiempo restante hasta el final del horario operativo (8 segundos)
        int64_t time_to_sleep = calculate_time_to_next_event(false); 
        ESP_ERROR_CHECK(esp_timer_start_once(timer, time_to_sleep));
    } else {
        ESP_LOGI(TAG, "Fuera del horario operativo: Entrando en Deep Sleep.");
        
        // Simular Deep Sleep (10 segundos)
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

    if (timeinfo.tm_hour >= OPERATIVE_HOUR_START && timeinfo.tm_hour < OPERATIVE_HOUR_END) {
        ESP_LOGI(TAG, "Dentro del horario operativo. Calculando tiempo hasta el final...");
        time_to_event = calculate_time_to_next_event(false);
    } else {
        ESP_LOGI(TAG, "Fuera del horario operativo. Calculando tiempo hasta el próximo inicio...");
        time_to_event = calculate_time_to_next_event(true);
    }

    ESP_LOGI(TAG, "Tiempo hasta el próximo evento: %lld microsegundos", time_to_event);
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

