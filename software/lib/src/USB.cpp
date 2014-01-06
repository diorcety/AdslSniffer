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

USBAsync::USBAsync() : mStarted(false) {
}

USBAsync::~USBAsync() {
	wait<>();
}

void USBAsync::terminate() {
	std::unique_lock<std::recursive_mutex> lock(*this);
	mStarted = false;
	mCond.notify_all();
}

bool USBAsync::isCompleted() {
	std::unique_lock<std::recursive_mutex> lock(*this);
	return mCompleted;
}

bool USBAsync::isRunning() {
	std::unique_lock<std::recursive_mutex> lock(*this);
	return mStarted;
}

bool USBAsync::start() {
	std::unique_lock<std::recursive_mutex> lock(*this);
	mCompleted = false;
	mStarted = true;
	return true;
}

bool USBAsync::stop() {
	std::unique_lock<std::recursive_mutex> lock(*this);
	mCompleted = true;
	return true;
}

void USBBuffer::transfer_callback(struct libusb_transfer *transfer) {
	if(mCallBack) {
		mCallBack(this->shared_from_this());
	}
}

void USBBuffer::__transfer_callback(struct libusb_transfer *transfer) {
	std::shared_ptr<USBBuffer> buffer = reinterpret_cast<USBBuffer *>(transfer->user_data)->shared_from_this();
	buffer->transfer_callback(transfer);
}

USBBuffer::USBBuffer(size_t size): mBufferSize(size) {
	mTransfer = libusb_alloc_transfer(0);
	mBuffer = new unsigned char[mBufferSize];
}

int USBBuffer::send(USBDevice &device, std::function<void(const std::shared_ptr<USBBuffer>&&)> cb, unsigned char endpoint, size_t len, unsigned int timeout) {
	mCallBack = cb;
	libusb_fill_bulk_transfer(mTransfer, device.getDeviceHandle(),
		endpoint, mBuffer, len, (libusb_transfer_cb_fn)__transfer_callback, this, timeout);
	return libusb_submit_transfer(mTransfer);
}

unsigned char* USBBuffer::getBuffer() const {
	return mBuffer;
}

size_t USBBuffer::getBufferSize() const {
	return mBufferSize;
}

int USBBuffer::getStatus() const {
	return mTransfer->status;
}

size_t USBBuffer::getLength() const {
	return mTransfer->length;
}

size_t USBBuffer::getActualLength() const {
	return mTransfer->actual_length;
}

USBBuffer::~USBBuffer() {
	libusb_free_transfer(mTransfer);
	delete[] mBuffer;
}

bool USBRequest::start() {
	return true;
}

USBRequest::USBRequest(size_t packet_count, size_t buffer_size, int timeout): mTimeout(timeout) {
	for(size_t i = 0; i < packet_count; ++i) {
		idle_buffers.push_back(std::make_shared<USBBuffer>(buffer_size));
	}
}

void USBRequest::receive(const std::shared_ptr<USBBuffer> &&buffer) {
	std::unique_lock<std::recursive_mutex> lock(*this);
	int status = buffer->getStatus();
	
	// Reset buffer
	processing_buffers.remove(buffer);
	if(mCallback) {
		mCallback(status, buffer);
	}
	idle_buffers.push_back(buffer);
	
	mBytes += buffer->getLength();
	if(status == LIBUSB_TRANSFER_COMPLETED) {
		mBytes -= buffer->getActualLength();
	} else {
		std::cerr << "Error " << status << " with buffer 0x" << buffer << std::endl;
		mError = status;
	}

	handleBuffers();
}

void USBRequest::handleBuffers() {
	while(!idle_buffers.empty() && mError == 0 && mBytes > 0) {
		std::shared_ptr<USBBuffer> buffer = idle_buffers.front();
		idle_buffers.pop_front();

		size_t size = mBytes;
		if(size > buffer->getBufferSize())
			size = buffer->getBufferSize();
		mBytes -= size;
		int ret = buffer->send(*mDevice, std::bind(&USBRequest::receive, this, std::placeholders::_1), mEndpoint, size, mTimeout);
		if(ret == 0) {
			processing_buffers.push_back(buffer);
		} else {
			std::cerr << "Can't send: " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
			mError = ret;
		}
	}
	
	if(mBytes == 0 || mError != 0) {
		if(processing_buffers.empty()) {
			std::cerr << "Request end: (remaining " << mBytes << " bytes)" << std::endl;
			if(mError != 0) {
				std::cerr << "Request error: " << mError << std::endl;
			}
			if(mCallback) {
				mCallback(mError, std::shared_ptr<USBBuffer>());
			}
			terminate();
		}
	}
}

bool USBRequest::send(USBDevice &device, unsigned char endpoint, size_t request_bytes, std::function<void (int, const std::shared_ptr<const USBBuffer> &&)> callback) {
	std::unique_lock<std::recursive_mutex> lock(*this);
	if(USBAsync::start()) {
		std::cerr << "Request start: (" << request_bytes << " bytes)" <<std::endl;
		mBytes = request_bytes;
		mDevice = &device;
		mError = 0;
		mEndpoint = endpoint;
		mCallback = callback;
		handleBuffers();
		return true;
	} else {
		std::cerr << "Request failed!" <<std::endl;
		return false;
	}
}

USBRequest::~USBRequest() {
	wait<>();
	while(!idle_buffers.empty()) {
		idle_buffers.pop_back();
	}
}

USBDevice::USBDevice(libusb_device_handle *hnd): mDeviceHandle(hnd) {
}

libusb_device_handle* USBDevice::getDeviceHandle() const {
	return mDeviceHandle;
}

USBContext::USBContext(libusb_context *context): mContext(context) {
}

USBContext::~USBContext() {
	wait<>();
}

libusb_context* USBContext::getContext() const {
	return mContext;
}

bool USBContext::start() {
	return true;
};

void USBContext::run() {
	while(!isCompleted()) {
		libusb_handle_events_timeout (mContext, &mTv);
	}
	terminate();
}
