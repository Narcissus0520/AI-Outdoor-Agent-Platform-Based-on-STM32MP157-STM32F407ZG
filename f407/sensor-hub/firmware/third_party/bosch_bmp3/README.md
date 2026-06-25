# Bosch BMP3 Sensor API

This directory contains Bosch Sensortec BMP3 Sensor API v2.0.5 files copied
from the user-provided BMP390 STM32 example.

- Upstream files: `bmp3.c`, `bmp3.h`, `bmp3_defs.h`
- License: BSD-3-Clause, retained in the source headers
- Local integration: `firmware/sensors/bmp390_provider_c.*`

The upstream files are kept unmodified. Project-specific I2C and timing logic
stays in the local provider.
