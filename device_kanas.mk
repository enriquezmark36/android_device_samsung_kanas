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

# Inherit from scx35-common device configuration
$(call inherit-product, device/samsung/scx35-common/common.mk)


# Overlay
DEVICE_PACKAGE_OVERLAYS += device/samsung/kanas/overlay

# Boot animation
TARGET_SCREEN_HEIGHT := 800
TARGET_SCREEN_WIDTH := 480

# Rootdir files
ROOTDIR_FILES := \
	$(LOCAL_PATH)/rootdir/init.sc8830.rc \
	$(LOCAL_PATH)/rootdir/init.sc8830.usb.rc \
	$(LOCAL_PATH)/rootdir/init.kanas3g_base.rc \
	$(LOCAL_PATH)/rootdir/ueventd.sc8830.rc \
	$(LOCAL_PATH)/rootdir/fstab.sc8830

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/rootdir/mediaserver.rc:system/etc/init/mediaserver.rc\
	$(LOCAL_PATH)/rootdir/rild.rc:system/etc/init/rild.rc

PRODUCT_COPY_FILES += \
	$(foreach f,$(ROOTDIR_FILES),$(f):root/$(notdir $(f)))

# Keylayouts
KEYLAYOUT_FILES := \
	device/samsung/kanas/keylayouts/ist30xx_ts_input.kl \
	device/samsung/kanas/keylayouts/sci-keypad.kl

PRODUCT_COPY_FILES += \
	$(foreach f,$(KEYLAYOUT_FILES),$(f):system/usr/keylayout/$(notdir $(f)))

# WiFi
$(call inherit-product, hardware/broadcom/wlan/bcmdhd/config/config-bcm.mk)

# Codecs
PRODUCT_PACKAGES += \
	libstagefright_sprd_soft_mpeg4dec \
	libstagefright_sprd_soft_h264dec

# Camera can only use HALv1
PRODUCT_PROPERTY_OVERRIDES += \
	media.stagefright.legacyencoder=true \
	media.stagefright.less-secure=true

# Sdcardfs
PRODUCT_PROPERTY_OVERRIDES += \
	ro.sys.sdcardfs=true

# Audio
PRODUCT_PACKAGES += \
	audio_policy.sc8830

# Some Lineageos Apps
PRODUCT_PACKAGES += \
       Snap

# Languages
PRODUCT_PROPERTY_OVERRIDES += \
	ro.product.locale.language=en \
	ro.product.locale.region=US

# Override phone-hdpi-512-dalvik-heap to match value on stock
PRODUCT_PROPERTY_OVERRIDES += \
	dalvik.vm.heapgrowthlimit=48m

# Modem
PRODUCT_PACKAGES += \
	modemd

# ART device props
PRODUCT_PROPERTY_OVERRIDES += \
	ro.kernel.android.checkjni=0 \
	dalvik.vm.checkjni=false \
	dalvik.vm.dex2oat-Xms=8m \
	dalvik.vm.dex2oat-Xmx=96m \
	dalvik.vm.dex2oat-filter=interpret-only \
	dalvik.vm.image-dex2oat-Xms=48m \
	dalvik.vm.image-dex2oat-Xmx=48m \
	dalvik.vm.image-dex2oat-filter=speed

# These are the hardware-specific settings that are stored in system properties.
# Note that the only such settings should be the ones that are too low-level to
# be reachable from resources or other mechanisms.
PRODUCT_PROPERTY_OVERRIDES += \
       ro.zygote.disable_gl_preload=true

# CONFIGS
# BLUETOOTH_CONFIGS := \
# 	device/samsung/kanas/configs/bluetooth/bt_vendor.conf

MEDIA_CONFIGS := \
	device/samsung/kanas/media/media_codecs.xml \
	device/samsung/kanas/media/media_profiles.xml

AUDIO_CONFIGS := \
	device/samsung/kanas/configs/audio/audio_para

# GPS_CONFIGS := \
# 	device/samsung/kanas/configs/gps/gps.xml

# WIFI_CONFIGS := \
# 	device/samsung/kanas/configs/wifi/wpa_supplicant.conf \
# 	device/samsung/kanas/configs/wifi/wpa_supplicant_overlay.conf \
# 	device/samsung/kanas/configs/wifi/p2p_supplicant_overlay.conf

PRODUCT_COPY_FILES += \
	$(foreach f,$(MEDIA_CONFIGS),$(f):system/etc/$(notdir $(f))) \
	$(foreach f,$(AUDIO_CONFIGS),$(f):system/etc/$(notdir $(f)))
# 	$(foreach f,$(GPS_CONFIGS),$(f):system/etc/$(notdir $(f))) \
# 	$(foreach f,$(WIFI_CONFIGS),$(f):system/etc/wifi/$(notdir $(f)))
# 	$(foreach f,$(BLUETOOTH_CONFIGS),$(f):system/etc/bluetooth/$(notdir $(f))) \
