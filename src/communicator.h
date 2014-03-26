/*
 * communicator.h
 *
 *  Created on: Dec 28, 2013
 *      Author: hubaiz
 */

#ifndef COMMUNICATOR_H_
#define COMMUNICATOR_H_

#include <syslog.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef TCP_USER_TIMEOUT
    #define TCP_USER_TIMEOUT 18  // how long for loss retry before timeout [ms]
#endif

#define PTP_HEADER 12

#define DDS_MAJOR  1
#define DDS_MINOR  0
#define DDS_NAME "DslrDashboardServer"

struct ImagingUsbDevice {
	uint16_t iVendorId;
	uint16_t iProductId;
	unsigned char iVendorName[256];
	unsigned char iProductName[256];
//	unsigned char iSerial[256];
};

struct PtpPacket{
	uint32_t packet_len;
	uint16_t packet_type;
	uint16_t packet_command;
	uint32_t session_ID;
};

class Communicator {


public:
	Communicator();
	virtual ~Communicator();

	void handleClientConnection(int socket);

private:
	int mSocket;
	libusb_context *mCtx;
	//bool mIsInitialized;
	//bool mIsUsbInitialized;
	pthread_t clientThread;

	libusb_device *mDevice;
	libusb_device_handle *mHandle;

	int mImagingInterface;
	//bool mInterfaceClaimed;
	uint8_t mReadEndpoint;
	uint8_t mWriteEndpoint;

	uint16_t mVendorId;
	uint16_t mProductId;

	bool isUsbContextInitialized();
	bool isSocketInitialized();
	bool isInitialized();

	void sendWelcomeMessage();
	bool readFromClient();
	int sendResponsePacket(uint16_t responseCode, uint32_t sessionId);
	void setPtpHeader(uint8_t *buf, int offset, int size, uint16_t packetType, uint16_t packetCommand, uint32_t sessionId);
	int sendBuffer(uint8_t *buf, int size);

	void initiateUsbConnection(uint32_t vendorId, uint32_t productId, uint32_t sessionId);
	bool processPacket(uint8_t *buf, int size) ;
	bool processUsbPacket(uint8_t * buf, int size);
	bool handleIncomingUsbPtpPacket();
	uint8_t * readUsbPacket(int &length);
	bool readPtpPacket(uint8_t *buf, int bufSize, int &length );

	bool openUsbDevice(uint16_t vendorId, uint16_t productId);
	bool initUsbDevice(libusb_device *device);
	bool claimInterface(uint8_t readEp, uint8_t writeEp, int interfaceNo);
	bool canOpenUsbImagingDevice(libusb_device *dev, libusb_device_descriptor *desc);
	void closeUsbDevice();

	void sendUsbDeviceList(uint32_t sessionId);
	bool isUsbImagingDevice(libusb_device *dev, ImagingUsbDevice *imgUsbDevice);
};


#endif /* COMMUNICATOR_H_ */
