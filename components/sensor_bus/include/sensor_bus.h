#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct sensor_bus {
    void *ctx;
    esp_err_t (*read_reg)(void *ctx, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);
    esp_err_t (*write_reg)(void *ctx, uint8_t dev_addr, uint8_t reg_addr, uint8_t value);
} sensor_bus_t;

static inline esp_err_t sensor_bus_read_reg(const sensor_bus_t *bus,
                                            uint8_t dev_addr,
                                            uint8_t reg_addr,
                                            uint8_t *data,
                                            size_t len)
{
    if (bus == NULL || bus->read_reg == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return bus->read_reg(bus->ctx, dev_addr, reg_addr, data, len);
}

static inline esp_err_t sensor_bus_write_reg(const sensor_bus_t *bus,
                                             uint8_t dev_addr,
                                             uint8_t reg_addr,
                                             uint8_t value)
{
    if (bus == NULL || bus->write_reg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return bus->write_reg(bus->ctx, dev_addr, reg_addr, value);
}
