include $(TOPDIR)/rules.mk

PKG_NAME:=demo
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/demo_server
    SECTION:=utils
    CATEGORY:=Utilities
    TITLE:=socket IPC demo server
endef

define Package/demo_client
    SECTION:=utils
    CATEGORY:=Utilities
    TITLE:=socket IPC demo client
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -R ./src/* $(PKG_BUILD_DIR)
endef

define Package/demo_server/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/demo_server $(1)/usr/sbin
endef

define Package/demo_client/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/demo_client $(1)/usr/sbin
endef

$(eval $(call BuildPackage,demo_server))
$(eval $(call BuildPackage,demo_client))
