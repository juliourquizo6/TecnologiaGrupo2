#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "time.h"
#include "stdlib.h"
#include "esp_err.h" 
//tiempo inicial si no se tiene hora
#define timer_prueba 14 * 60 * 60 * 1000000
#define sleep_timer 10 * 60 * 60 * 1000000
#define time_zone "UTC+1"

static esp_timer_handle_t timer;
//comprobar si esta dentro de horario operativo
void timer_cb(void *arg) {     
            ESP_LOGI(TAG, "El sistema esta entrando en Deep Sleep por 10 horas...");
            esp_sleep_enable_timer_wakeup(sleep_timer); 
    
        esp_deep_sleep_start();
}
void sync_hour(struct timeval *tv) {
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    ESP_ERROR_CHECK(esp_timer_restart(timer,));

}
void power_manager_init(void) {
   setenv("TZ", time_zone, 1);
   tzset();
   esp_timer_create_args_t timer_config={
        .name="manager_timer",
        .callback=timer_cb
   };

   ESP_ERROR_CHECK(esp_timer_create(timer_config, &timer));
   ESP_ERROR_CHECK(esp_timer_start_once(timer, timer_prueba));
   esp_sntp_config_t config= {
        .start=true,
        .sync_cb=sync_hour,
        .server_from_dhcp=true,
   };

   ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
   
   
}
