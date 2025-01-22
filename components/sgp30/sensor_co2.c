#include "sgp30.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include <esp_timer.h>
#include <esp_log.h>
#include "sensor_co2.h"

#define T_LECTURA   CONFIG_T_LECTURA
#define T_ENVIO     CONFIG_T_ENVIO
#define CALIBRATION 5

static const char* TAG = "SGP30";

i2c_port_t i2c_num = I2C_MASTER_NUM;
sgp30_dev_t sgp30_sensor;
esp_timer_handle_t tm_lectura_handle;
esp_timer_handle_t tm_envio_handle;
QueueHandle_t queue_timers_handle;
sensor_co2_handler sensor_handler;

struct data_sensor_co2
{
    uint16_t TVOC;
    uint16_t eCO2;
};

// data = media
// los dos handlers de los timers funcionan correctamente porque se activan mediante una sola tarea
static struct data_sensor_co2 data = {0};
static size_t count = 0;

/* I2C */
// Inicializacion
static void i2c_init(void) {
    int i2c_master_port = I2C_MODE_MASTER;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,         // select GPIO specific to your project
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,         // select GPIO specific to your project
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,  // select frequency specific to your project
    };

    // Configuracion
    ESP_ERROR_CHECK(i2c_param_config(i2c_num, &conf));

    // Instalacion del driver
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

}


int8_t main_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) { // *intf_ptr = dev->intf_ptr
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    if (len == 0) {
        return ESP_OK;
    }

    uint8_t chip_addr = *(uint8_t*)intf_ptr;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    
    if (reg_addr != 0xff) {
        i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
        i2c_master_start(cmd);
    }
    
    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);

    if (len > 1) {
        i2c_master_read(cmd, reg_data, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, NACK_VAL);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    
    i2c_cmd_link_delete(cmd);
    
    return ret;
}


int8_t main_i2c_write(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    int8_t ret = 0; /* Return 0 for Success, non-zero for failure */

    uint8_t chip_addr = *(uint8_t*)intf_ptr;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (chip_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    
    if (reg_addr != 0xFF) {
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    }

    i2c_master_write(cmd, reg_data, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    
    i2c_cmd_link_delete(cmd);
    
    return ret;
}


/* TIMERS */
// Callbacks
static void tm_lectura(void* arg) {
    sgp30_IAQ_measure(&sgp30_sensor);
    count++;
    data.eCO2 = data.eCO2 + (sgp30_sensor.eCO2 - data.eCO2) / count;
    data.TVOC = data.TVOC + (sgp30_sensor.TVOC - data.TVOC) / count;
}


static void tm_envio(void* arg) {
    cJSON *datos = cJSON_CreateObject();
    cJSON_AddNumberToObject(datos, "TVOC", data.TVOC);
    cJSON_AddNumberToObject(datos, "eCO2", data.eCO2);
    assert(datos);
    sensor_handler(datos);
    cJSON_Delete(datos);
    data.eCO2 = 0;
    data.TVOC = 0;
    count = 0;
}


// Inicializacion
static void timers_init(void) {
    // Timer lectura
    const esp_timer_create_args_t args_tm_lectura = {
        .callback = &tm_lectura,
        .name = "Tiempo de lectura"
    };

    ESP_ERROR_CHECK(esp_timer_create(&args_tm_lectura, &tm_lectura_handle));

    // Timer envio
    const esp_timer_create_args_t args_tm_envio = {
        .callback = &tm_envio,
        .name = "Tiempo de envio"
    };

    ESP_ERROR_CHECK(esp_timer_create(&args_tm_envio, &tm_envio_handle));

}

// Activacion
void set_timers() {
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_lectura_handle, pdMS_TO_TICKS(T_LECTURA)));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_envio_handle, pdMS_TO_TICKS(T_ENVIO)));
}

/* SGP30 */
// Inicializacion
static void sgp30_co2_init(void) {
    
    uint16_t eco2_baseline, tvoc_baseline;

    // I2C Bus
    i2c_init();
    
    // Sensor
    sgp30_init(&sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    // Calibracion del sensor
    for (int i = 0; i < CALIBRATION; i++) {
        ESP_LOGI(TAG, "Calibrando sensor %d de %d...", i+1, CALIBRATION);
        vTaskDelay(pdMS_TO_TICKS(1000));
        sgp30_IAQ_measure(&sgp30_sensor);
    }
    sgp30_get_IAQ_baseline(&sgp30_sensor, &eco2_baseline, &tvoc_baseline);

    timers_init();

    set_timers();
}


void sensor_co2_create(sensor_co2_handler handler){
    sensor_handler = handler;
    sgp30_co2_init();
}

void sensor_co2_set_frecuencia_lectura(int frec){
    ESP_ERROR_CHECK(esp_timer_restart(tm_lectura_handle, pdMS_TO_TICKS(frec)));
}

void sensor_co2_set_frecuencia_envio(int frec) {
    ESP_ERROR_CHECK(esp_timer_restart(tm_envio_handle, pdMS_TO_TICKS(frec)));
}
