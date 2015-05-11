include $(TOPDIR)/rules.mk

PKG_NAME:=whsnbg
PKG_VERSION:=1.00
PKG_RELEASE:=1

WHSNBG_STAGING_DIR = /home/openwrt/staging_dir
#WHSNBG_TARGET_DIR = target-mips_r2_uClibc-0.9.33.2
WHSNBG_TARGET_DIR = target-mips_34kc_uClibc-0.9.33.2
WHSNBG_INCLUDE_DIR = $(WHSNBG_STAGING_DIR)/$(WHSNBG_TARGET_DIR)/usr/include
WHSNBG_LIB_DIR = $(WHSNBG_STAGING_DIR)/$(WHSNBG_TARGET_DIR)/usr/lib
#PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR = $(BUILD_DIR)/$(PKG_NAME)
PKG_INCLPATH = -I. -I$(WHSNBG_INCLUDE_DIR)/mysql
PKG_LIBPATH = -L. -L$(WHSNBG_LIB_DIR) -L$(WHSNBG_LIB_DIR)/mysql
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

define Package/$(PKG_NAME)/description
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
	$(INSTALL_DIR) $(1)/bin $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/bin/
	$(INSTALL_DATA) ./files/etc/$(PKG_NAME).conf $(1)/etc/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
