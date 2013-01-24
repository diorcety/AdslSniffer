/**
 * Copyright (C) 2009 Ubixum, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

#include <stdlib.h>
#include <cstdio>
#include <cassert>
#include <thread>
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
		printf("%s", buf); 
		fflush(stdout);
	} else if(rv != LIBUSB_ERROR_TIMEOUT) {
		printf ( "IN Transfer(Debug) failed: %s(%d)\n", libusb_error_name(rv), rv);
	}
	return rv;
}

#define BENCH_DATA_SIZE (1024*1024*100)
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
	time /= 1000000;
	time += (bench_stop_time.tv_sec - bench_start_time.tv_sec) * 1000;
#endif
	rate = (float) BENCH_DATA_SIZE / time;
	printf("Stats %d bytes in %f ms : %f kbytes/s\n", BENCH_DATA_SIZE, time, rate);
	printf("Nb transfer %d\n", nb_transfer);
}

//#define TEST1

#ifndef TEST1
void thread_fct(libusb_context *ctx, bool &ok) {
	struct timeval tv;

	while(ok) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		libusb_handle_events_timeout (ctx, &tv);
	}
}
#endif

int main(int argc, char* argv[]) {
	int rv;
	libusb_context* ctx;
	libusb_init(&ctx);
	//libusb_set_debug(ctx, 4);
	libusb_device_handle* hndl = libusb_open_device_with_vid_pid(ctx,0x04b4,0x1004);
	if(hndl == NULL) {
		printf("Can't open device\n");
		return -100;
	}
#ifndef TEST1
	bool continue_thread = true;
	//std::thread usb_thread(thread_fct, ctx, std::ref(continue_thread)); // pass by reference
	USBDevice device(hndl);
#endif
 
#ifndef WIN32
	rv = libusb_kernel_driver_active(hndl, 0); 
	if (rv == 1) { 
		rv = libusb_detach_kernel_driver(hndl, 0); 
	} 
#endif
	libusb_set_configuration(hndl, 1);
	libusb_claim_interface(hndl, 0);
	libusb_set_interface_alt_setting(hndl, 0, 0);

	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
 
 	printf("TOTO\n"); 
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x94,
		0x11,
		0x22,
		0,
		0,
		0);
	if(rv != 0) {
        printf ( "CONTROL Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
        return rv;
	}
	read_debug(hndl, 0);
 
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
	read_debug(hndl, 0);
	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);
	
	
	int bench_size = BENCH_DATA_SIZE;
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

#ifndef TEST1
	USBRequest request(8, 2048);
#endif
	bench_start();
#ifndef TEST1
	request.request(ctx, &device, 0x82, BENCH_DATA_SIZE);
#else
	int timeout_count = 0;
	while(bench_size > 0) {
		int len = bench_size;
		int transferred;
		#define BUF_SIZE (512*3)
		unsigned char buf[BUF_SIZE];
		if(len > BUF_SIZE) {
			len = BUF_SIZE;
		}
		rv = libusb_bulk_transfer(hndl, 0x82, (unsigned char*)buf, len, &transferred, 1); 
		bench_size -= transferred;
		if(rv != LIBUSB_ERROR_TIMEOUT) {
			if (rv != 0) {
				printf ( "IN Transfer(Bench) failed: %s(%d)\n", libusb_error_name(rv), rv);
				read_debug(hndl, 500);
				return rv;
			} else {
				timeout_count = 0;
				bench_inc();
			}
		} else {
			if(++timeout_count > 300) {
				printf ( "Timeout ...\n");
				return -1;
			}
		}
	}
#endif
	
	bench_stop();

    printf("Start Test\n");
	rv = libusb_control_transfer(hndl, 
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		0x93,
		0xffff,
		0xffff,
		0,
		0,
		0);
	if(rv != 0) {
        printf ( "CONTROL(Start) Transfer failed: %s(%d)\n", libusb_error_name(rv), rv);
        return rv;
	}

	printf("Test end\n");
	bench_stats();



	while(read_debug(hndl) != LIBUSB_ERROR_TIMEOUT);

#ifndef TEST1
	continue_thread = false;
	//usb_thread.join();
#endif

	libusb_release_interface(hndl, 0);
	libusb_close(hndl);

	return 0;
}
