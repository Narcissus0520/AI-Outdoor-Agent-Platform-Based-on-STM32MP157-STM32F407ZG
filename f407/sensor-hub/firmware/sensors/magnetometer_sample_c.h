#pragma once

#include <stdint.h>

typedef struct {
    uint32_t uptime_ms;
    int32_t magnetic_x_nt;
    int32_t magnetic_y_nt;
    int32_t magnetic_z_nt;
} magnetometer_sample_c_t;
