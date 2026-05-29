#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "sensor_bus.h"

#define MPU9250_SENSOR_ADDR         0x68
#define MPU9250_WHO_AM_I_REG_ADDR   0x75
#define MPU9250_PWR_MGMT_1_REG_ADDR 0x6B
#define MPU9250_WHO_AM_I_EXPECTED   0x71
#define MPU9250_RESET_VALUE         0x80

typedef enum {
    MPU9250_STATUS_OK = 0,
    MPU9250_STATUS_BUS_ERROR,
    MPU9250_STATUS_UNEXPECTED_ID,
} mpu9250_status_t;

typedef struct {
    uint8_t who_am_i;
    mpu9250_status_t status;
    esp_err_t bus_error;
} mpu9250_probe_result_t;

esp_err_t mpu9250_read_who_am_i(const sensor_bus_t *bus, uint8_t *who_am_i);
esp_err_t mpu9250_reset(const sensor_bus_t *bus);
mpu9250_probe_result_t mpu9250_probe(const sensor_bus_t *bus);
const char *mpu9250_status_to_string(mpu9250_status_t status);
