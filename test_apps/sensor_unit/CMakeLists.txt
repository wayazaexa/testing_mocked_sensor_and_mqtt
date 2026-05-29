cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
    "../../components/sensor_bus"
    "../../components/mpu9250"
    "../../components/fake_sensor_bus")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sensor_unit_tests)
