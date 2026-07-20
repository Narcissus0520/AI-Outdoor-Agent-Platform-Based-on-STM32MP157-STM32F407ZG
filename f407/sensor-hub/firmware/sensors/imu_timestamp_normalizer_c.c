#include "sensors/imu_timestamp_normalizer_c.h"

#include <stdint.h>

void imu_timestamp_normalizer_init(imu_timestamp_normalizer_c_t* normalizer)
{
    if (normalizer == NULL) {
        return;
    }

    normalizer->last_timestamp_ms = 0U;
    normalizer->initialized = 0;
}

int imu_timestamp_normalizer_apply(imu_timestamp_normalizer_c_t* normalizer,
                                   imu_sample_c_t* samples,
                                   size_t sample_count,
                                   uint32_t sample_period_ms)
{
    size_t index;

    if (normalizer == NULL || sample_period_ms == 0U ||
        (samples == NULL && sample_count > 0U)) {
        return -1;
    }
    if (sample_count == 0U) {
        return 0;
    }

    if (normalizer->initialized != 0) {
        const uint32_t shift_ms =
            (normalizer->last_timestamp_ms + sample_period_ms) - samples[0].uptime_ms;
        for (index = 0U; index < sample_count; ++index) {
            samples[index].uptime_ms += shift_ms;
        }
    }

    normalizer->last_timestamp_ms = samples[sample_count - 1U].uptime_ms;
    normalizer->initialized = 1;
    return 0;
}
