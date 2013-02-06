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

#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef DSLRDASHBOARDSERVER_H_
#define DSLRDASHBOARDSERVER_H_

libusb_device *usbDevice = NULL;
libusb_device_handle *handle = NULL;
uint16_t usbVendorId = 0;
uint16_t usbProductId = 0;

libusb_context *ctx = NULL; //a libusb session
int imagingInterface = -1;
bool interfaceClaimed = false;
uint8_t readEndpoint = 0;
uint8_t writeEndpoint = 0;

int findImagingInterface(libusb_device *dev);
bool findImagingDevice();
bool initUsbDevice();
void closeUsb();
uint8_t* readPtpPacket(int &length);
void startSocketServer();
void listenClient(int clientSocket);

typedef struct PtpPacket{
	uint32_t packet_len;
	uint16_t packet_type;
	uint16_t packet_command;
	uint32_t session_ID;
} PtpPacketPtr;


typedef struct PtpUsbResponse {
	struct PtpPacket ptpPacket;
	uint32_t vendorId;
	uint32_t productId;
} PtpUsbResponsePtr;
#endif /* DSLRDASHBOARDSERVER_H_ */
