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

#ifndef __USB__
#define __USB__
#include <mutex>
#include <condition_variable>
#include <list>
#include <chrono>
#include <functional>
#ifdef _WIN32
	#include <libusbx-1.0/libusb.h>
#else
	#include <libusb-1.0/libusb.h>
#endif

class USBDevice;

class USBBuffer {
private:
	struct libusb_transfer* mTransfer;
	unsigned char* mBuffer;
	size_t	mBufferSize;
	std::function<void(USBBuffer*)> mCallBack;
	
	void transfer_callback(struct libusb_transfer *transfer);
	
	static void __transfer_callback(struct libusb_transfer *transfer);
	
public:
	USBBuffer(size_t size);
	
	int send(USBDevice *device, std::function<void(USBBuffer*)> cb, unsigned char endpoint, size_t len, unsigned int timeout);
	
	unsigned char* getBuffer();
	size_t getBufferSize();
	
	int getStatus();
	size_t getLength();
	size_t getActualLength();
	
	~USBBuffer();
};

class USBRequest {
private:
	std::list<USBBuffer *> idle_buffers;
	std::list<USBBuffer *> processing_buffers;
	std::mutex mMutex;
    std::condition_variable mCond;
	
	USBDevice *mDevice;
	
	std::function<void (int)> mCallback;
	int mErr;
	int mEp;
	bool mDone;
	size_t mBytes;
		
	void addBuffers();
	void receive(USBBuffer *buffer);
	void request_end();
public:
	USBRequest(size_t packet_count, size_t buffer_size);
	int request(USBDevice &device, unsigned char endpoint, size_t bytes, std::function<void (int)> callback=std::function<void (int)>());
	
	template< typename Rep = std::chrono::hours::rep, typename Period = std::chrono::hours::period >
	bool wait(std::chrono::duration<Rep, Period> duration = std::chrono::hours::max()) {
		std::unique_lock<std::mutex> lock(mMutex);
		if(!mDone) {
			mCond.wait_for(lock, std::chrono::seconds(120));
		}
		return mDone;
	}
	
	~USBRequest();
};

class USBDevice {
private:
	libusb_device_handle * mDeviceHandle;

public:
	USBDevice(libusb_device_handle *hnd);

	libusb_device_handle* getDeviceHandle() const;
};

class USBContext {
private:
	libusb_context * mContext;
	int mCompleted;
	bool mStarted;
	std::mutex mMutex;
    std::condition_variable mCond;
    
public:
	USBContext(libusb_context *context);
	~USBContext();
	libusb_context* getContext() const;
	
	void start();
	void stop();
	template< typename Rep = std::chrono::hours::rep, typename Period = std::chrono::hours::period >
	bool wait(std::chrono::duration<Rep, Period> duration = std::chrono::hours::max()) {
		std::unique_lock<std::mutex> lock(mMutex);
		if(mStarted) {
			mCond.wait_for(lock, duration);
		}
		return !mStarted;
	}
};

#endif
