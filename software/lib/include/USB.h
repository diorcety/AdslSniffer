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
#include <exception>
#include <libusb.h>

class USBException: public std::exception {
private:
	int mCode;
	std::string mWhat;	
	std::string mWhere;

public:
	USBException(int code, const std::string &&where = std::string());
	int code() const;

	virtual const char* where() const throw();
	virtual const char* what() const throw();
};

class USBAsync: protected std::recursive_mutex {
private:
    std::condition_variable mCond;
    std::mutex mMutex;
	int mCompleted;
	bool mStarted;
	
protected:
	virtual void terminate();
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
public:
	typedef std::shared_ptr<USBBuffer> Ptr;
	typedef std::shared_ptr<const USBBuffer> ConstPtr;

private:
	struct libusb_transfer* mTransfer;
	unsigned char* mBuffer;
	size_t	mBufferSize;
	std::function<void(const USBBuffer::Ptr &&)> mCallBack;
	
	void transfer_callback(struct libusb_transfer *transfer);
	static void __transfer_callback(struct libusb_transfer *transfer);
	
public:
	USBBuffer(size_t size);
	
	void fillBulkTransfer(USBDevice &device, std::function<void(const USBBuffer::Ptr &&)> cb, unsigned char endpoint, size_t len, unsigned int timeout = 3000);
	unsigned int controlTransfer(USBDevice &device, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t len, unsigned int timeout = 3000);
	unsigned int bulkTransfer(USBDevice &device, unsigned char endpoint, int len, unsigned int timeout = 3000);
	unsigned int interruptTransfer(USBDevice &device, unsigned char endpoint, int len, unsigned int timeout = 3000);
	
	unsigned char* getBuffer() const;
	size_t getBufferSize() const;
	
	int getStatus() const;
	size_t getLength() const;
	size_t getActualLength() const;
	
	~USBBuffer();
};

class USBRequest: public USBAsync {
private:
	std::list<std::shared_ptr<USBBuffer>> idle_buffers;
	std::list<std::shared_ptr<USBBuffer>> processing_buffers;
	
	std::shared_ptr<USBDevice> mDevice;
	
	std::function<void (int, const USBBuffer::ConstPtr &&)> mCallback;
	int mTimeout;
	int mError;
	int mEndpoint;
	size_t mBytes;
		
	void handleBuffers();
	void receive(const USBBuffer::Ptr &&buffer);
	
private:
	virtual bool start();

protected:
	virtual void terminate();
	
public:
	USBRequest(size_t packet_count, size_t buffer_size, int timeout = 3000);
	bool send(const std::shared_ptr<USBDevice> &device, unsigned char endpoint, size_t bytes, std::function<void (int, const USBBuffer::ConstPtr &&buffer)> callback=std::function<void (int, const USBBuffer::ConstPtr &&)>());
	
	~USBRequest();
};

class USBDevice {
public:
	typedef std::shared_ptr<USBDevice> Ptr;

private:
	mutable libusb_device_handle *mDeviceHandle;
	bool mOwn;

public:
	USBDevice(libusb_device_handle *hnd, bool own);
	~USBDevice();

	libusb_device_handle* getDeviceHandle() const;

	int getConfiguration() const;
        void setConfiguration(int configuration);

	void setInterfaceAltSetting(int interface, int setting);
        void claimInterface(int interface);
	void releaseInterface(int interface);

	bool isKernelDriverActive(int interface) const;
	void detachKernelDriver(int interface);
	void attachKernelDriver(int interface);

	void resetDevice();
};

class USBContext: public USBAsync {
private:
	mutable libusb_context* mContext;
	bool mOwn;
	struct timeval mTv;
	std::thread mThread;
	
	void run();
	
private:
	virtual bool start();
	
public:
	USBContext();
	USBContext(libusb_context *ctx);
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
        
	void setDebug(int level);
	USBDevice::Ptr openDeviceWithVidPid(uint16_t vendor_id, uint16_t product_id) const;
};

#endif
