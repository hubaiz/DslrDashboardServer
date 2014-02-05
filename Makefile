#
# Copyright (C) 2013 Zoltan Hubai
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ddserver
PKG_VERSION:=0.2
PKG_RELEASE:=11

include $(INCLUDE_DIR)/package.mk

define Package/ddserver
  SECTION:=utils
  CATEGORY:=Multimedia
  DEPENDS:=+libusb-1.0 +libstdcpp
  TITLE:=Server for DSLR camera to use with DslrDashboard
  MAINTAINER:=Zoltan Hubai <hubaiz@gmail.com>
endef

define Package/ddserver/description
 Server for DslrDashboard for controling connected DSLR
 camera with USB
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CXX) $(TARGET_CXXFLAGS) -Wall -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/libusb-1.0 \
		-L$(STAGING_DIR)/lib -L$(STAGING_DIR)/usr/lib \
		-lusb-1.0 \
                -lstdc++ \
                -lpthread \
		-o $(PKG_BUILD_DIR)/ddserver $(PKG_BUILD_DIR)/main.cpp $(PKG_BUILD_DIR)/communicator.cpp
endef

define Package/ddserver/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ddserver $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ddserver.init $(1)/etc/init.d/ddserver
endef

define Package/ddserver/postinst
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/ddserver enable || true
endef

$(eval $(call BuildPackage,ddserver))

