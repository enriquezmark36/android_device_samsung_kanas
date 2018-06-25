# Copyright (C) 2014 The CyanogenMod Project
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

LOCAL_PATH := device/samsung/kanas

# Inherit from vendor tree
$(call inherit-product-if-exists, vendor/samsung/kanas/kanas-vendor.mk)

# Inherit from AOSP product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# The gps config appropriate for this device
# Already provided.
# $(call inherit-product, device/common/gps/gps_us_supl.mk)

# ???Languages???
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)


DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# This device is hdpi
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Languages
PRODUCT_PROPERTY_OVERRIDES += \
	ro.product.locale.language=en \
	ro.product.locale.region=GB

# Rootdir files
ROOTDIR_FILES := \
	$(LOCAL_PATH)/rootdir/init.rc \
	$(LOCAL_PATH)/rootdir/init.board.rc \
	$(LOCAL_PATH)/rootdir/init.sc8830.rc \
	$(LOCAL_PATH)/rootdir/init.sc8830.usb.rc \
	$(LOCAL_PATH)/rootdir/init.sc8830_ss.rc \
	$(LOCAL_PATH)/rootdir/init.kanas3g.rc \
	$(LOCAL_PATH)/rootdir/init.kanas3g_base.rc \
	$(LOCAL_PATH)/rootdir/init.wifi.rc \
	$(LOCAL_PATH)/rootdir/ueventd.sc8830.rc \
	$(LOCAL_PATH)/rootdir/fstab.sc8830 \

PRODUCT_COPY_FILES += \
	$(foreach f,$(ROOTDIR_FILES),$(f):root/$(notdir $(f)))

# Keylayouts
KEYLAYOUT_FILES := \
	$(LOCAL_PATH)/configs/keylayouts/ist30xx_ts_input.kl \
	$(LOCAL_PATH)/configs/keylayouts/sci-keypad.kl

PRODUCT_COPY_FILES += \
	$(foreach f,$(KEYLAYOUT_FILES),$(f):system/usr/keylayout/$(notdir $(f)))

# Misc packages
PRODUCT_PACKAGES += \
	com.android.future.usb.accessory

# Filesystem management tools
PRODUCT_PACKAGES += \
	setup_fs \
	e2fsck \
	f2fstat \
	fsck.f2fs \
	fibmap.f2fs \
	mkfs.f2fs

# Bluetooth config
BLUETOOTH_CONFIGS := \
	$(LOCAL_PATH)/configs/bluetooth/bt_vendor.conf

PRODUCT_COPY_FILES += \
	$(foreach f,$(BLUETOOTH_CONFIGS),$(f):system/etc/bluetooth/$(notdir $(f)))

# Media config
MEDIA_CONFIGS := \
	$(LOCAL_PATH)/configs/media/media_codecs.xml \
	$(LOCAL_PATH)/configs/media/media_profiles.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_video_le.xml \
	frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml

PRODUCT_COPY_FILES += \
	$(foreach f,$(MEDIA_CONFIGS),$(f):system/etc/$(notdir $(f)))

AUDIO_CONFIGS := \
	device/samsung/kanas/configs/audio/audio_policy.conf \
	device/samsung/kanas/configs/audio/audio_hw.xml \
	device/samsung/kanas/configs/audio/audio_para \
	device/samsung/kanas/configs/audio/codec_pga.xml \
	device/samsung/kanas/configs/audio/tiny_hw.xml

PRODUCT_COPY_FILES += \
	$(foreach f,$(AUDIO_CONFIGS),$(f):system/etc/$(notdir $(f))) \

# GPS
GPS_CONFIGS := \
	device/samsung/kanas/configs/gps/gps.xml \
	device/samsung/kanas/configs/gps/gps.conf \

PRODUCT_COPY_FILES += \
	$(foreach f,$(GPS_CONFIGS),$(f):system/etc/$(notdir $(f)))

# Nvitem
NVITEM_CONFIGS := \
	device/samsung/kanas/configs/nvitem/nvitem_td.cfg \
	device/samsung/kanas/configs/nvitem/nvitem_w.cfg

PRODUCT_COPY_FILES += \
	$(foreach f,$(NVITEM_CONFIGS),$(f):system/etc/$(notdir $(f)))

WIFI_CONFIGS := \
	device/samsung/kanas/configs/wifi/wpa_supplicant.conf \
	device/samsung/kanas/configs/wifi/wpa_supplicant_overlay.conf \
	device/samsung/kanas/configs/wifi/p2p_supplicant_overlay.conf

PRODUCT_COPY_FILES += \
	$(foreach f,$(WIFI_CONFIGS),$(f):system/etc/wifi/$(notdir $(f)))

# Permissions
PRODUCT_PACKAGES += platform.xml

PERMISSION_XML_FILES := \
	$(LOCAL_PATH)/configs/permissions/handheld_core_hardware.xml \
	$(LOCAL_PATH)/configs/permissions/android.hardware.camera.flash.xml \
	frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.location.xml \
	frameworks/native/data/etc/android.hardware.location.gps.xml \
	frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.xml \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.software.sip.voip.xml \
	frameworks/native/data/etc/android.software.sip.xml \
	frameworks/native/data/etc/android.software.midi.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.wifi.direct.xml

PRODUCT_COPY_FILES += \
	$(foreach f,$(PERMISSION_XML_FILES),$(f):system/etc/permissions/$(notdir $(f)))

# Scripts
SCRIPTS_FILES := \
	$(LOCAL_PATH)/scripts/set_freq.sh

PRODUCT_COPY_FILES += \
	$(foreach f,$(SCRIPTS_FILES),$(f):system/bin/$(notdir $(f)))

# Dalvik heap config
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
# $(call inherit-product, frameworks/native/build/phone-xxhdpi-2048-hwui-memory.mk)

# Override phone-hdpi-512-dalvik-heap to match value on stock
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.heapgrowthlimit=48m

# enable Google-specific location features,
# like NetworkLocationProvider and LocationCollector
PRODUCT_PROPERTY_OVERRIDES += \
	ro.com.google.locationfeatures=1 \
	ro.com.google.networklocation=1

# For userdebug builds
ADDITIONAL_DEFAULT_PROPERTIES += \
	ro.secure=0 \
	ro.adb.secure=0 \
	ro.debuggable=1 \
	persist.sys.root_access=1 \
	persist.service.adb.enable=1

# Bluetooth
PRODUCT_PACKAGES += \
	bluetooth.default \
	libbluetooth_jni

# HWC
PRODUCT_PACKAGES += \
	gralloc.sc8830 \
	hwcomposer.sc8830 \
	sprd_gsp.sc8830 \
	libmemoryheapion_sprd  \
	libion

# PRODUCT_PACKAGES += \
# 	libmemoryheapion  \
# 	libion_sprd

# Camera
PRODUCT_PACKAGES += \
	camera.sc8830 \
	camera2.sc8830

# Codecs
PRODUCT_PACKAGES += \
	libstagefrighthw \
	libstagefright_sprd_soft_mpeg4dec \
	libstagefright_sprd_soft_h264dec \
	libstagefright_sprd_mpeg4dec \
	libstagefright_sprd_mpeg4enc \
	libstagefright_sprd_h264dec \
	libstagefright_sprd_h264enc \
	libstagefright_sprd_vpxdec \
	libstagefright_sprd_aacdec \
	libstagefright_sprd_mp3dec \
	libomx_aacdec_sprd.so \
	libomx_avcdec_hw_sprd.so \
	libomx_avcdec_sw_sprd.so \
	libomx_avcenc_hw_sprd.so \
	libomx_m4vh263dec_hw_sprd.so \
	libomx_m4vh263dec_sw_sprd.so \
	libomx_m4vh263enc_hw_sprd.so \
	libomx_mp3dec_sprd.so \
	libomx_vpxdec_hw_sprd.so

# Lights
PRODUCT_PACKAGES += \
	lights.sc8830

# Power: no kernel support yet
# PRODUCT_PACKAGES += \
# 	power.sc8830

# Packages
PRODUCT_PACKAGES += \
	SamsungServiceMode\
	SamsungDoze \
	Gello \
	Snap

# Live Wallpapers
PRODUCT_PACKAGES += \
	LiveWallpapers \
	LiveWallpapersPicker \
	VisualizationWallpapers \
	librs_jni

# Audio
PRODUCT_PACKAGES += \
	libaudio-resampler \
	libtinyalsa \
	libatchannel \
	audio.r_submix.default \
	audio.primary.sc8830 \
	audio_policy.sc8830 \
	libatchannel_wrapper \
	audio.a2dp.default \
	audio.usb.default \

# Use prebuilt webviewchromium
# PRODUCT_PACKAGES += \
# 	webview \
# 	libwebviewchromium_loader.so \
# 	libwebviewchromium_plat_support.so

# Wifi
PRODUCT_PACKAGES += \
	libnetcmdiface \
	dhcpcd.conf \
	macloader \
	wpa_supplicant \
	hostapd

# Set default USB interface
# WARNING: When in userdebug/eng builds, do not append ",adb".
#          You may if it's on user. This goes to the initrd/ramdisk
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

# Device props
PRODUCT_PROPERTY_OVERRIDES += \
	keyguard.no_require_sim=true \
	ro.com.android.dataroaming=false \

# Compatitbility
PRODUCT_PACKAGES += \
	libstlport \
	libboringssl-compat \
	libril_shim \
	libgps_shim

# ART device props
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.dex2oat-Xms=8m \
	dalvik.vm.dex2oat-Xmx=256m \
	dalvik.vm.dex2oat-flags=--no-watch-dog \
	dalvik.vm.image-dex2oat-Xms=48m \
	dalvik.vm.image-dex2oat-Xmx=48m \
	dalvik.vm.dex2oat-filter=interpret-only \
	ro.sys.fw.dex2oat_thread_count=4 \
	dalvik.vm.image-dex2oat-filter=speed

# Camera config
PRODUCT_PROPERTY_OVERRIDES += \
	camera.disable_zsl_mode=1

# These are the hardware-specific settings that are stored in system properties.
# Note that the only such settings should be the ones that are too low-level to
# be reachable from resources or other mechanisms.
PRODUCT_PROPERTY_OVERRIDES += \
	ro.zygote.disable_gl_preload=true

$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Set those variables here to overwrite the inherited values.
PRODUCT_NAME := full_kanas
PRODUCT_DEVICE := kanas
PRODUCT_BRAND := samsung
PRODUCT_MANUFACTURER := samsung
PRODUCT_MODEL := SM-G355H
