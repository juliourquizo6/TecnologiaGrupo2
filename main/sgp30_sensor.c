#include "sgp30_sensor.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>  // Para memcpy
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SGP30_I2C_ADDRESS 0x58  // Dirección I2C del SGP30
#define SGP30_CMD_INIT_AIR_QUALITY 0x2003
#define SGP30_CMD_MEASURE_AIR_QUALITY 0x2008
#define SGP30_CMD_GET_FEATURE_SET 0x202F

#define SGP30_SDA_PIN 21  // Pin SDA del ESP32
#define SGP30_SCL_PIN 22  // Pin SCL del ESP32
#define SGP30_FREQ_HZ 100000  // Frecuencia de bus I2C

static const char *TAG = "SGP30_SENSOR";

// Función para calcular el CRC de los datos según el polinomio CRC-8
static uint8_t sgp30_calculate_crc(uint8_t *data, size_t length) {
    uint8_t crc = 0xFF;  // Valor inicial del CRC
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;  // Polinomio CRC-8: 0x31
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// Función para enviar comandos al sensor SGP30
static esp_err_t sgp30_send_command(uint16_t cmd) {
    uint8_t data[2] = { cmd >> 8, cmd & 0xFF };  // Divide el comando en dos bytes
    return i2c_master_write_to_device(I2C_NUM_0, SGP30_I2C_ADDRESS, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
}

// Función para leer datos del sensor SGP30 con validación CRC
static esp_err_t sgp30_read_data(uint8_t *data, size_t length) {
    uint8_t raw_data[6];  // 6 bytes: 2 bytes CO2, 2 bytes TVOC, 2 bytes CRC
    esp_err_t ret = i2c_master_read_from_device(I2C_NUM_0, SGP30_I2C_ADDRESS, raw_data, sizeof(raw_data), 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al leer datos del sensor: %s", esp_err_to_name(ret));
        return ret;
    }

    // Verificar el CRC de cada par de datos (CO2 y TVOC)
    for (int i = 0; i < 2; i++) {
        if (sgp30_calculate_crc(&raw_data[i * 3], 2) != raw_data[i * 3 + 2]) {
            ESP_LOGE(TAG, "CRC inválido para el bloque %d", i);
            return ESP_ERR_INVALID_CRC;
        }
    }

    // Copiar los datos validados a la salida
    memcpy(data, raw_data, length);  // Solo copia los datos sin los valores CRC
    return ESP_OK;
}

// Inicialización del sensor SGP30
void sgp30_init(void) {
    ESP_LOGI(TAG, "Inicializando sensor SGP30...");

    esp_err_t ret;

    // Configuración del bus I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SGP30_SDA_PIN,
        .scl_io_num = SGP30_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SGP30_FREQ_HZ,
    };

    ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al configurar el I2C: %s", esp_err_to_name(ret));
        return;
    }

    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al instalar el driver I2C: %s", esp_err_to_name(ret));
        return;
    }

    // Envía el comando de inicialización de calidad del aire
    ret = sgp30_send_command(SGP30_CMD_INIT_AIR_QUALITY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando el sensor SGP30: %s", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);  // Espera breve tras la inicialización
    ESP_LOGI(TAG, "SGP30 inicializado correctamente.");
}

// Función para leer los datos de CO2 y TVOC
esp_err_t sgp30_read(uint16_t *co2, uint16_t *tvoc) {
    ESP_LOGI(TAG, "Leyendo datos del sensor SGP30...");

    // Envía el comando para medir la calidad del aire
    esp_err_t ret = sgp30_send_command(SGP30_CMD_MEASURE_AIR_QUALITY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al enviar comando de lectura: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(12 / portTICK_PERIOD_MS);  // Tiempo mínimo necesario para obtener datos

    uint8_t data[6];  // 2 bytes CO2, 2 bytes TVOC, 2 bytes CRC
    ret = sgp30_read_data(data, sizeof(data));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error al leer datos del sensor: %s", esp_err_to_name(ret));
        return ret;
    }

    // Extraer valores de CO2 y TVOC
    *co2 = (data[0] << 8) | data[1];
    *tvoc = (data[3] << 8) | data[4];

    ESP_LOGI(TAG, "Lectura exitosa: CO2 = %d ppm, TVOC = %d ppb", *co2, *tvoc);
    return ESP_OK;
}



