// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/serial.h
 */

#ifndef __LOLI_SERIAL_H_INC__
#define __LOLI_SERIAL_H_INC__

void serial_init(void);
void serial_write(const char *buf);
extern int gSerialAvailable;

#endif /* __LOLI_SERIAL_H_INC__ */
