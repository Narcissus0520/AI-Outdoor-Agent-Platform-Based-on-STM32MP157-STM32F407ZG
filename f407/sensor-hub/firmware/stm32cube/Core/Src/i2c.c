#include "i2c.h"

#ifndef SENSOR_HUB_I2C2_CLOCK_HZ
#define SENSOR_HUB_I2C2_CLOCK_HZ 400000U
#endif

I2C_HandleTypeDef hi2c2;

static int I2C2_BusRecovery(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t pulse;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_SET);
  HAL_Delay(1);

  for (pulse = 0; pulse < 18U && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET; ++pulse)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(1);
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
  HAL_Delay(1);

  return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_SET &&
         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_SET
             ? 0
             : -1;
}

static HAL_StatusTypeDef I2C2_InitPeripheral(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = SENSOR_HUB_I2C2_CLOCK_HZ;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  return HAL_I2C_Init(&hi2c2);
}

void MX_I2C2_Init(void)
{
  (void)I2C2_BusRecovery();
  (void)I2C2_InitPeripheral();
}

int MX_I2C2_Recover(void)
{
  (void)HAL_I2C_DeInit(&hi2c2);
  __HAL_RCC_I2C2_FORCE_RESET();
  __HAL_RCC_I2C2_RELEASE_RESET();
  (void)I2C2_BusRecovery();
  if (I2C2_InitPeripheral() != HAL_OK) {
    return -1;
  }
  return 0;
}
