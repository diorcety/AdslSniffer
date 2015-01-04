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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "as_source_impl.h"
#include <iostream>

namespace gr {
namespace adslsniffer {

	as_source::sptr as_source::make() {
		return gnuradio::get_initial_sptr(new as_source_impl());
	}

	const uint16_t as_source_impl::sVendorId = 0x04b4;
	const uint16_t as_source_impl::sDeviceId = 0x1004;
	const uint8_t as_source_impl::sEndpoint = 0x82;

	/*
	 * The private constructor
	 */
	as_source_impl::as_source_impl() : gr::sync_block("as_source",
			gr::io_signature::make(0, 0, 0),
			gr::io_signature::make(1, 1, sizeof(float))),
			mBufferSize(2048) {
		std::cout << "as_source_impl" << std::endl;
		mContext.start<>();
		mRequest = std::make_shared<USBRequest>(8, mBufferSize);
		mDevice = mContext.openDeviceWithVidPid(sVendorId, sDeviceId);
		if(!mDevice) {
			std::ios::fmtflags f(std::cerr.flags());
			std::cerr << std::hex << std::setw(4);
			std::cerr << "Can't find device " << sVendorId << ":" << sDeviceId << std::endl;
			std::cerr.flags(f);
			return;
		}
		configure();
		printVersion();
	}

	/*
	 * Our virtual destructor.
	 */
	as_source_impl::~as_source_impl() {
		std::cout << "~as_source_impl" << std::endl;
		if(mDevice) {
			mDevice->releaseInterface(0);
			mDevice.reset();
		}
		mContext.stop();
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

		double as_source_impl::get_samp_rate(void) {
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

	void as_source_impl::set_samp_rate(double rate) {
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
		mStatsTimePoint = std::chrono::steady_clock::now();
		mStatsData = 0;
		USBBuffer buffer(16);
		buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x92, 0x0, 0x0, 0);
		mRequest->send(mDevice, sEndpoint, -1/*Infinite*/, std::bind(&as_source_impl::usbCallback, this, std::placeholders::_1, std::placeholders::_2));
		return true;
	}

	bool as_source_impl::stop(void) {
		check();
		std::cout << "AS Stop" << std::endl;
		USBBuffer buffer(16);
		buffer.controlTransfer(*mDevice, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, 0x93, 0x0, 0x0, 0);
		mRequest->stop();
		mRequest->wait<>();
		return true;
	}

	void as_source_impl::check(void) {
		if(!mDevice) {
			throw std::runtime_error("Device not connected");
		}
	}

	void as_source_impl::usbCallback(int status, const USBBuffer::Ptr &usbBuffer) {
		//std::cout << "Data " << status << std::endl;
		if(status == 0 && usbBuffer) {
			std::unique_lock<std::mutex> lck(mMutex);
			mBufferList.push_back(usbBuffer->steal());
			mCondition.notify_all();
		}
	}

	int as_source_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items) {
		static const float mid = (float)(0x200);
		static const float scale = (float)(0x200);
		float *out = (float *) output_items[0];
		int i = 0;

		while(i < noutput_items) {
			USBBuffer::Ptr buffer;
			{
				std::unique_lock<std::mutex> lck(mMutex);
				// Timeout ... in case
				while(mBufferList.empty()) mCondition.wait_for(lck, std::chrono::seconds(1));
				if(!mBufferList.empty()) {
					buffer = mBufferList.front();
				}
			}

			if(buffer) {
				if(buffer->getActualLength()%sizeof(unsigned short)) {
					std::cerr << "Mod(size, 2) != 0 !!!! Should not append" << std::endl;
				}
				int ninput_items = buffer->getBufferSize();
				int j = buffer->getOffset();
				unsigned short *in = (unsigned short *)(buffer->getBuffer() + j);

				while(i < noutput_items && j < ninput_items) {
					*out = (((float) (0x3FF & *in)) - mid)/scale;
					if(*out > 1.0f || *out < -1.0f) {
						std::cerr << "Value " << *out << " is out of range [-1.0, 1.0]" << std::endl;
					}
					++in;
					++out;
					++i;
					j+=sizeof(unsigned short);
				}

				buffer->setOffset(j);

				if(buffer->getOffset() == buffer->getBufferSize()) {
					std::unique_lock<std::mutex> lck(mMutex);
					mBufferList.pop_front();
				}
			}
		}

		{
			mStatsData += i;
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			std::chrono::duration<float> time_span = std::chrono::duration_cast<std::chrono::duration<float>>(now - mStatsTimePoint);
			if(time_span.count() >= 2.0f) {
				float rate = ((float) mStatsData * (2.0f / 1024.0f)) / time_span.count();
				std::cout << "Bitrate " << rate << " kbits/s" << std::endl;
				mStatsTimePoint = now;
				mStatsData = 0;
			}
		}

		return i;
	}

} /* namespace adslsniffer */
} /* namespace gr */

