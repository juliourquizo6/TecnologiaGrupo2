#include "sgp30_sensor.h"
#include "power_manager.h"
#include "scheduler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h" // Para las configuraciones de menuconfig

#define I2C_SDA_PIN 21      // Pin SDA del ESP32
#define I2C_SCL_PIN 22      // Pin SCL del ESP32
#define I2C_FREQ_HZ 100000  // Frecuencia del bus I2C

static const char *TAG = "APP_MAIN"; // Solo un TAG global

// Función para inicializar el bus I2C
void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,   // GPIO para SDA
        .scl_io_num = I2C_SCL_PIN,   // GPIO para SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,  // Velocidad del bus: 100 kHz
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

// Función para escanear dispositivos en el bus I2C
void i2c_scan(void) {
    ESP_LOGI("I2C_SCAN", "Iniciando escaneo de dispositivos I2C...");
    for (int address = 1; address < 127; address++) {
        esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, address, NULL, 0, 1000 / portTICK_PERIOD_MS);
        if (ret == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "Dispositivo encontrado en dirección 0x%02X", address);
        }
    }
    ESP_LOGI("I2C_SCAN", "Escaneo completado.");
}

// Función principal
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema de monitorización de calidad del aire...");

    // Inicializar I2C
    i2c_master_init();

    // Inicializar los módulos
    sgp30_init();
    power_manager_init();
    scheduler_init();

    // Configurar horarios operativos desde menuconfig
    scheduler_config_operational_hours(CONFIG_WORKING_HOURS_START, CONFIG_WORKING_HOURS_END);

    // Determinar si el tiempo está configurado (ajustar según proyecto)
    scheduler_set_time_configured(false); // Cambia a 'true' si tienes un RTC o sincronización NTP

    // Escanear el bus I2C para encontrar dispositivos conectados
    i2c_scan();

    // Bucle principal
    while (1) {
        if (scheduler_is_operational()) {
            // En horario operativo
            uint16_t co2, tvoc;
            if (sgp30_read(&co2, &tvoc) == ESP_OK) {
                ESP_LOGI(TAG, "Lectura: CO2 = %d ppm, TVOC = %d ppb", co2, tvoc);
            } else {
                ESP_LOGE(TAG, "Error al leer datos del sensor SGP30");
            }
        } else {
            // Fuera del horario operativo
            ESP_LOGW(TAG, "Fuera del horario operativo. Entrando en modo de bajo consumo...");
            power_manager_check_and_sleep();
        }

        // Esperar según la frecuencia configurada en menuconfig
        vTaskDelay(pdMS_TO_TICKS(CONFIG_SAMPLE_FREQUENCY));
    }
}
