
#include "sgp30.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include <esp_timer.h>
#include <esp_log.h>

#define T_LECTURA  CONFIG_T_LECTURA
#define T_ENVIO    CONFIG_T_ENVIO
#define MAX_INDICE (T_ENVIO / T_LECTURA)

static const char* TAG = "SGP30";

i2c_port_t i2c_num = I2C_MASTER_NUM;
sgp30_dev_t sgp30_sensor;
esp_timer_handle_t tm_lectura_handle;
esp_timer_handle_t tm_envio_handle;


uint16_t eco2_lectura[MAX_INDICE];
int eco2_lectura_indice = 0;

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
    eco2_lectura[eco2_lectura_indice] = sgp30_sensor.eCO2;
    eco2_lectura_indice ++;
}


static void tm_envio(void* arg) {
    // Envio de datos.
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_lectura_handle, (T_LECTURA * 1000)));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tm_envio_handle, (T_ENVIO * 1000)));
}

// Paro
void stop_timers() {
    esp_timer_stop(tm_lectura_handle);
    esp_timer_stop(tm_envio_handle);
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
    for (int i = 0; i < 15; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        sgp30_IAQ_measure(&sgp30_sensor);
    }
    sgp30_get_IAQ_baseline(&sgp30_sensor, &eco2_baseline, &tvoc_baseline);

}

// Funciones: Datos obtenidos
uint16_t sgp30_datos() {
    
    uint16_t datos[sizeof(eco2_lectura)];

    // Se copia los datos de lectura
    for (int i = 0; i < 5; i++) {
        datos[i] = eco2_lectura[i];
    }

    // Se vacia el array principal
    for (int i = 0; i < 10; i++) {
        eco2_lectura[i] = 0;
    }

    // Resetear indice
    eco2_lectura_indice = 0;

    // Se envia el array nuevo
    return datos;
}

