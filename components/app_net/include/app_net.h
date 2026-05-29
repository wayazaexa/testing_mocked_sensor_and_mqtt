#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "mpu9250.h"

esp_err_t app_net_start(void);
esp_err_t app_net_publish_mpu9250_probe(uint32_t seq, const mpu9250_probe_result_t *result);
