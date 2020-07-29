# Release name
PRODUCT_RELEASE_NAME := kanas

# Inherit from the common Open Source product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)

# Inherit from our custom product configuration
$(call inherit-product, vendor/omni/config/common.mk)

# Time Zone data for Recovery
PRODUCT_COPY_FILES += \
    bionic/libc/zoneinfo/tzdata:recovery/root/system/usr/share/zoneinfo/tzdata

# Some additional tools
TWRP_REQUIRED_MODULES += \
	badblocks

RELINK_SOURCE_FILES += $(TARGET_OUT_EXECUTABLES)/badblocks

## Device identifier. This must come after all inclusions
PRODUCT_NAME := omni_kanas
PRODUCT_DEVICE := kanas
PRODUCT_MANUFACTURER := samsung
PRODUCT_MODEL := SM-G355H
