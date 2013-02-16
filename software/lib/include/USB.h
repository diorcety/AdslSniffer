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

#pragma once
#ifndef __USB__
#define __USB__
#include <mutex>
#include <memory>
#include <condition_variable>
#include <list>
#include <chrono>
#include <thread>
#include <functional>
#ifdef _WIN32
	#include <libusbx-1.0/libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif


class USBAsync: protected std::recursive_mutex {
private:
    std::condition_variable mCond;
    std::mutex mMutex;
	int mCompleted;
	bool mStarted;
	
protected:
	void terminate();
	bool isCompleted();
	
public:
	bool isRunning();
	virtual bool start();
	virtual bool stop();
	
	USBAsync();
	~USBAsync();
	
	template<typename Rep = std::chrono::hours::rep, typename Period = std::chrono::hours::period>
	bool wait(std::chrono::duration<Rep, Period> duration = std::chrono::hours::max()) {
		std::unique_lock<std::mutex> lock(mMutex);
		if(mStarted) {
			mCond.wait_for(lock, duration);
			return true;
		}
		return false;
	}
};


class USBDevice;

class USBBuffer: public std::enable_shared_from_this<USBBuffer> {
private:
	struct libusb_transfer* mTransfer;
	unsigned char* mBuffer;
	size_t	mBufferSize;
	std::function<void(const std::shared_ptr<USBBuffer>&)> mCallBack;
	
	void transfer_callback(struct libusb_transfer *transfer);
	static void __transfer_callback(struct libusb_transfer *transfer);
	
public:
	USBBuffer(size_t size);
	
	int send(USBDevice &device, std::function<void(const std::shared_ptr<USBBuffer>&)> cb, unsigned char endpoint, size_t len, unsigned int timeout);
	
	unsigned char* getBuffer();
	size_t getBufferSize();
	
	int getStatus();
	size_t getLength();
	size_t getActualLength();
	
	~USBBuffer();
};

class USBRequest: public USBAsync {
private:
	std::list<std::shared_ptr<USBBuffer>> idle_buffers;
	std::list<std::shared_ptr<USBBuffer>> processing_buffers;
	
	USBDevice* mDevice;
	
	std::function<void (int)> mCallback;
	int mError;
	int mEndpoint;
	size_t mBytes;
		
	void handleBuffers();
	void receive(const std::shared_ptr<USBBuffer> &buffer);
	
private:
	virtual bool start();
	
public:
	USBRequest(size_t packet_count, size_t buffer_size);
	int send(USBDevice &device, unsigned char endpoint, size_t bytes, std::function<void (int)> callback=std::function<void (int)>());
	
	~USBRequest();
};

class USBDevice {
private:
	libusb_device_handle *mDeviceHandle;

public:
	USBDevice(libusb_device_handle *hnd);

	libusb_device_handle* getDeviceHandle() const;
};

class USBContext: public USBAsync {
private:
	libusb_context* mContext;
	struct timeval mTv;
	std::thread mThread;
	
	void run();
	
private:
	virtual bool start();
	
public:
	USBContext(libusb_context *context);
	~USBContext();
	libusb_context* getContext() const;
	
	template<typename Rep = std::chrono::seconds::rep, typename Period = std::chrono::seconds::period>
	bool start(std::chrono::duration<Rep, Period> pool = std::chrono::seconds(1)) {
		std::unique_lock<std::recursive_mutex> lock(*this);
		if(USBAsync::start()) {
			std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(pool);
			mTv.tv_sec = seconds.count();
			mTv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(pool - seconds).count();
			mThread = std::thread(&USBContext::run, this);
			return true;
		}
		return false;
	}
	
	template<typename Rep = std::chrono::hours::rep, typename Period = std::chrono::hours::period>
	bool wait(std::chrono::duration<Rep, Period> duration = std::chrono::hours::max()) {
		bool ret = USBAsync::wait<Rep, Period>(duration);
		try {
			mThread.join();
		} catch (std::system_error&) {
		}
		return ret;
	}
};

#endif
