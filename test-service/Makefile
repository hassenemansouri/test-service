include $(TOPDIR)/rules.mk
PKG_NAME := test-service
PKG_VERSION := 0.1.0
PKG_RELEASE := 1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
include $(INCLUDE_DIR)/package.mk

define Package/test-service
  SECTION := utils
  CATEGORY := Utilities
  TITLE := Test Service for HGW-Doctor validation
  DEPENDS := +libamxd +libamxo +libamxc +libamxp +libamxb
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/src
	mkdir -p $(PKG_BUILD_DIR)/odl
	$(CP) $(TOPDIR)/feeds/test_service/src/main.c $(PKG_BUILD_DIR)/src/
	$(CP) $(TOPDIR)/feeds/test_service/Makefile $(PKG_BUILD_DIR)/Makefile
	$(CP) $(TOPDIR)/feeds/test_service/odl/test-service.odl $(PKG_BUILD_DIR)/odl/
	$(CP) $(TOPDIR)/feeds/test_service/odl/test-service_prefix.odl $(PKG_BUILD_DIR)/odl/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		STAGING_DIR="$(STAGING_DIR)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		PKG_BUILD_DIR="$(PKG_BUILD_DIR)"
endef

define Package/test-service/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/build/bin/test-service $(1)/usr/sbin/

	$(INSTALL_DIR) $(1)/etc/amx/test-service
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/odl/test-service.odl $(1)/etc/amx/test-service/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/odl/test-service_prefix.odl $(1)/etc/amx/test-service/

	$(INSTALL_DIR) $(1)/etc/amx/tr181-device/extensions
	$(INSTALL_DATA) $(TOPDIR)/feeds/test_service/test-service/files/01_device-test-service_mapping.odl \
		$(1)/etc/amx/tr181-device/extensions/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/feeds/test_service/test-service/files/test-service.init \
		$(1)/etc/init.d/test-service
endef

$(eval $(call BuildPackage,test-service))
