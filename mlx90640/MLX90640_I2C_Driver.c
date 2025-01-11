/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "MLX90640_I2C_Driver.h"

#include <stdio.h>

const I2CConfig i2c_cfg = {
    .timingr = STM32_TIMINGR_PRESC(2) | STM32_TIMINGR_SCLDEL(9U) |
               STM32_TIMINGR_SDADEL(0U) | STM32_TIMINGR_SCLH(112U) |
               STM32_TIMINGR_SCLL(162U),
    .cr1 = 0,
    .cr2 = 0};

void MLX90640_I2CInit() { i2cStart(&I2CD4, &i2c_cfg); }

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress,
                     uint16_t nMemAddressRead, uint16_t *data) {
  uint8_t *p = (uint8_t *)data;
  int cnt = 0;

  // ack = HAL_I2C_Mem_Read(&hi2c1, (slaveAddr<<1), startAddress,
  // I2C_MEMADD_SIZE_16BIT, p, nMemAddressRead*2, 500);
  uint8_t tx_buf[] = {startAddress >> 8, startAddress & 0xFF};
  msg_t ack = i2cMasterTransmitTimeout(&I2CD4, slaveAddr, tx_buf, 2, p,
                                       nMemAddressRead * 2, TIME_MS2I(50));

  if (ack != MSG_OK) {
    return -1;
  }

  for (cnt = 0; cnt < nMemAddressRead * 2; cnt += 2) {
    uint8_t tempBuffer = p[cnt + 1];
    p[cnt + 1] = p[cnt];
    p[cnt] = tempBuffer;
  }

  return 0;
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
  uint8_t cmd[2];
  static uint16_t dataCheck;

  cmd[0] = data >> 8;
  cmd[1] = data & 0x00FF;

  // ack = HAL_I2C_Mem_Write(&hi2c1, sa, writeAddress, I2C_MEMADD_SIZE_16BIT,
  // cmd, sizeof(cmd), 500);
  msg_t ack = i2cMasterTransmitTimeout(&I2CD4, slaveAddr, cmd, 2, NULL, 0,
                                       TIME_MS2I(50));

  if (ack != MSG_OK) {
    return -1;
  }

  MLX90640_I2CRead(slaveAddr, writeAddress, 1, &dataCheck);

  if (dataCheck != data) {
    return -2;
  }

  return 0;
}