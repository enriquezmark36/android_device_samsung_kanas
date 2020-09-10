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

# HWUI prop overrides
PRODUCT_PROPERTY_OVERRIDES += \
	ro.hwui.texture_cache_size=9 \
	ro.hwui.layer_cache_size=6 \
	ro.hwui.path_cache_size=1.5 \
	ro.hwui.gradient_cache_size=0.5 \
	ro.hwui.drop_shadow_cache_size=1 \
	ro.hwui.r_buffer_cache_size=1 \

# Inherit from scx35-common device configuration
$(call inherit-product, device/samsung/scx35-common/common.mk)

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
	nvitemd

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

# HIDL. (hide-el)
# Build extra services not included in scx35-common
include device/samsung/kanas/hidl.mk

# Prebuilt targets overrides:
# These files are declared as prebuilt targets in some Android.mk files
# but need some device specific modifications.
# Creating a new target for these files would result in an error so we do this instead.
# This would produce some warning about targets being overridden.
MEDIA_CONFIGS := \
	device/samsung/kanas/configs/media/media_profiles_V1_0.xml

AUDIO_CONFIGS := \
	device/samsung/kanas/configs/audio/audio_hw.xml \
	device/samsung/kanas/configs/audio/audio_para \
	device/samsung/kanas/configs/audio/audio_policy.conf\
	device/samsung/kanas/configs/audio/codec_pga.xml \
	device/samsung/kanas/configs/audio/tiny_hw.xml

WIFI_CONFIGS := \
	device/samsung/kanas/configs/wifi/p2p_supplicant_overlay.conf \
	device/samsung/kanas/configs/wifi/wpa_supplicant_overlay.conf \
	device/samsung/kanas/configs/wifi/wpa_supplicant.conf

RAMDISK_FILES := \
	device/samsung/kanas/rootdir/ueventd.sc8830.rc \
	device/samsung/kanas/rootdir/init.board.rc \
	device/samsung/kanas/rootdir/init.sc8830.rc

INIT_FILES := \
	device/samsung/kanas/system/etc/init/rild_scx15.rc \
	device/samsung/kanas/system/etc/init/kill_phone.rc \
	device/samsung/kanas/system/etc/init/refnotify.rc \
	device/samsung/kanas/system/etc/init/wpa_supplicant.rc

PRODUCT_COPY_FILES += \
	$(foreach f,$(MEDIA_CONFIGS),$(f):system/vendor/etc/$(notdir $(f))) \
	$(foreach f,$(AUDIO_CONFIGS),$(f):system/vendor/etc/$(notdir $(f))) \
	$(foreach f,$(WIFI_CONFIGS),$(f):system/vendor/etc/wifi/$(notdir $(f))) \
	$(foreach f,$(RAMDISK_FILES),$(f):root/$(notdir $(f))) \
	$(foreach f,$(INIT_FILES),$(f):system/vendor/etc/init/$(notdir $(f))) \

