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

#ifndef DEBUG__
#define DEBUG__

void usb_debug_enable();
__bit usb_debug_enabled();
void usb_debug_disable();

#define DECLARE_USB_PRINTF(port) 		\
int __usb_printf_##port(const char *format, ...)


DECLARE_USB_PRINTF(0);
DECLARE_USB_PRINTF(2);
DECLARE_USB_PRINTF(4);
DECLARE_USB_PRINTF(6);
DECLARE_USB_PRINTF(8);

#define USB_PRINTF(port,format,...)					\
__usb_printf_##port(format, ## __VA_ARGS__)				\

#define USB_DEBUG_PRINTF(port,format,...)				\
if(usb_debug_enabled())__usb_printf_##port(format, ## __VA_ARGS__)	\


#ifdef DEBUG_FIRMWARE
#include <serial.h>
#include <stdio.h>
#else
#include <stdio.h>
#define printf(...)
#endif

#endif //DEBUG__
