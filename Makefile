#
# Copyright (C) 2013 Zoltan Hubai
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ddserver
PKG_VERSION:=0.1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/ddserver
  SECTION:=utils
  CATEGORY:=Multimedia
  DEPENDS:=+libusb-1.0
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
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/libusb-1.0 \
		-L$(STAGING_DIR)/lib -L$(STAGING_DIR)/usr/lib \
		-lusb-1.0 \
		-o $(PKG_BUILD_DIR)/ddserver $(PKG_BUILD_DIR)/DslrDashboardServer.cpp
endef

define Package/ddserver/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ddserver $(1)/usr/bin/
endef

$(eval $(call BuildPackage,ddserver))

