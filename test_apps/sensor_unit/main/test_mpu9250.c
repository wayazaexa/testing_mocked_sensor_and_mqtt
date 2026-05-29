#include "fake_sensor_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu9250.h"
#include "unity.h"
#include "unity_test_runner.h"

static sensor_bus_t make_bus_with_values(fake_sensor_bus_t *fake, const uint8_t *values, size_t value_count)
{
    fake_sensor_bus_init(fake, MPU9250_SENSOR_ADDR, MPU9250_WHO_AM_I_REG_ADDR);
    fake_sensor_bus_set_read_values(fake, values, value_count);
    return fake_sensor_bus_as_bus(fake);
}

TEST_CASE("WHO_AM_I returns expected value", "[mpu9250]")
{
    fake_sensor_bus_t fake;
    const uint8_t values[] = {MPU9250_WHO_AM_I_EXPECTED};
    sensor_bus_t bus = make_bus_with_values(&fake, values, 1);

    uint8_t who_am_i = 0;
    TEST_ASSERT_EQUAL(ESP_OK, mpu9250_read_who_am_i(&bus, &who_am_i));
    TEST_ASSERT_EQUAL_HEX8(MPU9250_WHO_AM_I_EXPECTED, who_am_i);
    TEST_ASSERT_EQUAL_UINT(1, fake.read_count);
}

TEST_CASE("probe reports unexpected WHO_AM_I value", "[mpu9250]")
{
    fake_sensor_bus_t fake;
    const uint8_t values[] = {0x00};
    sensor_bus_t bus = make_bus_with_values(&fake, values, 1);

    mpu9250_probe_result_t result = mpu9250_probe(&bus);

    TEST_ASSERT_EQUAL(MPU9250_STATUS_UNEXPECTED_ID, result.status);
    TEST_ASSERT_EQUAL_HEX8(0x00, result.who_am_i);
    TEST_ASSERT_EQUAL(ESP_OK, result.bus_error);
}

TEST_CASE("probe propagates I2C read error", "[mpu9250]")
{
    fake_sensor_bus_t fake;
    const uint8_t values[] = {MPU9250_WHO_AM_I_EXPECTED};
    sensor_bus_t bus = make_bus_with_values(&fake, values, 1);
    fake_sensor_bus_set_read_error(&fake, ESP_ERR_TIMEOUT);

    mpu9250_probe_result_t result = mpu9250_probe(&bus);

    TEST_ASSERT_EQUAL(MPU9250_STATUS_BUS_ERROR, result.status);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, result.bus_error);
}

TEST_CASE("reset writes power management reset value", "[mpu9250]")
{
    fake_sensor_bus_t fake;
    fake_sensor_bus_init(&fake, MPU9250_SENSOR_ADDR, MPU9250_WHO_AM_I_REG_ADDR);
    sensor_bus_t bus = fake_sensor_bus_as_bus(&fake);

    TEST_ASSERT_EQUAL(ESP_OK, mpu9250_reset(&bus));

    const fake_sensor_bus_write_t *write = fake_sensor_bus_last_write(&fake);
    TEST_ASSERT_NOT_NULL(write);
    TEST_ASSERT_EQUAL_HEX8(MPU9250_SENSOR_ADDR, write->dev_addr);
    TEST_ASSERT_EQUAL_HEX8(MPU9250_PWR_MGMT_1_REG_ADDR, write->reg_addr);
    TEST_ASSERT_EQUAL_HEX8(MPU9250_RESET_VALUE, write->value);
}

TEST_CASE("fake bus can replay sensor read sequence", "[fake_sensor_bus]")
{
    fake_sensor_bus_t fake;
    const uint8_t values[] = {0x70, MPU9250_WHO_AM_I_EXPECTED};
    sensor_bus_t bus = make_bus_with_values(&fake, values, 2);

    uint8_t who_am_i = 0;
    TEST_ASSERT_EQUAL(ESP_OK, mpu9250_read_who_am_i(&bus, &who_am_i));
    TEST_ASSERT_EQUAL_HEX8(0x70, who_am_i);

    TEST_ASSERT_EQUAL(ESP_OK, mpu9250_read_who_am_i(&bus, &who_am_i));
    TEST_ASSERT_EQUAL_HEX8(MPU9250_WHO_AM_I_EXPECTED, who_am_i);
}

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_all_tests();
    int failures = UNITY_END();

    printf("Sensor unit test run finished with %d failure(s).\n", failures);
    vTaskDelay(pdMS_TO_TICKS(100));
}
