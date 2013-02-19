/*
DslrDashboardServer for OpenWrt
Copyright (C) 2013  Zoltan Hubai

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

#include "DslrDashboardServer.h"

int main() {

	startSocketServer();
	return 0;
}

void startSocketServer() {
	int sockfd, portno;
	int clientSocket;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int r;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		printf("ERROR opening socket\n");
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = 4757;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding\n");
		return;
	}
	listen(sockfd, 5);

	clilen = sizeof(cli_addr);

	while (true) {
		// await client connections (only 1 client)
		printf("Awaiting client connection\n");
		clientSocket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (clientSocket < 0) {
			printf("ERROR on accept\n");
		} else {
			// do we have an USB imaging device
			if (findImagingDevice()) {
				// init the USB imaging device
				if (initUsbDevice()) {
					// start listening for client packets
					listenClient(clientSocket);

					// client connection closed
					// reset the usb device
					r = libusb_reset_device(handle);
					if (r != 0) {
						printf("Error reseting USB device: %i\n", r);
					} else
						printf("USB reset OK\n");

					// close the USB device
					closeUsb();
				}
			}
			// close client socket
			close(clientSocket);
			printf("Client finished\n");
		}
	}
}

void listenClient(int clientSocket) {

	uint8_t *buf = (uint8_t *) malloc(1024);
	int r = 0;
	bool again = true;
	int doRetry = 0;

	while (again) {
		r = read(clientSocket, &buf[0], 1024);
		if (r < 0) {
			printf("Error reading from socket: %d\n", r);
			break;
		}
		if (r == 0) {
			printf("Error reading from socket 0\n");
			break;
		}

		int writen = 0;
		int i = 0;
		bool isFound = false;
		while (i < r) {
			if (buf[i] == 0x55 && ((i + 1) < r) && buf[i + 1] == 0xaa) {
				isFound = true;
				break;
			}
			i++;
		}

		if (isFound) {
			uint32_t len = le32toh(*(uint32_t *) &buf[i + 2]);
			if ((int) len == r) {
				PtpPacketPtr * ptp = (PtpPacketPtr *) &buf[i + 6];
				if (ptp->packet_command == 1) {
					printf("USB device vendor and product ID command\n");
					// return the usb vendor and product id response
					uint8_t *responseData = (uint8_t *) malloc(26);
					responseData[0] = 0x55;
					responseData[1] = 0xaa;
					uint32_t *tst = (uint32_t *)  &responseData[2];
					*tst = htole32(26);
					PtpUsbResponsePtr * usbResponse =
							(PtpUsbResponsePtr *) &responseData[6];
					(*usbResponse).ptpPacket.packet_len = htole32(12 + 8); // ptp packet + 2 * 4
					(*usbResponse).ptpPacket.packet_type = 0x03; // response
					(*usbResponse).ptpPacket.packet_command = htole16(0x2001); // OK response
					(*usbResponse).ptpPacket.session_ID = 1;
					(*usbResponse).vendorId = htole32((uint32_t) usbVendorId);
					(*usbResponse).productId = htole32((uint32_t) usbProductId);
					r = write(clientSocket, responseData, 6 + 12 + 8);
					free(responseData);
					if (r < 0) {
						printf("Error sending USB response\n");
						break;
					}
				} else {
					//cout<<"PTP Command: "<<(int)ptp->packet_command<<endl;
					int pLen = le32toh(ptp->packet_len);
					doRetry = 0;
					while(true) {
						r = libusb_bulk_transfer(handle, writeEndpoint,	(uint8_t *) ptp, pLen, &writen, 800);
						if (r != 0) {
							printf("Error write USB command packet: %d\n", r);
							doRetry++;
							if (doRetry == 5)
								break;
						}
						else
							break;
					}
					if (r != 0)
						break;
					if (pLen != writen) {
						printf("Command packet pLen: %d writen: %d\n", pLen, writen);
					}
					if ((int) len > (pLen + 6)) {
						// send the data packet
						ptp = (PtpPacketPtr *) &buf[i + 6 + pLen];
						pLen = le32toh(ptp->packet_len);
						r = libusb_bulk_transfer(handle, writeEndpoint,
								(uint8_t *) ptp, pLen, &writen, 800);
						if (r != 0) {
							printf("Error write USB data packet: %d\n", r);
							break;
						}
						if (pLen != writen) {
							printf("Command data packet pLen: %d writen: %d\n", pLen, writen);
						}
					}
					// wait for answer
					uint8_t *dataPacket = NULL;
					uint8_t *responsePacket = NULL;
					int dataPacketLength = 0, responsePacketLength = 0;
					bool doAgain = true;
					while (doAgain) {
						int read = 0;
						uint8_t *inData = readPtpPacket(read);
						if (inData != NULL) {
							PtpPacketPtr *ptr = (PtpPacketPtr *) inData;

							switch (le16toh(ptr->packet_type)) {
							case 3:
								//cout<<"Got response packet len: "<<(int)read<<endl;
								responsePacket = inData;
								responsePacketLength = read;
								// if we got a response packet then we finished
								doAgain = false;
								break;
							case 2:
								//cout<<"Got data packet len: "<<(int)read<<endl;
								dataPacket = inData;
								dataPacketLength = read;
								break;
							}

						} else {
							printf("No packet read from USB\n");
							break;
						}
					}
					//cout<<"Got USB response"<<endl;
					int respLength = 6 + responsePacketLength
							+ dataPacketLength;
					uint8_t *responseData = (uint8_t *) malloc(respLength);
					responseData[0] = 0x55;
					responseData[1] = 0xaa;
					uint32_t *tst = (uint32_t *) &responseData[2];
					*tst = htole32(6 + responsePacketLength + dataPacketLength);

					int index = 6;
					if (responsePacket != NULL) {
						memcpy((uint8_t *) &responseData[index], responsePacket,
								responsePacketLength);
						index += responsePacketLength;
						free(responsePacket);
					}
					if (dataPacket != NULL) {
						memcpy((uint8_t *) &responseData[index], dataPacket,
								dataPacketLength);
						index += dataPacketLength;
						free(dataPacket);
					}

					r = write(clientSocket, responseData, respLength);

					free(responseData);
					if (r < 0) {
						printf("Error sending response packet to client: %d\n", r);
						break;
					}
					if (r == 0) {
						printf("Error sending response packet to client 0\n");
						break;
					}
					//cout<<"USB response sent"<<endl;
				}
			}
		}
	}
	free(buf);
}

uint8_t* readPtpPacket(int &length) {
	int retry = 0;
	length = 0;
	uint8_t *buf = (uint8_t *) malloc(1024);
	int r, readBytes = 0;

	while (true) {
		r = libusb_bulk_transfer(handle, readEndpoint, buf, 1024, &readBytes,
				2000);
		if (r == 0) {
			if (readBytes > 0)
				break;
			else {
				retry++;
				if (retry > 10) {
					printf("Result is 0 but no USB data after 10 retries\n");
					free(buf);
					return NULL;
				}
			}
		}
		if (r == -1) {
			retry++;
			if (retry > 10) {
				printf("No USB data after 10 retries\n");
				free(buf);
				return NULL;
			}
		}
	}
	if (r == 0) {
		PtpPacketPtr *ptpPacket = (PtpPacketPtr *) buf;
		int len = le32toh(ptpPacket->packet_len);
		int read = readBytes;
		if (len > read) {
			buf = (uint8_t *) realloc(buf, len);
			retry = 0;
			while (true) {
				r = libusb_bulk_transfer(handle, readEndpoint,
						(uint8_t *) &buf[read], len - read, &readBytes, 2000);
				if (r == 0 && readBytes == (len - read)) {
					length = len;
					return buf;
				} else if (r == -1) {
					retry++;
					if (retry > 10) {
						printf("No read after 10 retries for USB second packet read: %d length: %d\n", read, len);
						free(buf);
						return NULL;
					}
				} else {
					printf("Error reading USB second packet: %d\n", r);
					free(buf);
					break;
				}
			}
		} else {
			length = len;
			return buf;
		}
	} else {
		printf("Error reading USB packet: %d\n", r);
		free(buf);
	}
	return NULL;
}

bool initUsbDevice() {
	bool initialized = false;
	int r;

	if (findImagingDevice()) {
		if (libusb_kernel_driver_active(handle, 0) == 1) { //find out if kernel driver is attached
			printf("Kernel driver active\n");
			if (libusb_detach_kernel_driver(handle, 0) == 0) //detach it
				printf("kernel driver detached!\n");
		}
	}
	if (imagingInterface >= 0) {
		r = libusb_claim_interface(handle, imagingInterface); //claim imaging interface
		if (r < 0) {
			printf("Cannot claim interface\n");
			return initialized;
		}
		printf("Claimed interface\n");
		interfaceClaimed = true;
		initialized = true;
	}
	return initialized;
}

void closeUsb() {
	int r;
	if (handle != NULL) {
		if (interfaceClaimed) {
			printf("Releasing interface\n");
			r = libusb_release_interface(handle, imagingInterface);
			if (r != 0) {
				printf("Cannot release interface\n");
			}
		}
		printf("Closing USB device\n");
		libusb_close(handle);
	}
	usbVendorId = 0;
	usbProductId = 0;

	if (ctx != NULL) {
		printf("Exit libusb\n");
		libusb_exit(ctx); //close the session
	}
}

// find imaging device (interface class = 6)
bool findImagingDevice() {
	readEndpoint = 0;
	writeEndpoint = 0;

	bool result = false;
	libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices

	int r; //for return values
	ssize_t cnt; //holding number of devices in list
	r = libusb_init(&ctx); //initialize a library session
	if (r < 0) {
		printf("Init error: %d\n", r);
		return result;
	}
	libusb_set_debug(ctx, 0); //set verbosity level to 3, as suggested in the documentation
	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if (cnt < 0) {
		printf("Get device error\n");
	}
	printf("Devices in list\n");
	ssize_t i; //for iterating through the list
	for (i = 0; i < cnt; i++) {
		libusb_device *device = devs[i];
		imagingInterface = findImagingInterface(device);
		if (imagingInterface >= 0) {
			r = libusb_open(device, &handle);
			if (r == 0) {
				printf("USB imaging device opened\n");
				usbDevice = device;
				result = true;
				break;
			} else
				printf("Error opening USB device\n");
		}
	}

	libusb_free_device_list(devs, 1); //free the list, unref the devices in it

	return result;

}

// find imaging interface (Class = 6)
int findImagingInterface(libusb_device *dev) {
	int found = -1;
	libusb_device_descriptor desc;
	int r = libusb_get_device_descriptor(dev, &desc);
	if (r < 0) {
		printf("Failed to get device descriptor: %d\n", r);
		return found;
	}
	printf("Number of possible configurations: %d Device Class: %d VendorID: %d, ProductID: %d\n", desc.bNumConfigurations, desc.bDeviceClass, desc.idVendor, desc.idProduct);

	usbVendorId = desc.idVendor;
	usbProductId = desc.idProduct;

	libusb_config_descriptor *config;
	libusb_get_config_descriptor(dev, 0, &config);

	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;

	int i = 0;
	bool again = true;
	while (again) {
		if (i >= config->bNumInterfaces)
			break;

		inter = &config->interface[i];
		printf("Number of alternate settings:\n");

		int j = 0;
		while (j < inter->num_altsetting) {
			interdesc = &inter->altsetting[j];

			printf("Interface class: %d Interface number: %d Number of endpoints: %d\n", interdesc->bInterfaceClass, interdesc->bInterfaceNumber, interdesc->bNumEndpoints);

			if (interdesc->bInterfaceClass == 6) {
				for (int k = 0; k < (int) interdesc->bNumEndpoints; k++) {

					epdesc = &interdesc->endpoint[k];

					printf("Endpoint descriptor type: %d address: %d\n", epdesc->bDescriptorType, epdesc->bEndpointAddress);


					if ((epdesc->bEndpointAddress
							== (LIBUSB_ENDPOINT_IN
									| LIBUSB_TRANSFER_TYPE_ISOCHRONOUS))
							|| (epdesc->bEndpointAddress
									== (LIBUSB_ENDPOINT_IN
											| LIBUSB_TRANSFER_TYPE_BULK)))
						readEndpoint = epdesc->bEndpointAddress;

					if ((epdesc->bEndpointAddress
							== (LIBUSB_ENDPOINT_OUT
									| LIBUSB_TRANSFER_TYPE_ISOCHRONOUS))
							|| (epdesc->bEndpointAddress
									== (LIBUSB_ENDPOINT_OUT
											| LIBUSB_TRANSFER_TYPE_BULK)))
						writeEndpoint = epdesc->bEndpointAddress;

				}
				printf("Found imaging device with write endpoint: %d and read endpoint: %d\n", writeEndpoint, readEndpoint);

				found = i;
				again = false;
				break;
			}
			j++;
		}
		i++;
	}
	libusb_free_config_descriptor(config);
	return found;
}

