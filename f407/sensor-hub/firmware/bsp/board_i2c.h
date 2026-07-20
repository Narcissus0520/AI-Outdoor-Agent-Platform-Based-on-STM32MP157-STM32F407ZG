#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BOARD_I2C_OPERATION_NONE = 0U,
    BOARD_I2C_OPERATION_READ = 1U,
    BOARD_I2C_OPERATION_WRITE = 2U,
};

typedef struct {
    uint32_t recovery_count;
    uint32_t transaction_failure_count;
    uint32_t last_hal_error;
    uint16_t last_length;
    uint8_t last_device_address;
    uint8_t last_register_address;
    uint8_t last_operation;
    uint8_t last_hal_status;
} board_i2c_diagnostics_t;

int board_i2c_mem_read(uint8_t device_address_7bit, uint8_t register_address, uint8_t* data, size_t length);
int board_i2c_mem_read_non_replayable(uint8_t device_address_7bit,
                                      uint8_t register_address,
                                      uint8_t* data,
                                      size_t length);
int board_i2c_mem_write(uint8_t device_address_7bit, uint8_t register_address, const uint8_t* data, size_t length);
uint32_t board_i2c_recovery_count(void);
uint32_t board_i2c_transaction_failure_count(void);
int board_i2c_transaction_failure_active(void);
void board_i2c_get_diagnostics(board_i2c_diagnostics_t* diagnostics);
void board_i2c_reset_diagnostics(void);

#ifdef __cplusplus
}
#endif
