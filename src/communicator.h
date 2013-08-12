/*
 * communicator.h
 *
 *  Created on: Aug 11, 2013
 *      Author: hubaiz
 */

#include <syslog.h>
#include <libusb.h>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <list>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef COMMUNICATOR_H_
#define COMMUNICATOR_H_

#define PTP_HEADER 12

struct ImagingUsbDevice {
	uint16_t iVendorId;
	uint16_t iProductId;
	unsigned char iVendorName[255];
	unsigned char iProductName[255];
};

struct PtpPacket{
	uint32_t packet_len;
	uint16_t packet_type;
	uint16_t packet_command;
	uint32_t session_ID;
};

class Communicator {
	int mSocket;
	libusb_context *mCtx;
	bool mIsInitialized;
	bool mIsUsbInitialized;
	pthread_t clientThread;

	libusb_device *mDevice;
	libusb_device_handle *mHandle;

	int mImagingInterface;
	bool mInterfaceClaimed;
	uint8_t mReadEndpoint;
	uint8_t mWriteEndpoint;

	void startListening();
	bool readFromClient();
	bool processPacket(uint8_t * buf, int size);
	bool initUsbDevice(uint16_t vendorId, uint16_t productId);
	bool claimInterface(uint8_t readEp, uint8_t writeEp, int interfaceNo);
	void closeUsbDevice();
	int sendBuffer(uint8_t *buf, int size);
	int sendResponsePacket(uint16_t responseCode, uint32_t sessionId);
	bool processUsbPacket(uint8_t * buf, int size);
	uint8_t * readUsbPacket(int &length);
	bool readPtpPacket(uint8_t *buf, int bufSize, int &length);
	bool handleIncomingUsbPtpPacket();

	std::list<ImagingUsbDevice> enumerateUsbImagingDevices();
	void isUsbImagingDevice(libusb_device *dev, std::list<ImagingUsbDevice> *deviceList);
	void sendDeviceList(int clientSocket, std::list<ImagingUsbDevice> *deviceList);


public:
	Communicator(int socket);
	virtual ~Communicator();

	void handleClientConnection();

};

#endif /* COMMUNICATOR_H_ */
