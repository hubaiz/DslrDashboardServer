/*
 * communicator.cpp
 *
 *  Created on: Aug 11, 2013
 *      Author: hubaiz
 */

#include "communicator.h"

Communicator::Communicator(int socket) : mSocket(socket), mIsInitialized(false), mIsUsbInitialized(false),
															mHandle(NULL), mDevice(NULL), mImagingInterface(-1), mInterfaceClaimed(false)
{
	int r = libusb_init(&mCtx);
	if (r == 0) {
		mIsUsbInitialized = true;
		libusb_set_debug(mCtx, 0); //set verbosity level to 3, as suggested in the documentation
	} else
		syslog(LOG_ERR, "Error initializing libusb %d", r);
}

Communicator::~Communicator() {
	libusb_exit(mCtx);
}

void Communicator::startListening() {
	while(true) {
		if (!readFromClient()) {
			syslog(LOG_ERR, "Stoping client");
			break;
		}
	}

	sleep(2);

	closeUsbDevice();

	close(mSocket);
}


bool Communicator::readFromClient() {
	bool result = false;
	int r = 0;
	uint32_t packetSize = 0;

	r = read(mSocket, &packetSize, 4);

	if (r == 4) {
		packetSize = le32toh(packetSize);
		//syslog(LOG_INFO, "incoming packet size: %d", packetSize);

		uint8_t *buf = (uint8_t *) malloc(packetSize - 4);

		r = read(mSocket, buf, packetSize - 4);

		if (r == (packetSize - 4)) {
			result = processPacket(buf, packetSize - 4);
		}
		else
			syslog(LOG_ERR, "Error reading packet : %d", r);

		free(buf);

	} else
		syslog(LOG_ERR, "Error reading total packet size: %d", r);



	return result;
}

bool Communicator::processPacket(uint8_t *buf, int size) {
	//syslog(LOG_INFO, "processing incoming packet");
	PtpPacket *header = (PtpPacket *)&buf[0];

	// device selector command, if initialized then ignore
	if (le16toh(header->packet_command) == 0x0001) {
		if (mIsInitialized) {
			// session already open
			sendResponsePacket(0x201e, header->session_ID);
			return true;
		}

		// read vendor and product id from command param, PTP header + 2 * uint32
		if (le32toh(header->packet_len) >= PTP_HEADER + 8) {
			// read vendor id
			uint32_t vendorId = le32toh(*(uint32_t *)&buf[PTP_HEADER]);
			// read product id
			uint32_t productId = le32toh(*(uint32_t *)&buf[PTP_HEADER+4]);

			// try to open usb device
			mIsInitialized = openUsbDevice((uint16_t)vendorId, (uint16_t)productId); //initUsbDevice((uint16_t)vendorId, (uint16_t)productId);

			// ok
			sendResponsePacket(mIsInitialized ? 0x2001 : 0x2002, header->session_ID);

			return mIsInitialized;
		} else {
			// general error
			sendResponsePacket(0x2002, le32toh(header->session_ID));
			return false;
		}
	} else {
		// if not initialized then ignore
		if (!mIsInitialized) {
			// session not open
			sendResponsePacket(0x2003, le32toh(header->session_ID));
			return true;
		} else {
			return processUsbPacket(buf, size);
		}
	}
}
bool Communicator::processUsbPacket(uint8_t * buf, int size){
	bool result = false;
	PtpPacket *header = (PtpPacket *)&buf[0];
	int packetSize = le32toh(header->packet_len);
	int writen = 0;

	//syslog(LOG_INFO, "sending packet to USB device");

	int r = libusb_bulk_transfer(mHandle, mWriteEndpoint, &buf[0], packetSize, &writen, 800);
	//syslog(LOG_INFO, "packet size: %d  writen to USB: %d", packetSize, writen);
	if (r == 0) {
		if (writen != packetSize)
			syslog(LOG_ERR, "Command Packet size was: %d  writen: %d", packetSize, writen);
		if (size > packetSize) {
			//syslog(LOG_INFO, "sending PTP data packet");
			// there is a data packet, write
			header = (PtpPacket *)&buf[packetSize];
			r = libusb_bulk_transfer(mHandle, mWriteEndpoint, &buf[packetSize], le32toh(header->packet_len), &writen, 800);
			if (r == 0) {
				if (writen != le32toh(header->packet_len))
					syslog(LOG_ERR, "Command data Packet size was: %d  writen: %d", le32toh(header->packet_len), writen);

				result = handleIncomingUsbPtpPacket();
			}
			else
				syslog(LOG_ERR, "Error command data packet USB bulk write: %d", r);
		}
		else
			result = handleIncomingUsbPtpPacket();
	}
	else
		syslog(LOG_ERR, "Error command packet USB bulk write: %d", r);
	return result;
}

bool Communicator::handleIncomingUsbPtpPacket() {
	bool result = false;
	int r, readBytes = 0;
	uint8_t *readBuf = NULL;

	readBuf = readUsbPacket(readBytes);
	if (readBuf != NULL) {
		r = sendBuffer(readBuf, readBytes);
		free(readBuf);

		result = (r == readBytes);
	}
	return result;
}

uint8_t * Communicator::readUsbPacket(int &length) {
	uint32_t packetSize1 = 0, packetSize2 = 0;
	int readBytes = 0, currentPacketSize = 4100, offset = 4;
	PtpPacket *ptpPacket;
	bool isResponse = false, resume = true;

	uint8_t *buf = (uint8_t *)malloc(currentPacketSize);

	if (readPtpPacket(&buf[offset], currentPacketSize - offset, readBytes)) {
		//syslog(LOG_INFO, "first currentPacketSize: %d  offset: %d   readBytes: %d", currentPacketSize, offset, readBytes);

		ptpPacket = (PtpPacket *)&buf[4];
		packetSize1 = le32toh(ptpPacket->packet_len);
		isResponse = le16toh(ptpPacket->packet_type) == 0x0003;
		//syslog(LOG_INFO, "packetSize1: %d", packetSize1);

		if (packetSize1 > (currentPacketSize - 4)) {
			offset = currentPacketSize;
			currentPacketSize = 4 + packetSize1 + 4096;
			buf = (uint8_t *)realloc(buf, currentPacketSize);
			if (readPtpPacket(&buf[offset], packetSize1 - (offset-4 ), readBytes)) {
				//syslog(LOG_INFO, "second packetSize1: %d currentPacketSize: %d  offset: %d   readBytes: %d", packetSize1, currentPacketSize, offset, readBytes);
			}
			else
				resume = false;
		}

		if (resume) {
			length = packetSize1 + 4;
			*(uint32_t *)&buf[0] = htole32(length);

			if (!isResponse) {
				//syslog(LOG_INFO, "first was data, read response");
				// read in the respone
				offset = 4 + packetSize1;
				if (readPtpPacket(&buf[offset], currentPacketSize - packetSize1 - 4, readBytes)) {

					ptpPacket = (PtpPacket *)&buf[offset];
					packetSize2 = le32toh(ptpPacket->packet_len);

					if (packetSize2 > (currentPacketSize - packetSize1 - 4)) {
						//syslog(LOG_INFO, "increase buffer");
						offset = currentPacketSize;
						currentPacketSize = 4 + packetSize1 + packetSize2;
						buf = (uint8_t *)realloc(buf, currentPacketSize);

						if (readPtpPacket(&buf[offset], packetSize2 - (offset - 4 - packetSize1), readBytes)) {
						}
					}
					length = 4 + packetSize1 + packetSize2;
					*(uint32_t *)&buf[0] = htole32(length);
					return buf;
				}
			} else
				return buf;
		}

	}

	syslog(LOG_ERR, "error reading USB PTP packets");
	free(buf);
	length = 0;
	return NULL;
}

bool Communicator::readPtpPacket(uint8_t *buf, int bufSize, int &length ) {
	bool success = false;
	int retry = 0;
	length = 0;
	int r, readBytes = 0;

	//syslog(LOG_INFO,"readPtpPacket");
	while (true) {
		r = libusb_bulk_transfer(mHandle, mReadEndpoint, buf, bufSize, &readBytes, 2000);
		//syslog(LOG_INFO, "USB read result: %d  read bytes: %d", r, readBytes);
		if (r == 0) {
			if (readBytes > 0) {
				success = true;
				length = readBytes;
				break;
			} else {
				retry++;
				if (retry > 10) {
					syslog(LOG_ERR, "Result is 0, read bytes %d", readBytes);
					break;
				}
			}
		}
		if (r == -1) {
			retry++;
			if (retry > 10) {
				syslog(LOG_ERR, "No USB data after 10 retries");
				break;
			}
		}
	}
	return success;
}


int Communicator::sendResponsePacket(uint16_t responseCode, uint32_t sessionId) {
	int len = sizeof(PtpPacket) + 4;
	uint8_t *buf = (uint8_t *)malloc(len);

	*(uint32_t *)&buf[0] = (uint32_t)htole32(len);

	PtpPacket *ptpPacket = (PtpPacket *)&buf[4];
	ptpPacket->packet_len = htole32(len - 4);
	ptpPacket->packet_type = htole16(0x0003);
	ptpPacket->packet_command = htole16(responseCode);
	ptpPacket->session_ID = htole32(sessionId);

	int r = sendBuffer(buf, len);

	free(buf);

	return r;
}
int Communicator::sendBuffer(uint8_t *buf, int size) {
	int r = write(mSocket, buf, size);
	if (r != size)
		syslog(LOG_ERR, "Error sending packet to client");
	return r;
}

bool Communicator::openUsbDevice(uint16_t vendorId, uint16_t productId) {
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_device_descriptor desc;
	int r = 0;
	ssize_t cnt; //holding number of devices in list

	std::list<ImagingUsbDevice> imgUsbDevices;

	if (mIsUsbInitialized) {

		cnt = libusb_get_device_list(mCtx, &devs); //get the list of devices
		if (cnt > 0) {
			syslog(LOG_INFO, "USB Devices in");
			ssize_t i = 0; //for iterating through the list
			while(i < cnt) {
				libusb_device *device = devs[i];
				r = libusb_get_device_descriptor(device, &desc);
				if (r == 0) {
					if (desc.idVendor == vendorId && desc.idProduct == productId) {
						if (canOpenUsbImagingDevice(device, &desc)) {
							return initUsbDevice(device);
						}
						i++;
					}
				}
			}
		}
		else
			syslog(LOG_INFO, "Can't found USB devices");
	}

	libusb_free_device_list(devs, 1); //free the list, unref the devices in it

	return false;
}

bool Communicator::initUsbDevice(libusb_device *device) {
	libusb_device_descriptor desc;
	libusb_config_descriptor *config;
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;

	int r;
	bool result = false;

	if (device != NULL) {
	mDevice = device;
	r = libusb_open(device, &mHandle);
	if (r == 0 && mHandle != NULL) {
		syslog(LOG_INFO, "USB device opened");
		r = libusb_get_device_descriptor(mDevice, &desc);
		if (r == 0) {
				r = libusb_get_config_descriptor(mDevice, 0, &config);
				if (r == 0) {
					int i = 0, j = 0;
					while (i < config->bNumInterfaces) {
						inter = &config->interface[i];
						j = 0;
						while (j < inter->num_altsetting) {
							interdesc = &inter->altsetting[j];
							if (interdesc->bInterfaceClass == 6) {
								uint8_t readEp = 129, writeEp = 2;
								for (int k = 0; k < (int) interdesc->bNumEndpoints; k++) {
									epdesc = &interdesc->endpoint[k];
									if ((epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)) || (epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_BULK))) {
										readEp = epdesc->bEndpointAddress;
										syslog(LOG_INFO, "Read endpoint adress: %d", readEp);
									}
									if ((epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_OUT | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)) || (epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_OUT	| LIBUSB_TRANSFER_TYPE_BULK))) {
										writeEp = epdesc->bEndpointAddress;
										syslog(LOG_INFO, "Write endpoint adress: %d", writeEp);
									}
								}
								result = claimInterface(readEp, writeEp, i);
								j = inter->num_altsetting;
								i = config->bNumInterfaces;
							}
							j++;
						}
						i++;
					}
					libusb_free_config_descriptor(config);
				}
				else
					syslog(LOG_ERR, "Error opening USB device config descriptor: %d", r);
		} else
			syslog(LOG_ERR, "Failed to get device descriptor: %d", r);
	}
	else
		syslog(LOG_ERR, "Error opening device");
	}
	return result;
}

bool Communicator::initUsbDevice(uint16_t vendorId, uint16_t productId) {
	libusb_device_descriptor desc;
	libusb_config_descriptor *config;
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;

	int r;
	bool result = false;

	mHandle = libusb_open_device_with_vid_pid(mCtx, vendorId, productId);
	if (mHandle != NULL) {
		syslog(LOG_INFO, "USB device opened");
		mDevice = libusb_get_device(mHandle);
		r = libusb_get_device_descriptor(mDevice, &desc);
		if (r == 0) {
				r = libusb_get_config_descriptor(mDevice, 0, &config);
				if (r == 0) {
					int i = 0, j = 0;
					while (i < config->bNumInterfaces) {
						inter = &config->interface[i];
						j = 0;
						while (j < inter->num_altsetting) {
							interdesc = &inter->altsetting[j];
							if (interdesc->bInterfaceClass == 6) {
								uint8_t readEp = 129, writeEp = 2;
								for (int k = 0; k < (int) interdesc->bNumEndpoints; k++) {
									epdesc = &interdesc->endpoint[k];
									if ((epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)) || (epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_BULK))) {
										readEp = epdesc->bEndpointAddress;
										syslog(LOG_INFO, "Read endpoint adress: %d", readEp);
									}
									if ((epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_OUT | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS)) || (epdesc->bEndpointAddress == (LIBUSB_ENDPOINT_OUT	| LIBUSB_TRANSFER_TYPE_BULK))) {
										writeEp = epdesc->bEndpointAddress;
										syslog(LOG_INFO, "Write endpoint adress: %d", writeEp);
									}
								}
								result = claimInterface(readEp, writeEp, i);
								j = inter->num_altsetting;
								i = config->bNumInterfaces;
							}
							j++;
						}
						i++;
					}
					libusb_free_config_descriptor(config);
				}
				else
					syslog(LOG_ERR, "Error opening USB device config descriptor: %d", r);
		} else
			syslog(LOG_ERR, "Failed to get device descriptor: %d", r);
	}
	else
		syslog(LOG_ERR, "Error opening device with VendorId: %d  ProductId: %d", vendorId, productId);
	return result;
}

bool Communicator::claimInterface(uint8_t readEp, uint8_t writeEp, int interfaceNo) {

	int r;
	mReadEndpoint = readEp;
	mWriteEndpoint = writeEp;
	mImagingInterface = interfaceNo;

	if (libusb_kernel_driver_active(mHandle, 0) == 1) { //find out if kernel driver is attached
		syslog(LOG_INFO, "Kernel driver active, trying to detach");
		if (libusb_detach_kernel_driver(mHandle, 0) == 0) //detach it
			syslog(LOG_INFO, "Kernel driver detached");
	}

	if (mImagingInterface >= 0) {
		r = libusb_claim_interface(mHandle, mImagingInterface); //claim imaging interface
		if (r == 0) {
			syslog(LOG_INFO, "USB interface claimed");
			mInterfaceClaimed = true;
			mIsInitialized = true;
			return true;
		}

		syslog(LOG_ERR, "Unable to claim interface");
	}
	else
		syslog(LOG_ERR, "No interface was found");
	closeUsbDevice();
	return false;
}

void Communicator::closeUsbDevice() {
	syslog(LOG_INFO, "Closing USB device");
	int r;
	if (mInterfaceClaimed && mHandle != NULL) {
		r = libusb_release_interface(mHandle, mImagingInterface);
		if (r == 0)
			syslog(LOG_INFO, "USB interface released");
		else
			syslog(LOG_ERR, "Unable to release USB interface");
	}
	if (mHandle != NULL) {
		libusb_close(mHandle);
	}
}



void Communicator::handleClientConnection() {

	std::list<ImagingUsbDevice> imgUsbDevices = enumerateUsbImagingDevices();
	if (imgUsbDevices.size() > 0) {
		sendDeviceList(mSocket, &imgUsbDevices);

		startListening();
	}
	else {
		syslog(LOG_INFO, "No USB Imaging devices found, closing socket");
		close(mSocket);
	}
}


void Communicator::sendDeviceList(int clientSocket, std::list<ImagingUsbDevice> *deviceList) {
	int bufSize = deviceList->size() * (sizeof(ImagingUsbDevice) + 4) + 4;
	syslog(LOG_INFO, "Packet size: %d", bufSize);
	uint8_t *buf = (uint8_t *)malloc(bufSize);
	int index = 0;
	*(uint32_t *)&buf[index] = htole32((uint32_t)bufSize);
	index += 4;
	for (std::list<ImagingUsbDevice>::const_iterator usbDevice = deviceList->begin(), end = deviceList->end(); usbDevice != end; ++usbDevice) {
		*(uint32_t *)&buf[index] = (uint32_t)htole32(sizeof(ImagingUsbDevice));
		index += 4;
		*(uint16_t *)&buf[index] = (uint16_t)htole16(usbDevice->iVendorId);
		index += 2;
		*(uint16_t *)&buf[index] = (uint16_t)htole16(usbDevice->iProductId);
		index += 2;
		memcpy(&buf[index], &usbDevice->iVendorName[0], 255);
		syslog(LOG_INFO, "Device Product: %s", usbDevice->iVendorName);
		index += 255;
		memcpy(&buf[index], &usbDevice->iProductName[0], 255);
		syslog(LOG_INFO, "Device Product: %s", usbDevice->iProductName);
		index += 255;
	}
	ssize_t writen = write(clientSocket, buf, bufSize);
	syslog(LOG_INFO, "Bytes writen: %d", writen);
}

std::list<ImagingUsbDevice> Communicator::enumerateUsbImagingDevices() {
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices

	ssize_t cnt; //holding number of devices in list

	std::list<ImagingUsbDevice> imgUsbDevices;

	if (mIsUsbInitialized) {

		cnt = libusb_get_device_list(mCtx, &devs); //get the list of devices
		if (cnt < 0) {
			syslog(LOG_INFO, "Can't found USB devices");
		}
		syslog(LOG_INFO, "USB Devices in");
		ssize_t i; //for iterating through the list
		for (i = 0; i < cnt; i++) {
			libusb_device *device = devs[i];
			isUsbImagingDevice(device, &imgUsbDevices);
		}
		syslog(LOG_INFO, "Imaging USB devices found: %d", imgUsbDevices.size());
	}

	libusb_free_device_list(devs, 1); //free the list, unref the devices in it

	return imgUsbDevices;
}

uint32_t Communicator::getHash(const unsigned char * str){
    uint32_t hash = 0;
    int c;

    while((c = *str++))
    {
        /* hash = hash * 33 ^ c */
        hash = ((hash << 5) + hash) ^ c;
    }

    return hash;
}

void Communicator::isUsbImagingDevice(libusb_device *dev, std::list<ImagingUsbDevice> *deviceList) {


	int r = 0;
	libusb_device_descriptor desc;
	libusb_config_descriptor *config;
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	libusb_device_handle *deviceHandle;

	r = libusb_get_device_descriptor(dev, &desc);

	if (r < 0) {
		syslog(LOG_ERR, "Failed to get device descriptor: %d", r);
		return;
	}

	syslog(LOG_INFO, "Number of possible configurations: %d Device Class: %d VendorID: %d, ProductID: %d", desc.bNumConfigurations, desc.bDeviceClass, desc.idVendor, desc.idProduct);

	r = libusb_get_config_descriptor(dev, 0, &config);
	if (r == 0) {

	int i = 0;
	bool again = true;
	while (again) {
		if (i >= config->bNumInterfaces)
			break;

		inter = &config->interface[i];
		syslog(LOG_INFO, "Number of alternate settings:");

		int j = 0;
		while (j < inter->num_altsetting) {
			interdesc = &inter->altsetting[j];

			syslog(LOG_INFO, "Interface class: %d Interface number: %d Number of endpoints: %d", interdesc->bInterfaceClass, interdesc->bInterfaceNumber, interdesc->bNumEndpoints);

			if (interdesc->bInterfaceClass == 6) {
				syslog(LOG_INFO, "Found USB imaging device, get vendor and product");
				again = false;

				r = libusb_open(dev, &deviceHandle);
				if (r != 0) {
					syslog(LOG_INFO, "Error opening USB imaging device: %d", r);
					break;
				} else {
					if (libusb_kernel_driver_active(deviceHandle, 0) == 0) { //find out if kernel driver is attached

						if( libusb_claim_interface(deviceHandle, i) == 0) {; //claim imaging interface


							ImagingUsbDevice imgUsbDevice;
//							unsigned char serial[255];
//							uint16_t lang = 0;

							imgUsbDevice.iVendorId = desc.idVendor;
							imgUsbDevice.iProductId = desc.idProduct;
							bzero(&imgUsbDevice.iVendorName[0], 255);
							bzero(&imgUsbDevice.iProductName[0], 255);

//							 libusb_get_string_descriptor(deviceHandle, 0, 0, &serial[0], 255);
//							 lang = serial[2] << 8 | serial[3];

							r = libusb_get_string_descriptor_ascii(deviceHandle, desc.iManufacturer, &(imgUsbDevice.iVendorName[0]), 255);
							if (r <= 0)
								syslog(LOG_ERR, "Error getting USB Vendor name: %d", r);
							r = libusb_get_string_descriptor_ascii(deviceHandle, desc.iProduct, &(imgUsbDevice.iProductName[0]), 255);
							if (r <= 0)
								syslog(LOG_ERR, "Error getting USB Product name: %d", r);
//							r = libusb_get_string_descriptor_ascii(deviceHandle, desc.iSerialNumber, &serial[0], 255);
//							if (r <= 0)
//								syslog(LOG_ERR, "Error getting USB serial: %d", r);

							libusb_release_interface(deviceHandle, i);
							libusb_close(deviceHandle);

							syslog(LOG_INFO, "Device Manufacturer: %s", imgUsbDevice.iVendorName);
							syslog(LOG_INFO, "Device Product: %s", imgUsbDevice.iProductName);
//							syslog(LOG_INFO, "Device Serial: %s", serial);
//							uint32_t hash = getHash(&serial[0]);
//							syslog(LOG_INFO, "Device Serial hash: %08x", hash);
							deviceList->push_front(imgUsbDevice);
						}
					}
					else
						syslog(LOG_INFO, "Kernel driver active");

				}
				break;
			}
			j++;
		}
		i++;
	}
	libusb_free_config_descriptor(config);
	}
}

bool Communicator::canOpenUsbImagingDevice(libusb_device *dev, libusb_device_descriptor *desc) {

	bool result = false;

	int r = 0;
	libusb_config_descriptor *config;
	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	libusb_device_handle *deviceHandle;


		syslog(LOG_INFO, "Number of possible configurations: %d Device Class: %d VendorID: %d, ProductID: %d",
				desc->bNumConfigurations, desc->bDeviceClass, desc->idVendor, desc->idProduct);

		r = libusb_get_config_descriptor(dev, 0, &config);
		if (r == 0) {

			int i = 0;
			bool again = true;
			while (again) {
				if (i >= config->bNumInterfaces)
					break;

				inter = &config->interface[i];
				syslog(LOG_INFO, "Number of alternate settings:");

				int j = 0;
				while (j < inter->num_altsetting) {
					interdesc = &inter->altsetting[j];

					syslog(LOG_INFO, "Interface class: %d Interface number: %d Number of endpoints: %d",
							interdesc->bInterfaceClass, interdesc->bInterfaceNumber, interdesc->bNumEndpoints);

					if (interdesc->bInterfaceClass == 6) {
						syslog(LOG_INFO, "Found USB imaging device, get vendor and product");
						again = false;

						r = libusb_open(dev, &deviceHandle);
						if (r != 0) {
							syslog(LOG_INFO, "Error opening USB imaging device: %d", r);
							break;
						} else {
							if (libusb_kernel_driver_active(deviceHandle, 0) == 0) { //find out if kernel driver is attached

								if( libusb_claim_interface(deviceHandle, i) == 0) {; //claim imaging interface

									libusb_release_interface(deviceHandle, i);

									libusb_close(deviceHandle);

									result = true;

									break;
								}
							}
							else
								syslog(LOG_INFO, "Kernel driver active");
						}
						break;
					}
					j++;
				}
				i++;
			}
			libusb_free_config_descriptor(config);
		}

	return result;
}

