DslrDashboardServer
===================

## Compile DslrDashboard Server on OpenWrt

Clone the *DslrDashboardServer* repository to the *packages* directory on OpenWrt:

	git clone git://github.com/hubaiz/DslrDashboardServer packages/DslrDashboardServer

Run then the following commands:

	execute
	make menuconfig

Under **Multimedia** select **ddserver**. Build OpenWrt or just a single package. The resulting package can be found in the *bin/[platform]/packages/ddserver_0.1-1.ipk* directory. If you selected * the package will be already installed in the OpenWrt image.


## Compile DslrDashboard Server on Linux

Switch to the *DslrDashboardServer/src* directory, and compile the source code:

	gcc -Wall DslrDashboardServer.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/i386-linux-gnu/libusb-1.0.a -lpthread -lrt -o ddserver

Make the resulting *ddserver* binary executable and launch the server:

	chmod +x ddserver
	./ddserver

## Compile DslrDashboard Server on Raspberry Pi

Install Git and libusb-1.0-0-dev:

	sudo apt-get install git libusb-1.0-0-dev

Clone the *DslrDashboardServer* repository and switch to the *src* directory:

	git clone git://github.com/hubaiz/DslrDashboardServer
	cd DslrDashboardServer/src

Compile the server using the command bellow:

	gcc -Wall DslrDashboardServer.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/arm-linux-gnueabihf/libusb-1.0.a -lpthread -lrt -o ddserver

Make the resulting *ddserver* binary executable and launch the server:

	chmod +x ddserver
	./ddserver
