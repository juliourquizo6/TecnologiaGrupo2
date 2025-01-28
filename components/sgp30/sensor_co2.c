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

static i2c_port_t i2c_num = I2C_MASTER_NUM;
sgp30_dev_t sgp30_sensor;
esp_timer_handle_t tm_lectura_handle;
esp_timer_handle_t tm_envio_handle;
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
    // Calibrated for ethanol (https://www.catsensors.com/media/pdf/Sensor_Sensirion_IAM.pdf)
    cJSON_AddNumberToObject(datos, "tvoc", (double)data.TVOC * (110.0 / (0.0244 * 1000.0)) / 1000.0);
    cJSON_AddNumberToObject(datos, "co2", data.eCO2);
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
void set_timers(void *arg) {
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_lectura_handle, T_LECTURA * 1000));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_envio_handle, T_ENVIO * 1000));
}


/* SGP30 */
// Inicializacion
static void sgp30_co2_init(void) {

    uint16_t eco2_baseline, tvoc_baseline;

    // I2C Bus
    i2c_init();

    // Sensor
    sgp30_init(&sgp30_sensor, (sgp30_read_fptr_t)main_i2c_read, (sgp30_write_fptr_t)main_i2c_write);

    timers_init();

    esp_timer_handle_t start_timer;

    const esp_timer_create_args_t args_start_timer = {
        .callback = &set_timers,
        .name = "Tiempo de inicio"
    };

    ESP_ERROR_CHECK(esp_timer_create(&args_start_timer, &start_timer));
    // Wait 15 seconds before starting the timers (from datasheet)
    ESP_ERROR_CHECK(esp_timer_start_once(start_timer, 15000000));
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
