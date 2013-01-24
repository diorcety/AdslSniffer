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

USBRequest::USBRequest(size_t packet_count, size_t buffer_size) {
	for(size_t i = 0; i < packet_count; ++i) {
		idle_buffers.push_back(new USBBuffer(buffer_size));
	}
}

void USBRequest::receive(USBBuffer *buffer) {
	std::unique_lock<std::mutex> lock(mMutex);
	bytes += buffer->getLength();
	if(buffer->getStatus() == LIBUSB_TRANSFER_COMPLETED) {
		//std::cout << "Receive " << buffer->getActualLength() << std::endl;
		bytes -= buffer->getActualLength();
		/*int tt = buffer->getActualLength();
		unsigned char * bb = buffer->getBuffer();
		std::cout << *((unsigned short*)bb) << " " << (int)bb[tt-3] << 
		" " << *((unsigned short*)(bb + tt - 2)) << std::endl; */
	} else {
		std::cout << "Error " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
	}
	processing_buffers.remove(buffer);
	idle_buffers.push_back(buffer);
	addBuffers();
	//mCond.notify_all();
}

void USBRequest::addBuffers() {
	while(!idle_buffers.empty() && err == 0 && bytes > 0) {
		USBBuffer *buffer = idle_buffers.front();
		idle_buffers.pop_front();

		size_t size = bytes;
		if(size > buffer->getBufferSize())
			size = buffer->getBufferSize();
		bytes -= size;
		//std::cout << "Send " << size << " remaining " << bytes << std::endl;
		int ret = buffer->send(mDevice, std::bind(&USBRequest::receive, this, std::placeholders::_1), ep, size, 3000);
		if(ret == 0) {
			//std::cout << "Ok" << std::endl; 
			processing_buffers.push_back(buffer);
		} else {
			std::cout << "Can't send: " << buffer->getStatus() << " with buffer 0x" << buffer << std::endl;
			err = ret;
		}
	}
	if(bytes == 0 || err == 0) {
		if(processing_buffers.empty()) {
			done = 1;
		}
	}
}

int USBRequest::request(libusb_context* ctx, USBDevice *device, unsigned char endpoint, size_t request_bytes) {
	std::cout << "Request start" << std::endl;
	bytes = request_bytes;
	mDevice = device;
	err = 0;
	done = 0;
	ep = endpoint;
	addBuffers();
	while(!done) {
		libusb_handle_events_completed(ctx, &done);
	}
	std::cout << "Remaining: " << bytes << std::endl;
	if(err != 0) {
		std::cout << "Erreur" << std::endl;
	}
	return err;
}

void USBRequest::waitAll() {
	std::cout << "Wait all" << std::endl;
	std::unique_lock<std::mutex> lock(mMutex);
	while(!processing_buffers.empty()) {
		mCond.wait(lock);
	}
}

USBRequest::~USBRequest() {
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

