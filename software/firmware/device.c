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

#include <fx2macros.h>
#include <delay.h>
#include <setupdat.h>
#include "utils.h"
#include "debug.h"

void reset();

//************************** Configuration Handlers *****************************

// change to support as many interfaces as you need
volatile __xdata BYTE interface=0;
volatile __xdata BYTE alt=0; // alt interface

// set *alt_ifc to the current alt interface for ifc
BOOL handle_get_interface(BYTE ifc, BYTE* alt_ifc) {
	if(ifc == 0) {
		*alt_ifc = alt;
	}
	return TRUE;
}

// return TRUE if you set the interface requested
// NOTE this function should reconfigure and reset the endpoints
// according to the interface descriptors you provided.
BOOL handle_set_interface(BYTE ifc, BYTE alt_ifc) {  
	interface = ifc;
	alt = alt_ifc;
	return TRUE;
}

// handle getting and setting the configuration
// 1 is the default.  If you support more than one config
// keep track of the config number and return the correct number
// config numbers are set int the dscr file.
volatile BYTE config = 1;
BYTE handle_get_configuration() { 
	return 1;
}

// NOTE changing config requires the device to reset all the endpoints
BOOL handle_set_configuration(BYTE cfg) { 
	config = cfg;
	return TRUE;
}


//******************* VENDOR COMMAND HANDLERS **************************
__bit bench_start = 0;
long count = 0;
BOOL handle_vendorcommand(BYTE cmd) {
	if(cmd == 0x90) {
		reset();
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x91) {
		short val = SETUP_VALUE();
		if(val != 0) {
			usb_debug_enable();
			USB_PRINTF(6, "Debug enabled");
		} else {
			usb_debug_disable();
		}
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x92) {
		USB_PRINTF(6, "Start");
		IOA &= ~(0x1 << 3); // Low 
		count = 0;
		bench_start = 1;
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x93) {
		USB_PRINTF(6, "Stop");
		IOA |= (0x1 << 3); // High
		bench_start = 0;
		FIFORESET = bmNAKALL; SYNCDELAY();
		FIFORESET = bmNAKALL | 2; SYNCDELAY();  // reset EP2
		FIFORESET = 0x00; SYNCDELAY();
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x94) {
		__bit enabled  = usb_debug_enabled();
		if(!enabled) {
			usb_debug_enable();
		}
		USB_PRINTF(6, "AdslSniffer V0.0.1");
		if(!enabled) {
			usb_debug_disable();
		}
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	}else if(cmd == 0x99) {
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		USB_PRINTF(6, "Test");
		return TRUE;
	}
	return FALSE;
}

void reset() {
	bench_start = 0;
	count = 0;
	EP2CS &= ~bmBIT0; SYNCDELAY();// remove stall bit
	EP6CS &= ~bmBIT0; SYNCDELAY();// remove stall bit
 
	// Reset all the FIFOs
	FIFORESET = bmNAKALL; SYNCDELAY(); 
	FIFORESET = 2; SYNCDELAY();   // reset EP2
	FIFORESET = 6; SYNCDELAY();   // reset EP6
	FIFORESET = 0x00; SYNCDELAY(); 
 
	EP2FIFOCFG &= ~bmBIT0; SYNCDELAY();// not worldwide
	EP6FIFOCFG &= ~bmBIT0; SYNCDELAY();// not worldwide
	
	IOA |= (0x1 << 3); // High at start
	OEA |= (0x1 << 3); // OEA 3 Output
 
	usb_debug_disable();
}

//********************  INIT ***********************

void main_init() {
	REVCTL = 0x03;
	SYNCDELAY(); 
	PORTACFG = 0x00;
	SETIF48MHZ();
	SYNCDELAY(); 
  
	reset();

	// set IFCONFIG
	EP1INCFG &= ~bmVALID; SYNCDELAY(); 
	EP1OUTCFG &= ~bmVALID;
	EP2CFG = (bmVALID | bmBULK | bmBUF4X /*| bmBUF1024*/ | bmDIR); SYNCDELAY();
	EP4CFG &= ~bmVALID; /* = (bmVALID | bmISO | bmBUF2X | bmDIR);*/ SYNCDELAY(); 
	EP6CFG = (bmVALID | bmBULK | bmBUF2X | bmDIR); SYNCDELAY(); 
	EP8CFG &= ~bmVALID;  SYNCDELAY();
}

#define BUFF_SIZE (512)
void main_loop() {
	if(bench_start) {
		while((EP2468STAT & bmEP2FULL) == 0) {
			EP2FIFOBUF[0] = LSB(count);
			EP2FIFOBUF[1] = MSB(count);
			EP2FIFOBUF[BUFF_SIZE-3] = MICROFRAME;
			EP2FIFOBUF[BUFF_SIZE-2] = USBFRAMEL;
			EP2FIFOBUF[BUFF_SIZE-1] = USBFRAMEH;

			// ARM ep2 in
			EP2BCH = MSB(BUFF_SIZE);
			SYNCDELAY();
			EP2BCL = LSB(BUFF_SIZE);
			SYNCDELAY();
			++count;
		}
	}
}
