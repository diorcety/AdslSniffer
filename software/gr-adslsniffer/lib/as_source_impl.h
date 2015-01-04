/*
AdslSniffer
Copyright (C) 2013	Yann Diorcet <diorcet.yann@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.
*/

#ifndef INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H
#define INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H

#include <adslsniffer/as_source.h>
#include <USB.h>
#include <mutex>
#include <condition_variable>
#include <list>
#include <chrono>

namespace gr {
namespace adslsniffer {

	class as_source_impl : public as_source
	{
	private:
		static const uint16_t sVendorId;
		static const uint16_t sDeviceId;
		static const uint8_t sEndpoint;

		const size_t mBufferSize;
		USBContext mContext;
		USBDevice::Ptr mDevice;
		USBRequest::Ptr mRequest;

		std::mutex mMutex;
		std::condition_variable mCondition;

		std::list<USBBuffer::Ptr> mBufferList;

		std::chrono::steady_clock::time_point mStatsTimePoint;
		size_t mStatsData;

		void check(void);
		void configure(void);
		void printVersion(void);

		void usbCallback(int status, const USBBuffer::Ptr &usbBuffer);

	public:
		as_source_impl();
		~as_source_impl();

		void set_samp_rate(double rate);
		double get_samp_rate(void);

		void flush(void);
		bool start(void);
		bool stop(void);

		int work(int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items);
	};

} // namespace adslsniffer
} // namespace gr

#endif /* INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H */

