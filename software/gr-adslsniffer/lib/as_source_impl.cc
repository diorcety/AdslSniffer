/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "as_source_impl.h"
#include <iostream>

namespace gr {
  namespace adslsniffer {

    as_source::sptr
    as_source::make()
    {
      return gnuradio::get_initial_sptr
        (new as_source_impl());
    }
    
    const uint16_t as_source_impl::sVendorId = 0x04b4;
    const uint16_t as_source_impl::sDeviceId = 0x1004;
    const uint8_t as_source_impl::sEndpoint = 0x82;

    /*
     * The private constructor
     */
    as_source_impl::as_source_impl()
      : gr::sync_block("as_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float))),
	mBufferSize(2048)
    {
	std::cout << "as_source_impl" << std::endl;
	mContext.start<>();
	mRequest = std::make_shared<USBRequest>(8, mBufferSize);
	mDevice = mContext.openDeviceWithVidPid(sVendorId, sDeviceId);
	if(!mDevice) {
		std::ios::fmtflags f(std::cout.flags());
		std::cerr << std::hex << std::setw(4);
		std::cerr << "Can't find device " << sVendorId << ":" << sDeviceId << std::endl; 
		std::cout.flags(f);
		return;
	}	
	configure();
	printVersion();
    }

    /*
     * Our virtual destructor.
     */
    as_source_impl::~as_source_impl()
    {
	std::cout << "~as_source_impl" << std::endl;
	stop();
	if(mDevice) {
		mDevice->releaseInterface(0);
		mDevice.reset();
	}
	mContext.wait<>();
    }
    
    void as_source_impl::configure(void) {
	check();
#ifndef WIN32
	if(mDevice->isKernelDriverActive(0)) mDevice->detachKernelDriver(0);
#endif
	mDevice->setConfiguration(1);
	mDevice->claimInterface(0);
	mDevice->setInterfaceAltSetting(0, 0);
    }

    double as_source_impl::get_sample_rate(void) {
	check();
	USBBuffer buffer(4);
	unsigned int ret = buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x95, 0x0, 0x0, buffer.getBufferSize());
	if(ret == 4) {
		unsigned int *rate = (unsigned int *)buffer.getBuffer();
		std::cout << "Sample rate: " << *rate << std::endl;
		return (double) *rate;
	} else {
		std::cerr << "Error during sample rate grabbing" << std::endl;
		return 0.0;
	}
    }

    void as_source_impl::set_sample_rate(double rate) {
	std::cerr << "Ignore set_sample_rate(" << rate << ")" << std::endl;
    }

    void as_source_impl::printVersion(void) {
	check();
	USBBuffer buffer(65);
	unsigned int ret = buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x94, 0x0, 0x0, buffer.getBufferSize()-1);
	if(ret < buffer.getBufferSize()) {
		unsigned char *cBuffer = buffer.getBuffer();
		cBuffer[ret] = '\0';
		std::cout << cBuffer << std::endl;
	} else {
		std::cerr << "Error during version grabbing" << std::endl;
	}
    } 

    void as_source_impl::flush(void) {
    }

    bool as_source_impl::start(void) {
	check();
	std::cout << "AS Start" << std::endl;
	USBBuffer buffer(16);
	buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x92, 0x0, 0x0, 0);
	mRequest->send(mDevice, sEndpoint, -1/*Infinite*/, std::bind(&as_source_impl::usbCallback, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool as_source_impl::stop(void) {
	check();
	std::cout << "AS Stop" << std::endl;
	USBBuffer buffer(16);
	buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x93, 0x0, 0x0, 0);
	mRequest->stop();
	mRequest->wait<>();
    }

    void as_source_impl::check(void) {
	if(!mDevice) {
		throw std::runtime_error("Device not connected");
	}
    }

    void as_source_impl::usbCallback(int status, const std::shared_ptr<const USBBuffer> &usbBuffer) {
	//std::cout << "Data " << status << std::endl;
    }

    int
    as_source_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        float *out = (float *) output_items[0];

        // Do <+signal processing+>

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace adslsniffer */
} /* namespace gr */

