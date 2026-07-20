#include "bsp/board_i2c.h"

#include "i2c.h"

#include <stdint.h>

enum {
    BOARD_I2C_TIMEOUT_MS = 30U,
};

static uint32_t g_i2c_recovery_count;
static uint32_t g_i2c_transaction_failure_count;
static int g_i2c_transaction_failure_active;
static uint32_t g_i2c_last_hal_error;
static uint16_t g_i2c_last_length;
static uint8_t g_i2c_last_device_address;
static uint8_t g_i2c_last_register_address;
static uint8_t g_i2c_last_operation;
static uint8_t g_i2c_last_hal_status;

static void record_i2c_failure(uint8_t device_address_7bit,
                               uint8_t register_address,
                               size_t length,
                               uint8_t operation,
                               HAL_StatusTypeDef status)
{
    g_i2c_last_device_address = device_address_7bit;
    g_i2c_last_register_address = register_address;
    g_i2c_last_length = (uint16_t)length;
    g_i2c_last_operation = operation;
    g_i2c_last_hal_status = (uint8_t)status;
    g_i2c_last_hal_error = HAL_I2C_GetError(&hi2c2);
}

int board_i2c_mem_read(uint8_t device_address_7bit, uint8_t register_address, uint8_t* data, size_t length)
{
    HAL_StatusTypeDef status;

    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    status = HAL_I2C_Mem_Read(&hi2c2,
                              (uint16_t)(device_address_7bit << 1U),
                              register_address,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              (uint16_t)length,
                              BOARD_I2C_TIMEOUT_MS);
    if (status == HAL_OK) {
        g_i2c_transaction_failure_active = 0;
        return 0;
    }

    record_i2c_failure(device_address_7bit,
                       register_address,
                       length,
                       BOARD_I2C_OPERATION_READ,
                       status);
    ++g_i2c_recovery_count;
    if (MX_I2C2_Recover() != 0) {
        ++g_i2c_transaction_failure_count;
        g_i2c_transaction_failure_active = 1;
        return -1;
    }

    status = HAL_I2C_Mem_Read(&hi2c2,
                              (uint16_t)(device_address_7bit << 1U),
                              register_address,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              (uint16_t)length,
                              BOARD_I2C_TIMEOUT_MS);
    if (status != HAL_OK) {
        record_i2c_failure(device_address_7bit,
                           register_address,
                           length,
                           BOARD_I2C_OPERATION_READ,
                           status);
        ++g_i2c_transaction_failure_count;
        g_i2c_transaction_failure_active = 1;
        return -1;
    }
    g_i2c_transaction_failure_active = 0;
    return 0;
}

int board_i2c_mem_read_non_replayable(uint8_t device_address_7bit,
                                      uint8_t register_address,
                                      uint8_t* data,
                                      size_t length)
{
    HAL_StatusTypeDef status;

    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    status = HAL_I2C_Mem_Read(&hi2c2,
                              (uint16_t)(device_address_7bit << 1U),
                              register_address,
                              I2C_MEMADD_SIZE_8BIT,
                              data,
                              (uint16_t)length,
                              BOARD_I2C_TIMEOUT_MS);
    if (status == HAL_OK) {
        g_i2c_transaction_failure_active = 0;
        return 0;
    }

    record_i2c_failure(device_address_7bit,
                       register_address,
                       length,
                       BOARD_I2C_OPERATION_READ,
                       status);
    ++g_i2c_recovery_count;
    if (MX_I2C2_Recover() != 0) {
        ++g_i2c_transaction_failure_count;
        g_i2c_transaction_failure_active = 1;
    } else {
        g_i2c_transaction_failure_active = 0;
    }
    return -1;
}

int board_i2c_mem_write(uint8_t device_address_7bit, uint8_t register_address, const uint8_t* data, size_t length)
{
    HAL_StatusTypeDef status;

    if (data == NULL || length == 0U || length > UINT16_MAX) {
        return -1;
    }

    status = HAL_I2C_Mem_Write(&hi2c2,
                               (uint16_t)(device_address_7bit << 1U),
                               register_address,
                               I2C_MEMADD_SIZE_8BIT,
                               (uint8_t*)data,
                               (uint16_t)length,
                               BOARD_I2C_TIMEOUT_MS);
    if (status == HAL_OK) {
        g_i2c_transaction_failure_active = 0;
        return 0;
    }

    record_i2c_failure(device_address_7bit,
                       register_address,
                       length,
                       BOARD_I2C_OPERATION_WRITE,
                       status);
    ++g_i2c_recovery_count;
    if (MX_I2C2_Recover() != 0) {
        ++g_i2c_transaction_failure_count;
        g_i2c_transaction_failure_active = 1;
        return -1;
    }

    status = HAL_I2C_Mem_Write(&hi2c2,
                               (uint16_t)(device_address_7bit << 1U),
                               register_address,
                               I2C_MEMADD_SIZE_8BIT,
                               (uint8_t*)data,
                               (uint16_t)length,
                               BOARD_I2C_TIMEOUT_MS);
    if (status != HAL_OK) {
        record_i2c_failure(device_address_7bit,
                           register_address,
                           length,
                           BOARD_I2C_OPERATION_WRITE,
                           status);
        ++g_i2c_transaction_failure_count;
        g_i2c_transaction_failure_active = 1;
        return -1;
    }
    g_i2c_transaction_failure_active = 0;
    return 0;
}

uint32_t board_i2c_recovery_count(void)
{
    return g_i2c_recovery_count;
}

uint32_t board_i2c_transaction_failure_count(void)
{
    return g_i2c_transaction_failure_count;
}

int board_i2c_transaction_failure_active(void)
{
    return g_i2c_transaction_failure_active;
}

void board_i2c_get_diagnostics(board_i2c_diagnostics_t* diagnostics)
{
    if (diagnostics == NULL) {
        return;
    }

    diagnostics->recovery_count = g_i2c_recovery_count;
    diagnostics->transaction_failure_count = g_i2c_transaction_failure_count;
    diagnostics->last_hal_error = g_i2c_last_hal_error;
    diagnostics->last_length = g_i2c_last_length;
    diagnostics->last_device_address = g_i2c_last_device_address;
    diagnostics->last_register_address = g_i2c_last_register_address;
    diagnostics->last_operation = g_i2c_last_operation;
    diagnostics->last_hal_status = g_i2c_last_hal_status;
}

void board_i2c_reset_diagnostics(void)
{
    g_i2c_recovery_count = 0U;
    g_i2c_transaction_failure_count = 0U;
    g_i2c_transaction_failure_active = 0;
    g_i2c_last_hal_error = 0U;
    g_i2c_last_length = 0U;
    g_i2c_last_device_address = 0U;
    g_i2c_last_register_address = 0U;
    g_i2c_last_operation = BOARD_I2C_OPERATION_NONE;
    g_i2c_last_hal_status = 0U;
}
