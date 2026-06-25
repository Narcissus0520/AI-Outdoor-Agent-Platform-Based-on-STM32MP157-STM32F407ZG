#pragma once

#include "sensors/barometer_sample_c.h"
#include "third_party/bosch_bmp3/bmp3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct bmp3_dev device;
    struct bmp3_settings settings;
    uint8_t i2c_address;
    int initialized;
} bmp390_provider_t;

int bmp390_provider_init(bmp390_provider_t* provider);
int bmp390_provider_read(bmp390_provider_t* provider,
                         uint32_t uptime_ms,
                         barometer_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
