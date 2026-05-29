#include "fake_sensor_bus.h"

#include <string.h>

static esp_err_t fake_read_reg(void *ctx, uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    fake_sensor_bus_t *fake = (fake_sensor_bus_t *)ctx;
    if (fake == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (fake->read_error != ESP_OK) {
        return fake->read_error;
    }

    if (dev_addr != fake->dev_addr || reg_addr != fake->reg_addr) {
        return ESP_ERR_NOT_FOUND;
    }

    size_t index = fake->read_count;
    if (index >= fake->value_count) {
        index = fake->value_count > 0 ? fake->value_count - 1 : 0;
    }

    memset(data, 0, len);
    if (fake->value_count > 0) {
        data[0] = fake->values[index];
    }
    fake->read_count++;
    return ESP_OK;
}

static esp_err_t fake_write_reg(void *ctx, uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    fake_sensor_bus_t *fake = (fake_sensor_bus_t *)ctx;
    if (fake == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (fake->write_error != ESP_OK) {
        return fake->write_error;
    }

    if (fake->write_count < FAKE_SENSOR_BUS_MAX_WRITES) {
        fake->writes[fake->write_count] = (fake_sensor_bus_write_t) {
            .dev_addr = dev_addr,
            .reg_addr = reg_addr,
            .value = value,
        };
        fake->write_count++;
    }

    return ESP_OK;
}

void fake_sensor_bus_init(fake_sensor_bus_t *fake, uint8_t dev_addr, uint8_t reg_addr)
{
    if (fake == NULL) {
        return;
    }

    memset(fake, 0, sizeof(*fake));
    fake->dev_addr = dev_addr;
    fake->reg_addr = reg_addr;
    fake->read_error = ESP_OK;
    fake->write_error = ESP_OK;
}

sensor_bus_t fake_sensor_bus_as_bus(fake_sensor_bus_t *fake)
{
    return (sensor_bus_t) {
        .ctx = fake,
        .read_reg = fake_read_reg,
        .write_reg = fake_write_reg,
    };
}

void fake_sensor_bus_set_read_values(fake_sensor_bus_t *fake, const uint8_t *values, size_t value_count)
{
    if (fake == NULL || values == NULL) {
        return;
    }

    if (value_count > FAKE_SENSOR_BUS_MAX_READS) {
        value_count = FAKE_SENSOR_BUS_MAX_READS;
    }

    memcpy(fake->values, values, value_count);
    fake->value_count = value_count;
    fake->read_count = 0;
}

void fake_sensor_bus_set_read_error(fake_sensor_bus_t *fake, esp_err_t error)
{
    if (fake != NULL) {
        fake->read_error = error;
    }
}

void fake_sensor_bus_set_write_error(fake_sensor_bus_t *fake, esp_err_t error)
{
    if (fake != NULL) {
        fake->write_error = error;
    }
}

const fake_sensor_bus_write_t *fake_sensor_bus_last_write(const fake_sensor_bus_t *fake)
{
    if (fake == NULL || fake->write_count == 0) {
        return NULL;
    }

    return &fake->writes[fake->write_count - 1];
}
