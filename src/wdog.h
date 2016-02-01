/*
 * wdog.h
 *
 *  Created on: Feb 1, 2016
 *      Author: petera
 */

#ifndef _WDOG_H_
#define _WDOG_H_

void WDOG_init(void);
void WDOG_start(u32_t seconds);
void WDOG_feed(void);

#endif /* _WDOG_H_ */
