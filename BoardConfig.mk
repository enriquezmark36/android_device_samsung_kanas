# Copyright (C) 2014 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Inherit from the proprietary version
-include vendor/samsung/kanas/BoardConfigVendor.mk

# For some reason, su is not being installed with this on
# BLOCK_BASED_OTA :=false

# Since we are using someone else's code for this device
# And this device is a wierd mix of scx15 and sc8830 from
# their perspective, we'll need to maintain this simple modification
# to tell thier code to use the scx15 part in the sc8830 version
SOC_SCX35 := true

# Platform
TARGET_ARCH := arm
TARGET_BOARD_PLATFORM := sc8830
TARGET_BOARD_PLATFORM_GPU := mali-400 MP
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a7
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOOTLOADER_BOARD_NAME := SC7735S
BOARD_VENDOR := samsung

WITH_SU := true
TARGET_NO_RADIOIMAGE := true
TARGET_NO_BOOTLOADER := true
# Allow bootcharting
INIT_BOOTCHART := true
# CMHW
BOARD_HARDWARE_CLASS := device/samsung/kanas/cmhw/

BOARD_BOOTIMAGE_PARTITION_SIZE := 8388608
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 12582912
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1111490560
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2457862144
BOARD_CACHEIMAGE_PARTITION_SIZE := 209715200
BOARD_FLASH_BLOCK_SIZE := 131072
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_HAS_LARGE_FILESYSTEM := true

# RIL
BOARD_PROVIDES_RILD := true
BOARD_PROVIDES_LIBREFERENCE_RIL := true
BOARD_PROVIDES_LIBRIL := true
BOARD_RIL_CLASS := ../../../device/samsung/kanas/ril
COMMON_GLOBAL_CFLAGS += -DSEC_PRODUCT_FEATURE_RIL_CALL_DUALMODE_CDMAGSM
COMMON_GLOBAL_CFLAGS += -DDISABLE_ASHMEM_TRACKING

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/kanas/bluetooth
BOARD_BLUEDROID_VENDOR_CONF := device/samsung/kanas/bluetooth/libbt_vndcfg.txt
#USE_BLUETOOTH_BCM4343 := true

# Wifi
BOARD_WLAN_DEVICE := bcmdhd
BOARD_WLAN_DEVICE_REV := bcm4330
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
WIFI_DRIVER_FW_PATH_PARAM := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA := "/system/etc/wifi/bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_AP := "/system/etc/wifi/bcmdhd_apsta.bin"
WIFI_DRIVER_NVRAM_PATH_PARAM := "/sys/module/dhd/parameters/nvram_path"
WIFI_DRIVER_NVRAM_PATH := "/system/etc/wifi/nvram_net.txt"
WIFI_BAND := 802_11_ABG
BOARD_HAVE_SAMSUNG_WIFI := true

# Graphics
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
USE_OPENGL_RENDERER := true
BOARD_EGL_NEEDS_HANDLE_VALUE := true
HWUI_COMPILE_FOR_PERF := true
##
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
COMMON_GLOBAL_CFLAGS += -DFORCE_SCREENSHOT_CPU_PATH
##
### testing: negate when something bad happens
TARGET_REQUIRES_SYNCHRONOUS_SETSURFACE := false
###

# Hardware rendering
BOARD_EGL_CFG := device/samsung/kanas/configs/egl/egl.cfg
USE_SPRD_HWCOMPOSER := true
USE_OVERLAY_COMPOSER_GPU := true
COMMON_GLOBAL_CFLAGS += -DSC8830_HWC
##
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
##
##### Testing: negate when ....
USE_SPRD_DITHER := false
#####
######## Not found
# BOARD_EGL_WORKAROUND_BUG_10194508 := false
# BOARD_USE_MHEAP_SCREENSHOT := false
# DEVICE_FORCE_VIDEO_GO_OVERLAYCOMPOSER := false
########

# Enable WEBGL in WebKit
ENABLE_WEBGL := true

# Fix video autoscaling on old OMX decoders
# TARGET_OMX_LEGACY_RESCALING := true

TARGET_SYSTEM_PROP := device/samsung/kanas/system.prop
TARGET_USE_CUSTOM_LUN_FILE_PATH := /sys/class/android_usb/android0/f_mass_storage/lun/file

# Bootanimation
TARGET_BOOTANIMATION_PRELOAD := true
TARGET_BOOTANIMATION_MULTITHREAD_DECODE := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := true

# Resolution
TARGET_SCREEN_HEIGHT := 800
TARGET_SCREEN_WIDTH := 480

# Audio
BOARD_USES_TINYALSA_AUDIO := true
BOARD_USES_SS_VOIP := true
BOARD_USE_LIBATCHANNEL_WRAPPER := true
TARGET_TINY_ALSA_IGNORE_SILENCE_SIZE := true
######### Compile Error since acp tries to follow a dangling symlink
# TARGET_NEEDS_VBC_EQ_SYMLINK := true
######### Audio Legacy // DO NOT! JUST DON'T set to 1
# USE_LEGACY_AUDIO_POLICY := 0

# Media
COMMON_GLOBAL_CFLAGS += -DADD_LEGACY_ACQUIRE_BUFFER_SYMBOL
COMMON_GLOBAL_CFLAGS += -DBOARD_CANT_REALLOCATE_OMX_BUFFERS

# Board specific features
COMMON_GLOBAL_CFLAGS += -DUSE_LEGACY_BLOBS
COMMON_GLOBAL_CFLAGS += -DSPRD_HARDWARE
# To be solved by libcolorswitcher, so don't say true
# Unless you're still using the "depreciated" patchset
BOARD_USE_SAMSUNG_COLORFORMAT := true

# Unless you're using the patchset from lollipop
# This flag won't have any effect
BOARD_NEEDS_MEMORYHEAPION_SPRD := true

# Healthd
BOARD_HAL_STATIC_LIBRARIES := libhealthd.sc8830

# Camera
TARGET_BOARD_CAMERA_HAL_VERSION := HAL1.0
#android zsl capture
TARGET_BOARD_CAMERA_ANDROID_ZSL_MODE := false
#back camera rotation capture
TARGET_BOARD_BACK_CAMERA_ROTATION := false
#front camera rotation capture
TARGET_BOARD_FRONT_CAMERA_ROTATION := false
#rotation capture
TARGET_BOARD_CAMERA_ROTATION_CAPTURE := false
# select camera 2M,3M,5M,8M
CAMERA_SUPPORT_SIZE := 5M
#
TARGET_BOARD_NO_FRONT_SENSOR := false
#
TARGET_BOARD_CAMERA_FLASH_CTRL := true
#select camera zsl cap mode
TARGET_BOARD_CAMERA_CAPTURE_MODE := false
#face detect
TARGET_BOARD_CAMERA_FACE_DETECT := false
#
TARGET_BOARD_CAMERA_USE_IOMMU := true
TARGET_BOARD_CAMERA_DMA_COPY := true
#
TARGET_BOARD_BACK_CAMERA_INTERFACE := ccir
TARGET_BOARD_FRONT_CAMERA_INTERFACE := ccir
#select continuous auto focus
TARGET_BOARD_CAMERA_CAF := true
#
CONFIG_CAMERA_ISP := true
COMMON_GLOBAL_CFLAGS += -DCONFIG_CAMERA_ISP

# Kernel
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8
BOARD_KERNEL_PAGESIZE := 2048
TARGET_KERNEL_CONFIG := impasta_kanas_defconfig
TARGET_KERNEL_CONFIG := sandroid_kanas_defconfig
TARGET_KERNEL_SOURCE := kernel/samsung/kanas

# Init
TARGET_NR_SVC_SUPP_GIDS := 32
TARGET_PROVIDES_INIT_RC := true

# Assert
TARGET_OTA_ASSERT_DEVICE := kanas,kanas3g,kanas3gxx,kanas3gub,kanas3gubxx,kanas3gnfcxx,kanas3gnfc,SM-G355H,SM-G355HN,SM-G355M

BOARD_SEPOLICY_DIRS += device/samsung/kanas/sepolicy

# Use dmalloc() for such low memory devices like us
# On the second thought, dlmalloc causes more glitches than
# the memory it suppose to save; use jemalloc instead
MALLOC_IMPL := jemalloc
BOARD_USES_LEGACY_MMAP := true

# Maybe redundant; Compile mke2fs binary for creating ext4 FS
TARGET_USES_MKE2FS := true

# Let vold recognize exfat and ntfs partitions
TARGET_USES_EXFAT := true
TARGET_KERNEL_HAVE_NTFS := true

# Enable dex-preoptimization to speed up the first boot sequence
# Seems to be the cause of backtrace at boot
# WITH_DEXPREOPT := true
# WITH_DEXPREOPT_BOOT_IMG_ONLY := true
# WITH_DEXPREOPT_PIC := true
# WITH_DEXPREOPT_COMP := false

# Charger
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_NO_CHARGER_LED := true
BOARD_CHARGING_MODE_BOOTING_LPM := /sys/class/power_supply/battery/batt_lp_charging
CHARGING_ENABLED_PATH := /sys/class/power_supply/battery/batt_lp_charging
BACKLIGHT_PATH := /sys/class/backlight/panel/brightness

# GCC 4.9 causes problems. Whenever possible, use 4.8 instead.
# Dunno about clang though.
# USE_CLANG_PLATFORM_BUILD := true

# Recovery
BOARD_HAS_NO_REAL_SDCARD := true
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"

# TWRP
RECOVERY_GRAPHICS_USE_LINELENGTH := true
SP1_NAME := "internal_sd"
SP1_BACKUP_METHOD := files
SP1_MOUNTABLE := 1
TW_INTERNAL_STORAGE_PATH := "/data/media/0"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
TW_FLASH_FROM_STORAGE := true
TW_NO_REBOOT_BOOTLOADER := true
TW_CUSTOM_CPU_TEMP_PATH := "/sys/devices/platform/sec-thermistor/temperature"
TWHAVE_SELINUX := true
TARGET_RECOVERY_INITRC := device/samsung/kanas/etc/init.rc
TARGET_RECOVERY_FSTAB := device/samsung/kanas/rootdir/twrp.fstab
TW_HAS_DOWNLOAD_MODE := true
DEVICE_RESOLUTION := 480x800
BOARD_USE_CUSTOM_RECOVERY_FONT := \"roboto_10x18.h\"
