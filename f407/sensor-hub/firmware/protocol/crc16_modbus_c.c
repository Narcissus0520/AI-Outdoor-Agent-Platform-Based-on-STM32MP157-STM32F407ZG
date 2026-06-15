#include "protocol/crc16_modbus_c.h"

uint16_t crc16_modbus_c(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFFU;
    size_t index;

    if (data == 0 && length > 0U) {
        return crc;
    }

    for (index = 0U; index < length; ++index) {
        int bit;
        crc ^= data[index];
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc = (uint16_t)(crc >> 1U);
            }
        }
    }

    return crc;
}
