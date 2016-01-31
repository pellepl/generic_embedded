/*
 * hmc5883l_hw.h
 *
 *  Created on: Dec 29, 2015
 *      Author: petera
 */

#ifndef HMC5883L_HW_H_
#define HMC5883L_HW_H_

#define HMC5883L_R_CFG_A    0x00
#define HMC5883L_R_CFG_B    0x01
#define HMC5883L_R_MODE     0x02
#define HMC5883L_R_X_MSB    0x03
#define HMC5883L_R_X_LSB    0x04
#define HMC5883L_R_Y_MSB    0x05
#define HMC5883L_R_Y_LSB    0x06
#define HMC5883L_R_Z_MSB    0x07
#define HMC5883L_R_Z_LSB    0x08
#define HMC5883L_R_SR       0x09
#define HMC5883L_R_ID_A     0x0a
#define HMC5883L_R_ID_B     0x0b
#define HMC5883L_R_ID_C     0x0c

#define HMC5883L_R_CFG_A_MA_POS   5
#define HMC5883L_R_CFG_A_MA_MASK  (0b11 << HMC5883L_R_CFG_A_MA_POS)
#define HMC5883L_R_CFG_A_DO_POS   2
#define HMC5883L_R_CFG_A_DO_MASK  (0b111 << HMC5883L_R_CFG_A_DO_POS)
#define HMC5883L_R_CFG_A_MS_POS   0
#define HMC5883L_R_CFG_A_MS_MASK  (0b11 << HMC5883L_R_CFG_A_MS_POS)

#define HMC5883L_R_CFG_B_GN_POS   5
#define HMC5883L_R_CFG_B_GN_MASK  (0b111 << HMC5883L_R_CFG_B_GN_POS)

#define HMC5883L_R_MODE_HS_POS    7
#define HMC5883L_R_MODE_HS_MASK   (0b1 << HMC5883L_R_MODE_HS_POS)
#define HMC5883L_R_MODE_MD_POS    0
#define HMC5883L_R_MODE_MD_MASK   (0b11 << HMC5883L_R_MODE_MD_POS)

#define HMC5883L_R_SR_LOCK_POS    1
#define HMC5883L_R_SR_LOCK_MASK   (0b1 << HMC5883L_R_SR_LOCK_POS)
#define HMC5883L_R_SR_RDY_POS     0
#define HMC5883L_R_SR_RDY_MASK    (0b1 << HMC5883L_R_SR_RDY_POS)

#define HMC5883L_ID_A       0b01001000  /* H */
#define HMC5883L_ID_B       0b00110100  /* 4 */
#define HMC5883L_ID_C       0b00110011  /* 3 */

#define HMC5883L_I2C_ADDR   0x3c

#endif /* HMC5883L_HW_H_ */
