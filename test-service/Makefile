include $(TOPDIR)/rules.mk

PKG_NAME    := test-service
PKG_VERSION := 0.1.0
PKG_RELEASE := 1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/test-service
  SECTION  := utils
  CATEGORY := Utilities
  TITLE    := Test Service for HGW-Doctor validation
endef

define Package/test-service/description
  Simple dummy daemon to validate HGW-Doctor process monitoring and recovery.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	:
endef

define Package/test-service/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(TOPDIR)/feeds/test_service/test-service/files/test-service.sh \
		$(1)/usr/bin/test-service.sh
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(TOPDIR)/feeds/test_service/test-service/files/test-service.init \
		$(1)/etc/init.d/test-service
endef

$(eval $(call BuildPackage,test-service))
