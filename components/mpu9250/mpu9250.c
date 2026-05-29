#include "mpu9250.h"

#include "esp_err.h"

esp_err_t mpu9250_read_who_am_i(const sensor_bus_t *bus, uint8_t *who_am_i)
{
    if (who_am_i == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return sensor_bus_read_reg(bus, MPU9250_SENSOR_ADDR, MPU9250_WHO_AM_I_REG_ADDR, who_am_i, 1);
}

esp_err_t mpu9250_reset(const sensor_bus_t *bus)
{
    return sensor_bus_write_reg(bus, MPU9250_SENSOR_ADDR, MPU9250_PWR_MGMT_1_REG_ADDR, MPU9250_RESET_VALUE);
}

mpu9250_probe_result_t mpu9250_probe(const sensor_bus_t *bus)
{
    mpu9250_probe_result_t result = {
        .who_am_i = 0,
        .status = MPU9250_STATUS_OK,
        .bus_error = ESP_OK,
    };

    result.bus_error = mpu9250_read_who_am_i(bus, &result.who_am_i);
    if (result.bus_error != ESP_OK) {
        result.status = MPU9250_STATUS_BUS_ERROR;
        return result;
    }

    if (result.who_am_i != MPU9250_WHO_AM_I_EXPECTED) {
        result.status = MPU9250_STATUS_UNEXPECTED_ID;
    }

    return result;
}

const char *mpu9250_status_to_string(mpu9250_status_t status)
{
    switch (status) {
    case MPU9250_STATUS_OK:
        return "ok";
    case MPU9250_STATUS_BUS_ERROR:
        return "bus_error";
    case MPU9250_STATUS_UNEXPECTED_ID:
        return "unexpected_id";
    default:
        return "unknown";
    }
}
