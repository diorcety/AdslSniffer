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

#include "USB.h"
#include <iostream>
class USBDevice;

void USBBuffer::transfer_callback(struct libusb_transfer *transfer) {
	mCallBack(this);
}

void USBBuffer::__transfer_callback(struct libusb_transfer *transfer) {
	USBBuffer *buffer = (USBBuffer *) transfer->user_data;
	buffer->transfer_callback(transfer);
}

USBBuffer::USBBuffer(size_t size): mBufferSize(size) {
	mTransfer = libusb_alloc_transfer(0);
	mBuffer = new unsigned char[mBufferSize];
}

int USBBuffer::send(USBDevice *device, std::function<void(USBBuffer*)> cb, unsigned char endpoint, size_t len, unsigned int timeout) {
	mCallBack = cb;
	libusb_fill_bulk_transfer(mTransfer, device->getDeviceHandle(),
		endpoint, mBuffer, len, (libusb_transfer_cb_fn)__transfer_callback, this, timeout);
	return libusb_submit_transfer(mTransfer);
}

unsigned char* USBBuffer::getBuffer() {
	return mBuffer;
}

size_t USBBuffer::getBufferSize() {
	return mBufferSize;
}

int USBBuffer::getStatus() {
	return mTransfer->status;
}

size_t USBBuffer::getLength() {
	return mTransfer->length;
}

size_t USBBuffer::getActualLength() {
	return mTransfer->actual_length;
}

USBBuffer::~USBBuffer() {
	libusb_free_transfer(mTransfer);
	delete[] mBuffer;
}

USBRequest::USBRequest(size_t packet_count, size_t buffer_size): mDone(true) {
	for(size_t i = 0; i < packet_count; ++i) {
		idle_buffers.push_back(new USBBuffer(buffer_size));
	}
}

void USBRequest::receive(USBBuffer *buffer) {
	std::unique_lock<std::mutex> lock(mMutex);
	mBytes += buffer->getLength();
	if(buffer->getStatus() == LIBUSB_TRANSFER_COMPLETED) {
		mBytes -= buffer->getActualLength();
	} else {
		std::cout << "Error " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
	}
	processing_buffers.remove(buffer);
	idle_buffers.push_back(buffer);
	addBuffers();
}

void USBRequest::addBuffers() {
	while(!idle_buffers.empty() && mErr == 0 && mBytes > 0) {
		USBBuffer *buffer = idle_buffers.front();
		idle_buffers.pop_front();

		size_t size = mBytes;
		if(size > buffer->getBufferSize())
			size = buffer->getBufferSize();
		mBytes -= size;
		int ret = buffer->send(mDevice, std::bind(&USBRequest::receive, this, std::placeholders::_1), mEp, size, 3000);
		if(ret == 0) {
			processing_buffers.push_back(buffer);
		} else {
			std::cout << "Can't send: " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
			mErr = ret;
		}
	}
	if(mBytes == 0 || mErr == 0) {
		if(processing_buffers.empty()) {
			mDone = true;
			request_end();
		}
	}
}

void USBRequest::request_end() {
	std::cout << "Request end: (remaining " << mBytes << " bytes)"<< std::endl;
	if(mErr != 0) {
		std::cout << "Request error: " << mErr  << std::endl;
	}
	if(mCallback) {
		mCallback(mErr);
	}
	mCond.notify_all();
}

int USBRequest::request(USBDevice &device, unsigned char endpoint, size_t request_bytes, std::function<void (int)> callback) {
	std::cout << "Request start: (" << request_bytes << " bytes)" <<std::endl;
	mBytes = request_bytes;
	mDevice = &device;
	mErr = 0;
	mDone = false;
	mEp = endpoint;
	mCallback = callback;
	addBuffers();
}

USBRequest::~USBRequest() {
	wait<>();
	while(!idle_buffers.empty()) {
		delete idle_buffers.back();
		idle_buffers.pop_back();
	}
}

USBDevice::USBDevice(libusb_device_handle *hnd): mDeviceHandle(hnd) {
}

libusb_device_handle* USBDevice::getDeviceHandle() const {
	return mDeviceHandle;
}

USBContext::USBContext(libusb_context *context): mContext(context), mStarted(false) {
}

USBContext::~USBContext() {
	stop();
	wait();
}

libusb_context* USBContext::getContext() const {
	return mContext;
}

void USBContext::start() {
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mCompleted = 0;
		mStarted = true;
	}
	while(!mCompleted) {
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		libusb_handle_events_timeout_completed(mContext, &tv, &mCompleted);
	}
	
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mStarted = false;
	}
	mCond.notify_all();
}

void USBContext::stop() {
	std::lock_guard<std::mutex> lock(mMutex);
	mCompleted = 1;
}
