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
#include <fx2timer.h>
#include <fx2ints.h>
#include <delay.h>
#include <setupdat.h>
#include "utils.h"
#include "debug.h"

#define SIMULATION
#define SIG_EN (0x1 << 7)

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

BOOL handle_get_descriptor() {
	return FALSE;
}


//******************* VENDOR COMMAND HANDLERS **************************

BOOL handle_vendorcommand(BYTE cmd) {
	if(cmd == 0x90) { // Reset
		reset();
		
		// Nothing to reply
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x91) { // Enable/Disable debug
		short val = SETUP_VALUE();
		if(val != 0) {
			usb_debug_enable();
			USB_DEBUG_PRINTF(6, "Debug enabled");
		} else {
			usb_debug_disable();
		}
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x92) { // Start stream
		USB_DEBUG_PRINTF(6, "Start");
		IOA &= ~SIG_EN; // Low
		
		// Nothing to reply
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		
#ifdef SIMULATION
		fx2_setup_timer0(29);
#else
		IFCONFIG |= bmIFFIFO;
#endif
		return TRUE;
	} else if(cmd == 0x93) { // Stop stream
		USB_DEBUG_PRINTF(6, "Stop");
		IOA |= SIG_EN; // High
		
		// Reset EP2
		FIFORESET = bmNAKALL; SYNCDELAY();
		FIFORESET = bmNAKALL | 2; SYNCDELAY();
		FIFORESET = 0x00; SYNCDELAY();
		
		// Nothing to reply
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
#ifdef SIMULATION
		fx2_setup_timer0(0);
#else
		IFCONFIG &= ~bmIFFIFO;
#endif
		return TRUE;
	} else if(cmd == 0x94) { // Get version
 		USB_PRINTF(0, "AdslSniffer V0.0.1");
		EP0CS |= bmHSNAK;
 		return TRUE;
	} else if(cmd == 0x95) { // Get bitrate
		WORD size = sizeof(DWORD);
		DWORD *rate = (DWORD*)EP0BUF;
		*rate = 8832000;
		EP0BCH = MSB(size);
		EP0BCL = LSB(size);
		EP0CS |= bmHSNAK;
		return TRUE;
	} else if(cmd == 0x99) { // Test debug EP
		USB_DEBUG_PRINTF(6, "Test");
		EP0BCH = 0;
		EP0BCL = 0;
		EP0CS |= bmHSNAK;
		return TRUE;
	}
	return FALSE;
}

void reset() {
	EP2CS &= ~bmBIT0; SYNCDELAY();// remove stall bit
	EP6CS &= ~bmBIT0; SYNCDELAY();// remove stall bit
 
	// Reset all the FIFOs
	FIFORESET = bmNAKALL; SYNCDELAY(); 
	FIFORESET = bmNAKALL | 2; SYNCDELAY();   // reset EP2
	FIFORESET = bmNAKALL | 6; SYNCDELAY();   // reset EP6
	FIFORESET = 0x00; SYNCDELAY(); 
 
	EP2FIFOCFG &= ~bmBIT0; SYNCDELAY();// not worldwide
	EP6FIFOCFG &= ~bmBIT0; SYNCDELAY();// not worldwide
	
	IOA |= SIG_EN; // High at start
	OEA |= SIG_EN; // OEA 3 Output

	DISABLE_TIMER0();
 
	usb_debug_disable();
}

// PKTEND(PA6) = 0
// SLOE(PA2) = x // Don't care in IN mode
// FIFOADR0(PA4) = 0 // EP2
// FIFOADR1(PA5)  = 0 // EP2
// CLK(RDY1) = CLK
// PA7 = EN
// PB0-7 = D0-7
// PD0-7 = D8-15

//********************  INIT ***********************

void configure_fifo() {
	FIFOPINPOLAR = 0xFF; SYNCDELAY(); // Enable on high level
	PINFLAGSAB = 0x00; SYNCDELAY(); // Don't care
	PINFLAGSCD = 0x00; SYNCDELAY(); // Don't care
	
	IFCONFIG = bmIFCLKSRC | bmASYNC; SYNCDELAY(); // External clock source, async
	
	EP2AUTOINLENH = 0x02; SYNCDELAY();// EZ-USB automatically commits data in 512-byte chunks
	EP2AUTOINLENL = 0x00; SYNCDELAY();
	
	EP2FIFOCFG = bmAUTOIN | bmWORDWIDE; SYNCDELAY();
}

void main_init() {
	REVCTL = 0x03;
	SYNCDELAY();
	SETIF48MHZ();
	SYNCDELAY(); 

	// Configure port
	PORTACFG = 0x00; SYNCDELAY(); 
	PORTCCFG = 0x00; SYNCDELAY(); 
	PORTECFG = 0x00; SYNCDELAY(); 

	// set IFCONFIG
	EP1INCFG &= ~bmVALID; SYNCDELAY(); 
	EP1OUTCFG &= ~bmVALID; SYNCDELAY(); 
	EP2CFG = (bmVALID | bmBULK | bmBUF4X /*| bmBUF1024*/ | bmDIR); SYNCDELAY();
	EP4CFG &= ~bmVALID; /* = (bmVALID | bmISO | bmBUF2X | bmDIR);*/ SYNCDELAY(); 
	EP6CFG = (bmVALID | bmBULK | bmBUF2X | bmDIR); SYNCDELAY(); 
	EP8CFG &= ~bmVALID;  SYNCDELAY();
	
	reset();
	
#ifndef SIMULATION
	configure_fifo();
#endif
}

#ifdef SIMULATION
WORD init = 0;

#define BUFF_SIZE (512)

void init_waveform() {
	short i;
	unsigned short *wdata = (unsigned short *)EP2FIFOBUF;
	for(i = 0; i < BUFF_SIZE/2; ++i) {
		if(i < BUFF_SIZE/4) {
			*wdata = 0x0000;
		} else {
			*wdata = 0xFFFF;
		}
	//	*wdata = 0x2000;
		++wdata;
	}
}

void send() {
	// Need to flusb the 4 buffers
	if(init < 4) {
		init_waveform();
		init++;
	}
	// ARM ep2 in
	EP2BCH = MSB(BUFF_SIZE); SYNCDELAY();
	EP2BCL = LSB(BUFF_SIZE); SYNCDELAY();
}
#endif

void timer0_callback() {
#ifdef SIMULATION
	if((EP2468STAT & bmEP2FULL) == 0) {
		send();
	}
#endif
}

void main_loop() {
}
