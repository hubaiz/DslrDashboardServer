DslrDashboardServer
===================

 DslrDashboardServer (ddserver) allows network connections (wired/wireless) for connected USB imaging devices (DSLR cameras) to DslrDashboard.
 It can provide multiple USB camera connections to same or different DslrDashboard client.
 It is primary writen for OpenWrt (TP-Link MR3040 or WR703N) but it will run on any Linux distribution including Raspbery Pi or pcDuino for example.
 
 
 **Multiple device connections was introduced in V0.2 and it can be used with DslrDashboard V0.30.24 and up.**
 
 After DslrDashboard has connect to ddserver, ddserver will send the connected USB cameras list.
 If there is only one USB camera connected then DslrDashboard will open that one.
 If there are more USB cameras connected DslrDashboard will display a dialog where you can select the desired camera.
 
 You can find installation image for MR3040 at http://dslrdashboard.info/downloads/
 or you can contact me if you have problem building your own image.
 
 **Added a simple device discovery**
 
## Compile DslrDashboard Server on OpenWrt
 
 Clone the *DslrDashboardServer* repository to the *package* directory on OpenWrt:
 
 	git clone git://github.com/hubaiz/DslrDashboardServer package/DslrDashboardServer
 
 Run then the following commands:
 
 	make menuconfig
 
 Under **Multimedia** select **ddserver**. Build OpenWrt or just a single package. The resulting package can be found in the *bin/[platform]/packages/ddserver_0.1-1.ipk* directory. If you selected * the package will be already installed in the OpenWrt image.
 Upon installation DDserver will add an init script so you can start/stop it from the web interface (System->Startup).


## Compile DslrDashboard Server on Debian based distributions, such as Ubuntu and Raspbian

```sh
sudo apt-get install build-essential cmake libusb-1.0-0-dev
mkdir build
cd build
cmake ..
sudo cmake install
sudo systemctl daemon-reload
sudo systemctl enable ddserver.service
sudo systemctl start ddserver.service
```

check the status whenever `sudo systemctl status ddserver.service`

## Compile DslrDashboard Server on other Linux distributions

install libusb, cmake, and any appropriate build essentials package, then build and install

```sh
mkdir build
cd build
cmake ..
sudo cmake install
sudo systemctl daemon-reload
sudo systemctl enable ddserver.service
sudo systemctl start ddserver.service
```
