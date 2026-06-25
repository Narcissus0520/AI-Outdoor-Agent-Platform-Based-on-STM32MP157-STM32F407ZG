#pragma once

#include <stdint.h>

typedef struct {
    uint32_t uptime_ms;
    uint32_t pressure_pa;
    int16_t temperature_centi_c;
} barometer_sample_c_t;
