#pragma once

#include "sensors/magnetometer_sample_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int initialized;
    uint8_t product_id;
} mmc5603_provider_t;

int mmc5603_provider_init(mmc5603_provider_t* provider);
int mmc5603_provider_read(mmc5603_provider_t* provider,
                          uint32_t uptime_ms,
                          magnetometer_sample_c_t* sample);

#ifdef __cplusplus
}
#endif
