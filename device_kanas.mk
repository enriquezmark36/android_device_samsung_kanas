#
# Copyright (C) 2016 The Android Open Source Project
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Inherit from vendor
$(call inherit-product, vendor/samsung/kanas/kanas-vendor.mk)

# Add our overlay first as a matter of precedence
DEVICE_PACKAGE_OVERLAYS += device/samsung/kanas/overlay
DEVICE_PACKAGE_OVERLAYS += device/samsung/kanas/overlay-lineage

# Thanks to Google's common kernel source
# YACK should be capable of using sdcardfs on Android M and newer
PRODUCT_PROPERTY_OVERRIDES += \
	ro.config.media_vol_steps=25 \
	ro.sys.sdcardfs=true

# ART device props. overrides both go_defaults* and scx35-common tree
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.image-dex2oat-filter=speed \
	dalvik.vm.dex2oat-filter=space \
	pm.dexopt.first-boot=quicken \
	pm.dexopt.boot=verify \
	pm.dexopt.install=quicken \
	pm.dexopt.bg-dexopt=space-profile \
	pm.dexopt.ab-ota=quicken \
	pm.dexopt.inactive=verify \
	pm.dexopt.shared=space \
	dalvik.vm.image-dex2oat-threads=4 \
	dalvik.vm.dex2oat-threads=4 \
	dalvik.vm.dex2oat-swap=true \

# Values from stock
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.heapgrowthlimit=64m \
	dalvik.vm.heapsize=96m \

# Go low ram mode! It's not that bad anymore compared to
# earlier Android versions. We may lose splitscreen,
# multiuser, multiprocess webview and voice recognition.
# Why would you expect a ~700MB device do all of that?
PRODUCT_PROPERTY_OVERRIDES += \
	ro.config.low_ram=true

# Android Go-ish ART profiles
PRODUCT_COPY_FILES += \
    device/samsung/kanas/configs/art/preloaded-classes:system/etc/preloaded-classes \
    device/samsung/kanas/configs/art/compiled-classes:system/etc/compiled-classes \
    device/samsung/kanas/configs/art/dirty-image-objects:system/etc/dirty-image-objects

# Only allow adb as default usb function on.
# As it turns out, f_mtp does not work well with ffs
# (adb works through ffs, f_mtp implements mtp)
PRODUCT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=adb

# Inherit from scx35-common device configuration
$(call inherit-product, device/samsung/scx35-common/common.mk)

# Override Android Go profiles
PRODUCT_DEX_PREOPT_BOOT_IMAGE_PROFILE_LOCATION := device/samsung/kanas/configs/art/boot-image-profile.txt

# Boot animation
TARGET_SCREEN_HEIGHT := 800
TARGET_SCREEN_WIDTH := 480

# Rootdir files
PRODUCT_PACKAGES += \
	fstab.sc8830

# Keylayouts
KEYLAYOUT_FILES := \
	device/samsung/kanas/keylayouts/ist30xx_ts_input.kl \
	device/samsung/kanas/keylayouts/sci-keypad.kl

PRODUCT_COPY_FILES += \
	$(foreach f,$(KEYLAYOUT_FILES),$(f):system/usr/keylayout/$(notdir $(f)))

# Codecs
PRODUCT_PACKAGES += \
	libstagefright_sprd_soft_mpeg4dec \
	libstagefright_sprd_soft_h264dec

# Some Lineageos Apps
PRODUCT_PACKAGES += \
       Snap

# Sensors
PRODUCT_PACKAGES += \
	sensors.sc8830

# Graphics
PRODUCT_PACKAGES += \
	sprd_gsp.sc8830 \
	hwcomposer.sc8830

# MDNIE
PRODUCT_PACKAGES += \
	AdvancedDisplay

# Memtrack
PRODUCT_PACKAGES += \
	memtrack.sc8830

# GPS wrapper, wraps the GPS HAL
PRODUCT_PACKAGES += \
	gps.default

# Cell Radio
PRODUCT_PACKAGES += \
	refnotify \
	nvitemd \
	rild_scx15.rc

# Camera
PRODUCT_PACKAGES += \
	camera.sc8830 \
	camera2.sc8830

# Audio
PRODUCT_PACKAGES += \
	audio_vbc_eq

# Explicitly include mkf2fsuserimg.sh
PRODUCT_PACKAGES += \
	mkf2fsuserimg.sh \
	badblocks \

# Extra shims
PRODUCT_PACKAGES += \
	libengpc_shim \
	libvsp_shim \
	liblpm_shim \

# wpa_supplicant.conf
# This is where the wpa_supplicant.conf went from WIFI_CONFIGS.
PRODUCT_PACKAGES += \
	wpa_supplicant.conf

# HIDL. (hide-el)
# Build extra services not included in scx35-common
include device/samsung/kanas/hidl.mk

# Vendor security patch level -- G355HXXS0AQD1
PRODUCT_PROPERTY_OVERRIDES += \
    ro.lineage.build.vendor_security_patch=2017-04-07

PRODUCT_DIST_BOOT_AND_SYSTEM_JARS := true
PRODUCT_SYSTEM_SERVER_APPS := \
    SystemUI \

PRODUCT_DEXPREOPT_SPEED_APPS += \
    SystemUI \
    TrebuchetQuickStep

# AAPT
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi mdpi

# Device and blobs are from KitKat release
PRODUCT_SHIPPING_API_LEVEL := 19

# Camcorder profiles and media encoder limits, the configuration file.
MEDIA_CONFIGS := \
	device/samsung/kanas/configs/media/media_profiles_V1_0.xml

# Additional Features/Services to enable within system_server
PERMISSIONS_XML_FILES := \
	frameworks/native/data/etc/android.software.picture_in_picture.xml \

PRODUCT_COPY_FILES += \
	$(foreach f,$(MEDIA_CONFIGS),$(f):system/vendor/etc/$(notdir $(f))) \
	$(foreach f,$(PERMISSIONS_XML_FILES),$(f):system/vendor/etc/permissions/$(notdir $(f))) \

