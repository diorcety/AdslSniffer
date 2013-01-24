#ifndef __USB__
#define __USB__
#include <mutex>
#include <condition_variable>
#include <list>
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
	int err;
	int ep;
	int done;
	void waitAll();
	void addBuffers();
	void receive(USBBuffer *buffer);
	size_t bytes;
public:
	USBRequest(size_t packet_count, size_t buffer_size);
	int request(libusb_context* ctx, USBDevice *device, unsigned char endpoint, size_t bytes);
	~USBRequest();
};

class USBDevice {
private:
	libusb_device_handle * mDeviceHandle;
public:
	USBDevice(libusb_device_handle *hnd);

	libusb_device_handle* getDeviceHandle() const;
};

#endif