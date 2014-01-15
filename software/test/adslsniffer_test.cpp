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

#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <cassert>
#ifdef WIN32
#include <windows.h>
void usleep(int s) {
  Sleep(s/1000);
}
#endif
#include "USB.h"


int read_debug(libusb_device_handle *hndl, int wait = 1) {
	//return LIBUSB_ERROR_TIMEOUT;
	int rv;
	int transferred;
	unsigned char buf[64];
	rv = libusb_bulk_transfer(hndl, 0x86, (unsigned char*)buf, sizeof(buf), &transferred, wait); 
	if (rv == 0 && transferred > 0) {
		std::cout << ">>>>> " << buf << " <<<<<" << std::endl;
	} else if(rv != LIBUSB_ERROR_TIMEOUT) {
		printf ( "IN Transfer(Debug) failed: %s(%d)\n", libusb_error_name(rv), rv);
	}
	return rv;
}

#define BENCH_DATA_SIZE (2*8832*1000) // 8832*1000 capture of 16 bits
int nb_transfer = 0;
#ifdef WIN32
LARGE_INTEGER bench_base_time;
LARGE_INTEGER bench_start_time;
LARGE_INTEGER bench_stop_time;
#else
struct timespec bench_base_time;
struct timespec bench_start_time;
struct timespec bench_stop_time;
#endif

void bench_start() {
	nb_transfer = 0;
#ifdef WIN32
	QueryPerformanceFrequency(&bench_base_time);
	QueryPerformanceCounter(&bench_start_time);
#else
	clock_getres(CLOCK_MONOTONIC, &bench_base_time);
	clock_gettime(CLOCK_MONOTONIC, &bench_start_time);
#endif
}

void bench_inc() {
	++nb_transfer;
}

void bench_stop() {
#ifdef WIN32
	QueryPerformanceCounter(&bench_stop_time);
#else
	clock_gettime(CLOCK_MONOTONIC, &bench_stop_time);
#endif
}

void bench_stats() {
	float time;
	float rate;
#ifdef WIN32
	time = bench_stop_time.QuadPart - bench_start_time.QuadPart;
	time /= bench_base_time.QuadPart;
	time *= 1000;
#else
	time =  bench_stop_time.tv_nsec - bench_start_time.tv_nsec;
	time /= 1000000000;
	time += (bench_stop_time.tv_sec - bench_start_time.tv_sec);
#endif
	rate = (float) BENCH_DATA_SIZE / 1024 / time;
	printf("============================================\n");
	printf("Stats                                       \n");
	printf("--------------------------------------------\n");
	printf("Transfer: %d kbytes                         \n", BENCH_DATA_SIZE/1024);
	printf("Nb transfer: %d                             \n", nb_transfer);
	printf("Time: %f s                                  \n", time);
	printf("Rate: %f kbytes/s                           \n", rate);
	printf("============================================\n");
}

void usb_cb(int status, const std::shared_ptr<const USBBuffer> &usbBuffer) {
	return;
}

int main(int argc, char* argv[]) {
	int rv;
	unsigned int ret;
	USBContext context;
	USBBuffer buffer(65);
	//context.setDebug(4);
	USBDevice::Ptr device = context.openDeviceWithVidPid(0x04b4, 0x1004);

#ifndef WIN32
	if(device->isKernelDriverActive(0)) device->detachKernelDriver(0);
#endif
	
	device->setConfiguration(1);
	device->claimInterface(0);
	device->setInterfaceAltSetting(0, 0);

	libusb_device_handle *hndl = device->getDeviceHandle();

	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
 
 	printf("AdslSniffer Test\n"); 
	printf("RESET\n"); 
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x90,
		0x11,
		0x22,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}

	ret = buffer.controlTransfer(*device, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x94, 0x11, 0x22, buffer.getBufferSize()-1);
	if(ret < buffer.getBufferSize()) {
		unsigned char *cBuffer = buffer.getBuffer();
		cBuffer[ret] = '\0';
		std::cout << cBuffer << std::endl;
	} else {
		std::cerr << "Error during version grabbing" << std::endl;
	}
	
	ret = buffer.controlTransfer(*device, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x95, 0x11, 0x22, buffer.getBufferSize()-1);
	if(ret == 4) {
		unsigned int *rate = (unsigned int *)buffer.getBuffer();
		std::cout << "Sample rate: " << *rate << std::endl;
	} else {
		std::cerr << "Error during sample rate grabbing" << std::endl;
	}
 
	printf("Enable debug\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x91,
		0x01,
		0x22,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL(Debug) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}

	read_debug(hndl, 1000);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
	
	printf("Print test\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x99,
		0x00,
		0x22,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL(Debug) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}

	read_debug(hndl, 1000);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
	
	
	printf("Start Test\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x92,
		0xffff,
		0xffff,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL(Start) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}
	read_debug(hndl, 0);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
	
	USBRequest request(8, 2048);
	
	context.start<>();
	
	// Send request
	bench_start();
	request.send(device, 0x82, BENCH_DATA_SIZE, usb_cb);
	bench_inc();
	request.wait<>();
	bench_stop();

	context.stop();
	context.wait<>();
	
	printf("Stop Test\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x93,
		0xffff,
		0xffff,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL(Stop) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}
	read_debug(hndl, 0);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
	
	printf("Print test\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x99,
		0x00,
		0x22,
		0,
		0,
		0);
	if(rv != 0) {
		printf ( "CONTROL(Debug) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
		return rv;
	}
	read_debug(hndl, 0);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);

	// Empty debug
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);

	// Close device
	device->releaseInterface(0);
	device.reset();
	
	printf("Test end\n");
	bench_stats();

	return 0;
}
