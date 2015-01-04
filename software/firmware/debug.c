/*
AdslSniffer
Copyright (C) 2013  Yann Diorcet <diorcet.yann@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "debug.h"
#include "utils.h"
#include <fx2macros.h>
#include <stdio.h>
#include <string.h>

static __bit usb_debug = 0;

void usb_debug_enable() {
  usb_debug = 1;
}

__bit usb_debug_enabled() {
  return usb_debug;
}

void usb_debug_disable() {
  usb_debug = 0;
}

#define DEFINE_USB_PRINTF_S(port)			\
int __usb_printf_##port(const char *format, ...) {	\
	int len; 					\
	int ret; 					\
	va_list argptr; 				\
	char *dest = EPxBUF(port); 			\
							\
	/* Format */					\
	dest[0]='\0';					\
	va_start(argptr, format);			\
	ret = vsprintf(dest, format, argptr);		\
	va_end(argptr);					\
							\
	/* Send */					\
	len = strlen(dest);				\
	if(len > 0) {					\
		len++;					\
							\
		/* ARM epx out */			\
		EPxBCH(port)=MSB(len);			\
		EPxBCL(port)=LSB(len);			\
	}						\
	return ret;					\
}

#define DEFINE_USB_PRINTF(port)				\
int __usb_printf_##port(const char *format, ...) {	\
	int len; 					\
	int ret; 					\
	va_list argptr; 				\
	char *dest = EPxFIFOBUF(port); 			\
							\
	/* Wait */					\
	while(EP2468STAT & bmEPxFULL(port)) {		\
		__asm					\
		nop					\
		nop					\
		nop					\
		nop					\
		nop					\
		nop					\
		nop					\
		__endasm;				\
	}						\
							\
	/* Format */					\
	dest[0]='\0';					\
	va_start(argptr, format);			\
	ret = vsprintf(dest, format, argptr);		\
	va_end(argptr);					\
							\
	/* Send */					\
	len = strlen(dest);				\
	if(len > 0) {					\
		len++;					\
							\
		/* ARM epx out */			\
		EPxBCH(port)=MSB(len);			\
		SYNCDELAY();				\
		EPxBCL(port)=LSB(len);			\
	}						\
	return ret;					\
}

DEFINE_USB_PRINTF_S(0)
DEFINE_USB_PRINTF(2)
DEFINE_USB_PRINTF(4)
DEFINE_USB_PRINTF(6)
DEFINE_USB_PRINTF(8)
