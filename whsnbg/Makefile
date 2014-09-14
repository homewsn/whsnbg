include $(TOPDIR)/rules.mk

PKG_NAME:=whsnbg
PKG_VERSION:=1.20
PKG_RELEASE:=1

#PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)
PKG_INCLPATH = -I. -I/home/openwrt/openwrt/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include/mysql
PKG_LIBPATH = -L. -L/home/openwrt/openwrt/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib -L/home/openwrt/openwrt/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib/mysql
PKG_LIBS = -lpthread -lgcc_eh -lmysqlclient -lz -lm -laxtls

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=base
  CATEGORY:=Network
  DEFAULT:=n
  TITLE:=Wireless home sensor network broker-gateway
  URL:=http://www.homewsn.com/
  DEPENDS:=+libc +libaxtls +libmysqlclient +libpthread
endef

define Package/whsnbg/description
  MQTT broker and rules engine + MQTT-SN gateway for wireless home sensor networks
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
    CC="$(TARGET_CC)" \
    CFLAGS="$(TARGET_CFLAGS)" \
    LDFLAGS="$(TARGET_LDFLAGS)" \
    TARGET_INCLPATH="$(PKG_INCLPATH)" \
    TARGET_LIBPATH="$(PKG_LIBPATH)" \
    TARGET_LIBS="$(PKG_LIBS)" \
    $(MAKE) openwrt -C $(PKG_BUILD_DIR)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
