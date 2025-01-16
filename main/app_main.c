#include "sgp30_sensor.h" 
#include "power_manager.h"
#include "scheduler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"

#define I2C_SDA_PIN 21  // Pin SDA del ESP32
#define I2C_SCL_PIN 22  // Pin SCL del ESP32
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
    esp_err_t ret;
    ESP_LOGI(TAG, "Escaneando el bus I2C...");

    // Escanea las direcciones del bus I2C
    for (uint8_t addr = 0; addr < 128; addr++) {
        // Intentar escribir a cada dirección
        ret = i2c_master_write_to_device(I2C_NUM_0, addr, NULL, 0, 10 / portTICK_PERIOD_MS);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Dispositivo encontrado en la dirección 0x%02X", addr);
            // Ahora leer desde la dirección para confirmar la comunicación
            uint8_t data;
            ret = i2c_master_read_from_device(I2C_NUM_0, addr, &data, 1, 10 / portTICK_PERIOD_MS);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Dispositivo en 0x%02X responde correctamente", addr);
            }
        } else {
            ESP_LOGE(TAG, "No se encontró dispositivo en la dirección 0x%02X", addr);
        }
    }
}


void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema de monitorización de calidad del aire...");

    // Inicializar I2C
    i2c_master_init();

    // Inicializar el sensor y otros módulos
    sgp30_init();
    power_manager_init();
    scheduler_init();

    // Escanear el bus I2C para encontrar dispositivos conectados
    i2c_scan();

    // Bucle principal
    while (1) {
        if (scheduler_is_operational()) {
            uint16_t co2, tvoc;
            if (sgp30_read(&co2, &tvoc) == ESP_OK) {
                ESP_LOGI(TAG, "Lectura: CO2 = %d ppm, TVOC = %d ppb", co2, tvoc);
            }
        } else {
            power_manager_enter_deep_sleep();
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_SAMPLE_FREQUENCY));
    }
}
