DslrDashboardServer
===================

DslrDashboard Server for OpenWrt

checout the repository to your OpenWrt packages folder

git clone git://github.com/hubaiz/DslrDashboardServer packages/DslrDashboardServer

execute
make menuconfig

under Multimedia select ddserver

build OpenWrt or just a single package

your package will be in bin/_platform_/packages/ddserver_0.1-1.ipk

If you selected * the package will be already installed in the OpenWrt image.


You can compile and test in on a Linux machine.
To compile it issue the following command in the src folder:

gcc -Wall DslrDashboardServer.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/i386-linux-gnu/libusb-1.0.a -lpthread -lrt -o ddserver


