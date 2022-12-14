# Pegatron Platform modules
FLINES_PLATFORM_MODULE_VERSION = 1.0.0
export FLINES_PLATFORM_MODULE_VERSION
ifeq ($(CONFIGURED_ARCH), amd64)
TARGET_ARCH = x86_64
else
TARGET_ARCH = $(CONFIGURED_ARCH)
endif
FLINES_DS610_48C8D_PLATFORM_MODULE = sonic-platform-flines-ds610-48c8d_$(FLINES_PLATFORM_MODULE_VERSION)_$(CONFIGURED_ARCH).deb
$(FLINES_DS610_48C8D_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-flines
$(FLINES_DS610_48C8D_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(FLINES_DS610_48C8D_PLATFORM_MODULE)_PLATFORM = $(TARGET_ARCH)-flines_ds610_48c8d-r0
SONIC_DPKG_DEBS += $(FLINES_DS610_48C8D_PLATFORM_MODULE)
