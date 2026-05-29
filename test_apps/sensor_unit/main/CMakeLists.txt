idf_component_register(SRCS "test_mpu9250.c"
                       INCLUDE_DIRS "."
                       REQUIRES unity mpu9250 fake_sensor_bus)
