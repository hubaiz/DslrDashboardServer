DslrDashboardServer
===================
  
 DslrDashboardServer (ddserver) allows network connections (wired/wireless) for connected USB imaging devices (DSLR cameras) to DslrDashboard.
 It can provide multiple USB camera connections to same or different DslrDashboard client.
 It is primary writen for OpenWrt (TP-Link MR3040 or WR703N) but it will run on any Linux distribution including Raspbery Pi or pcDuino for example.
 
 
 **Multiple device connections was introduced in V0.2 and it can be used with DslrDashboard V0.30.24 and up.**
 
 After DslrDashboard has connect to ddserver, ddserver will send the connected USB cameras list.
 If there is only one USB camera connected then DslrDashboard will open that one.
 If there are more USB cameras connected DslrDashboard will display a dialog where you can select the desired camera.
 
 You can find installation image for MR3040 at https://code.google.com/p/dslrdashboard/downloads/list
 or you can contact me if you have problem building your own image.
 
 **Added a simple device discovery**
 
## Compile DslrDashboard Server on OpenWrt
 
 Clone the *DslrDashboardServer* repository to the *packages* directory on OpenWrt:
 
 	git clone git://github.com/hubaiz/DslrDashboardServer packages/DslrDashboardServer
 
 Run then the following commands:
 
 	make menuconfig
 
 Under **Multimedia** select **ddserver**. Build OpenWrt or just a single package. The resulting package can be found in the *bin/[platform]/packages/ddserver_0.1-1.ipk* directory. If you selected * the package will be already installed in the OpenWrt image.
 Upon installation DDserver will add an init script so you can start/stop it from the web interface (System->Startup).
 
## Compile DslrDashboard Server on Linux
 
 Switch to the *DslrDashboardServer/src* directory, and compile the source code:
 
 	g++ -Wall main.cpp communicator.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/i386-linux-gnu/libusb-1.0.a lpthread -lrt -lstdc++ -o ddserver

or on x64 systems

	g++ -Wall main.cpp communicator.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/x86_64-linux-gnu/libusb-1.0.a lpthread -lrt -lstdc++ -o ddserver

Make the resulting *ddserver* binary executable and launch the server:

	chmod +x ddserver
	./ddserver

## Compile DslrDashboard Server on Raspberry Pi

Install Git and libusb-1.0-0-dev:
 
 	sudo apt-get install git libusb-1.0-0-dev
 
 Clone the *DslrDashboardServer* repository and switch to the *src* directory:
 
 	git clone git://github.com/hubaiz/DslrDashboardServer
 	cd DslrDashboardServer/src
 
## Compile the server using the command bellow:
 
 	g++ -Wall main.cpp communicator.cpp -I/usr/include/libusb-1.0/ -L/usr/lib /usr/lib/arm-linux-gnueabihf/libusb-1.0.a -lpthread -lrt -lstdc++ -o ddserver
 
 Make the resulting *ddserver* binary executable and launch the server:
 
 	chmod +x ddserver
 	./ddserver
