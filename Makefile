#
# Copyright (C) 2013 Zoltan Hubai
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ddserver
PKG_VERSION:=0.2
PKG_RELEASE:=12

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ddserver
  SECTION:=utils
  CATEGORY:=Multimedia
  DEPENDS:=+libusb-1.0 +uclibcxx
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
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LIBS="-nodefaultlibs -lgcc -lc -lusb-1.0 -lpthread -luClibc++" \
		LDFLAGS="$(EXTRA_LDFLAGS)" \
		CXXFLAGS="$(TARGET_CFLAGS) $(EXTRA_CPPFLAGS)" \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS="$(TARGET_CROSS)" \
		ARCH="$(ARCH)" \
		$(1);
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
