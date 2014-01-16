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

USBException::USBException(int code, std::string &&where): mCode(code), mWhere(where) {
	if(!mWhere.empty()) {
		mWhat += mWhere + ":";
	}
	mWhat += "(";
	mWhat += libusb_error_name(code);
	mWhat += ") ";
	mWhat += libusb_strerror((enum libusb_error)code);
}

int USBException::code() const {
	return mCode;
}

const char* USBException::where() const throw() {
	return mWhere.c_str();
}

const char* USBException::what() const throw() {
	return mWhat.c_str();
}

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
	USBBuffer::Ptr buffer = reinterpret_cast<USBBuffer *>(transfer->user_data)->shared_from_this();
	buffer->transfer_callback(transfer);
}

USBBuffer::USBBuffer(size_t size, unsigned char *buffer): mBufferSize(size), mOffset(0), mBuffer(buffer) {
	mTransfer = libusb_alloc_transfer(0);
	if(size > 0) {
		if(mBuffer == NULL) {
			mBuffer = new unsigned char[mBufferSize];
		}
	} else {
		mBuffer = NULL;
	}
} 

USBBuffer::Ptr USBBuffer::steal() {
	unsigned char *ptr = mBuffer;
	mBuffer = new unsigned char[mBufferSize];
	USBBuffer::Ptr newBuffer = std::make_shared<USBBuffer>(getActualLength(), ptr);
	return newBuffer;
}

size_t USBBuffer::getOffset() const {
	return mOffset;
}

void USBBuffer::setOffset(size_t offset) {
	mOffset = offset;
}

void USBBuffer::fillBulkTransfer(USBDevice &device, std::function<void(const USBBuffer::Ptr &)> cb, unsigned char endpoint, size_t len, unsigned int timeout) {
	mCallBack = cb;
	libusb_fill_bulk_transfer(mTransfer, device.getDeviceHandle(),
		endpoint, mBuffer, len, (libusb_transfer_cb_fn)__transfer_callback, this, timeout);
	int ret = libusb_submit_transfer(mTransfer);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

unsigned int USBBuffer::controlTransfer(USBDevice &device, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t len, unsigned int timeout) {
	int ret = libusb_control_transfer(device.getDeviceHandle(), bmRequestType, bRequest, wValue, wIndex, mBuffer, len, timeout);
	if(ret < 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
	return (unsigned int)ret;
}

unsigned int USBBuffer::bulkTransfer(USBDevice &device, unsigned char endpoint, int len, unsigned int timeout) {
	int transfered;
	int ret = libusb_bulk_transfer(device.getDeviceHandle(), endpoint, mBuffer, len, &transfered, timeout);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
	return (unsigned int)transfered;
}

unsigned int USBBuffer::interruptTransfer(USBDevice &device, unsigned char endpoint, int len, unsigned int timeout) {
	int transfered;
	int ret = libusb_interrupt_transfer(device.getDeviceHandle(), endpoint, mBuffer, len, &transfered, timeout);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
	return (unsigned int)transfered;
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
	if(mBuffer != NULL) {
		delete[] mBuffer;
	}
}

bool USBRequest::start() {
	return true;
}

USBRequest::USBRequest(size_t packet_count, size_t buffer_size, int timeout): mTimeout(timeout) {
	for(size_t i = 0; i < packet_count; ++i) {
		idle_buffers.push_back(std::make_shared<USBBuffer>(buffer_size));
	}
}

void USBRequest::receive(const USBBuffer::Ptr &buffer) {
	std::unique_lock<std::recursive_mutex> lock(*this);
	int status = buffer->getStatus();
	
	// Reset buffer
	processing_buffers.remove(buffer);
	if(mCallback) {
		mCallback(status, buffer);
	}
	idle_buffers.push_back(buffer);
	
	mBytes -= buffer->getBufferSize();
	if(status == LIBUSB_TRANSFER_COMPLETED) {
		//std::cerr << "Received " << buffer->getActualLength() << std::endl;
		mBytes += buffer->getActualLength();
	} else {
		std::cerr << "Error " << status << " with buffer 0x" << buffer << std::endl;
		mError = status;
	}

	handleBuffers();
}

void USBRequest::handleBuffers() {
	while(!idle_buffers.empty() && mError == 0 && (mRequestedBytes < 0 || (mRequestedBytes - mBytes > 0)) && !isCompleted()) {
		USBBuffer::Ptr buffer = idle_buffers.front();
		idle_buffers.pop_front();

		size_t size = mRequestedBytes - mBytes;
		if(size > buffer->getBufferSize() || mRequestedBytes < 0)
			size = buffer->getBufferSize();
		mBytes += size;
		try {
			buffer->fillBulkTransfer(*mDevice, std::bind(&USBRequest::receive, this, std::placeholders::_1), mEndpoint, size, mTimeout);
			processing_buffers.push_back(buffer);
		} catch(USBException &ex) {
			std::cerr << "Can't send: " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
			mError = ex.code();
		}
	}
	
	if((mRequestedBytes >=0 && (mRequestedBytes - mBytes) == 0) || mError != 0 || isCompleted()) {
		if(processing_buffers.empty()) {
			std::cerr << "Request end: (remaining ";
			if(mRequestedBytes > 0) {
				std::cerr << mRequestedBytes - mBytes;
			} else {
				std::cerr << "infinite";
			}
			std::cerr << " bytes)" << std::endl;
			if(mError != 0) {
				std::cerr << "Request error: " << mError << std::endl;
			}
			if(mCallback) {
				mCallback(mError, USBBuffer::Ptr());
			}
			terminate();
		}
	}
}

void USBRequest::terminate() {
	USBAsync::terminate();
	mDevice.reset();
}

bool USBRequest::send(const USBDevice::Ptr &device, unsigned char endpoint, size request_bytes, std::function<void (int, const USBBuffer::Ptr &)> callback) {
	std::unique_lock<std::recursive_mutex> lock(*this);
	if(USBAsync::start()) {
		mRequestedBytes = request_bytes;
		std::cerr << "Request start: (requested ";
		if(mRequestedBytes > 0) {
			std::cerr << mRequestedBytes;
		} else {
			std::cerr << "infinite";
		}
		std::cerr << " bytes)" << std::endl;
		mBytes = 0;
		mDevice = device;
		mError = 0;
		mEndpoint = endpoint;
		mCallback = callback;
		handleBuffers();
		return true;
	} else {
		std::cerr << "Request failed!" << std::endl;
		return false;
	}
}

USBRequest::~USBRequest() {
	wait<>();
	while(!idle_buffers.empty()) {
		idle_buffers.pop_back();
	}
}

USBDevice::USBDevice(libusb_device_handle *hnd, bool own): mDeviceHandle(hnd), mOwn(own) {
	if(mDeviceHandle == NULL) {
		throw std::runtime_error("No Device");
	}
}


bool USBDevice::isKernelDriverActive(int interface) const {
	int ret = libusb_kernel_driver_active(mDeviceHandle, interface);
	if(ret == 0) {
		return false;
	} else if(ret == 1) {
		return true;
	} else {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

void USBDevice::detachKernelDriver(int interface) {
	int ret = libusb_detach_kernel_driver(mDeviceHandle, interface);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

void USBDevice::attachKernelDriver(int interface) {
	int ret = libusb_detach_kernel_driver(mDeviceHandle, interface);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

void USBDevice::setConfiguration(int configuration) {
	int ret = libusb_set_configuration(mDeviceHandle, configuration);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

int USBDevice::getConfiguration() const {
	int configuration;
	int ret = libusb_get_configuration(mDeviceHandle, &configuration);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
	return configuration;
}

void USBDevice::claimInterface(int interface) {
	int ret = libusb_claim_interface(mDeviceHandle, interface);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

void USBDevice::setInterfaceAltSetting(int interface, int setting) {
	int ret = libusb_set_interface_alt_setting(mDeviceHandle, interface, setting);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

void USBDevice::releaseInterface(int interface) {
	int ret = libusb_release_interface(mDeviceHandle, interface);
	if(ret != 0) {
		throw USBException(ret, __PRETTY_FUNCTION__);
	}
}

USBDevice::~USBDevice() {
	if(mOwn) {
		libusb_close(mDeviceHandle);
	}
}

libusb_device_handle* USBDevice::getDeviceHandle() const {
	return mDeviceHandle;
}

USBContext::USBContext(libusb_context *context): mContext(context), mOwn(false) {
}

USBContext::USBContext(): mOwn(true) {
	libusb_init(&mContext);
}

USBContext::~USBContext() {
	wait<>();
	if(mOwn) {
		libusb_exit(mContext);
	}
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

void USBContext::setDebug(int level) {
	libusb_set_debug(mContext, level);
}

USBDevice::Ptr USBContext::openDeviceWithVidPid(uint16_t vendor, uint16_t device) const {
	return std::make_shared<USBDevice>(libusb_open_device_with_vid_pid(mContext, vendor, device), true);
}

