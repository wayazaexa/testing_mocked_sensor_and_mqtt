/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_net.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "mpu9250.h"
#include "sensor_bus.h"

static const char *TAG = "example";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

typedef struct {
    i2c_master_dev_handle_t dev_handle;
} i2c_sensor_bus_ctx_t;

static esp_err_t i2c_sensor_bus_read_reg(void *ctx,
                                         uint8_t dev_addr,
                                         uint8_t reg_addr,
                                         uint8_t *data,
                                         size_t len)
{
    (void)dev_addr;
    i2c_sensor_bus_ctx_t *i2c_ctx = (i2c_sensor_bus_ctx_t *)ctx;
    return i2c_master_transmit_receive(i2c_ctx->dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t i2c_sensor_bus_write_reg(void *ctx, uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    (void)dev_addr;
    i2c_sensor_bus_ctx_t *i2c_ctx = (i2c_sensor_bus_ctx_t *)ctx;
    uint8_t write_buf[2] = {reg_addr, value};
    return i2c_master_transmit(i2c_ctx->dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU9250_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    i2c_sensor_bus_ctx_t i2c_ctx = {
        .dev_handle = dev_handle,
    };
    sensor_bus_t sensor_bus = {
        .ctx = &i2c_ctx,
        .read_reg = i2c_sensor_bus_read_reg,
        .write_reg = i2c_sensor_bus_write_reg,
    };

    mpu9250_probe_result_t probe = mpu9250_probe(&sensor_bus);
    ESP_LOGI(TAG,
             "MPU9250 probe status=%s WHO_AM_I=0x%02X bus_error=%s",
             mpu9250_status_to_string(probe.status),
             probe.who_am_i,
             esp_err_to_name(probe.bus_error));

    if (probe.status == MPU9250_STATUS_OK) {
        ESP_ERROR_CHECK(mpu9250_reset(&sensor_bus));
        ESP_LOGI(TAG, "MPU9250 reset command sent");
    }

#if CONFIG_EXAMPLE_ENABLE_MQTT_TELEMETRY
    ESP_ERROR_CHECK(app_net_start());
    ESP_ERROR_CHECK(app_net_publish_mpu9250_probe(1, &probe));
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
