/*
 * crc.h
 *
 *  Created on: Apr 7, 2017
 *      Author: petera
 */

#ifndef _CRC_H_
#define _CRC_H_

u32_t crc32(u32_t crc, const void *buf, u32_t size);
u16_t crc16_char(u16_t crc, u8_t data);
u16_t crc16(u16_t crc, u8_t *data, u32_t len);

#endif /* GENERIC_EMBEDDED_SRC_CRC_H_ */
