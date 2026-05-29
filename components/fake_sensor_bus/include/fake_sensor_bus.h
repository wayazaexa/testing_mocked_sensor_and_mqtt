#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "sensor_bus.h"

#define FAKE_SENSOR_BUS_MAX_READS 8
#define FAKE_SENSOR_BUS_MAX_WRITES 8

typedef struct {
    uint8_t dev_addr;
    uint8_t reg_addr;
    uint8_t value;
} fake_sensor_bus_write_t;

typedef struct {
    uint8_t dev_addr;
    uint8_t reg_addr;
    uint8_t values[FAKE_SENSOR_BUS_MAX_READS];
    size_t value_count;
    size_t read_count;
    esp_err_t read_error;
    esp_err_t write_error;
    fake_sensor_bus_write_t writes[FAKE_SENSOR_BUS_MAX_WRITES];
    size_t write_count;
} fake_sensor_bus_t;

void fake_sensor_bus_init(fake_sensor_bus_t *fake, uint8_t dev_addr, uint8_t reg_addr);
sensor_bus_t fake_sensor_bus_as_bus(fake_sensor_bus_t *fake);
void fake_sensor_bus_set_read_values(fake_sensor_bus_t *fake, const uint8_t *values, size_t value_count);
void fake_sensor_bus_set_read_error(fake_sensor_bus_t *fake, esp_err_t error);
void fake_sensor_bus_set_write_error(fake_sensor_bus_t *fake, esp_err_t error);
const fake_sensor_bus_write_t *fake_sensor_bus_last_write(const fake_sensor_bus_t *fake);
