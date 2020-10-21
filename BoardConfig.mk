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

# Just in case this is not yet defined,
# May fix those using TARGET_COPY_OUT_VENDOR to copy their files
# not actually copying their files.
TARGET_COPY_OUT_VENDOR := system/vendor

# Inherit from SCX35 common configs
-include device/samsung/scx35-common/BoardConfigCommon.mk

# SCX35-common tree overrides
WITH_DEXPREOPT_BOOT_IMG_AND_SYSTEM_SERVER_ONLY := false
PRODUCT_DEX_PREOPT_BOOT_FLAGS := $(filter-out --compiler-filter=speed,$(PRODUCT_DEX_PREOPT_BOOT_FLAGS))
PRODUCT_SYSTEM_SERVER_COMPILER_FILTER := speed-profile
WITH_DEX_PREOPT_GENERATE_PROFILE :=
PRODUCT_SYSTEM_SERVER_APPS := \
    SystemUI \

PRODUCT_DEXPREOPT_SPEED_APPS += \
    SystemUI \
    TrebuchetQuickStep

# Inherit from the proprietary version
-include vendor/samsung/kanas/BoardConfigVendor.mk

# Platform
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOOTLOADER_BOARD_NAME := SC7735S
TARGET_BOARD_PLATFORM_GPU := mali-400 MP

# Some magic
SOC_SCX35 := true

# Enable privacy guard's su
WITH_SU := true

# Img configuration
BOARD_BOOTIMAGE_PARTITION_SIZE := 15728640
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 15728640
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1195376640
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2415919104
BOARD_CACHEIMAGE_PARTITION_SIZE := 125829120
BOARD_FLASH_BLOCK_SIZE := 4096
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USES_MKE2FS := true

# Wifi
# CONFIG_NL80211_TESTMODE is required in the kernel config
# Also, take note of the "/vendor/..." as it is specified in the config
#
# We should also have this line below but
# bcmdhd_p2p.bin and bcmdhd_sta.bin are copies of each other
# WIFI_DRIVER_FW_PATH_P2P := "/vendor/etc/wifi/bcmdhd_p2p.bin"
BOARD_WLAN_DEVICE := bcmdhd
BOARD_WLAN_DEVICE_REV := bcm4330
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_bcmdhd
WIFI_DRIVER_FW_PATH_PARAM := "/sys/module/dhd/parameters/firmware_path"
WIFI_DRIVER_FW_PATH_STA := "/vendor/etc/wifi/bcmdhd_sta.bin"
WIFI_DRIVER_FW_PATH_AP := "/vendor/etc/wifi/bcmdhd_apsta.bin"
WIFI_DRIVER_NVRAM_PATH_PARAM := "/sys/module/dhd/parameters/nvram_path"
WIFI_DRIVER_NVRAM_PATH := "/vendor/etc/wifi/nvram_net.txt"
WIFI_BAND := 802_11_ABG
BOARD_HAVE_SAMSUNG_WIFI := true

# Graphics
TARGET_USE_3_FRAMEBUFFER := true
USE_SPRD_DITHER := false
NUM_FRAMEBUFFER_SURFACE_BUFFERS := 3
SF_START_GRAPHICS_ALLOCATOR_SERVICE := true
# Overlay Composer uses EGL but for some reason
# the ANativeWindow being passed via hwbinder is an invalid
# EGLNativeWindowType causing failures (black screens)
# everytime OC is used to layout the layers.
USE_OVERLAY_COMPOSER_GPU := false
# Enables workarounds within Gralloc that allows it to work under HIDL
# This is a must for compiling on Android O and above without the libui patch
TARGET_USES_HIDL_WORKAROUNDS := true
# Additional Gralloc 0 flags
# Gralloc2 will probably flip out when it gets some invalid Gralloc1 flags
# This will tell it not to do that.
TARGET_ADDITIONAL_GRALLOC_10_USAGE_BITS := 0x07001400

##############################
## Experimental Graphics flags
##############################
# GSP needs IOMMU or layers must be in physically contiguous memory.
# We don't have IOMMU, but we could do the latter by forcing layers
# how/where they are allocated. This is what this flags does.
# If conditions are met, GSP will do layer blending rather than the GPU.
TARGET_FORCE_HWC_CONTIG := true
# Allow GSP blend multiple OSD layers up to n layers.
# The scx15 SPRDHWC code only allows only 1 layer processing or
# 2 layer processing (IFF there is exactly 1 video and 1 OSD layer)
# The sc8830 SPRDHWC can blend multiple layers but cannot simply be
# backported to the scx15 one so I had to write it as an extension.
GSP_MAX_OSD_LAYERS := 5
# Forces Gralloc to allocate GRALLOC_USAGE_OVERLAY_BUFFER on ION_HEAP_ID_MASK_MM
# All other fallback mechanisms are implemented in-kernel.
#
# Rationale:
# TARGET_FORCE_HWC_CONTIG will put pressure on the CMA making it likely
# that CMA will fail to allocate even in absence of memory pressure.
# CMA and vmalloc works under few msec, gen_pool_alloc under some hundred usecs.
# SurfaceFlinger, when TARGET_FORCE_HWC_CONTIG is true, often allocates using gralloc.
TARGET_ION_OVERLAY_IS_CARVEOUT := true
# This is a custom flag to gralloc that forces define or forces unset the
# FBIOGET_DMABUF IOCTL. This would allow MALI to directly write onto
# the framebuffer avoiding a copy from its internal buffers every frame.
# That's the theory, though every libMali.so is different for each device
# and that our Mali is already using the mmaped address of the FB.
# This requires the FB_DMABUF kernel patchset applied.
# The main benefit here is that we will never have an invalid fd in a
# gralloc handle, not much about the performance.
TARGET_HAS_FB_DMABUF := true
# This is an experimental flag that allows gralloc to recreate handle
# from the framebuffer for the HWC. These handles can then be used by GSP
# to directly write to the framebuffer and bypassing DISPC's overlay feature.
# FBIOGET_DMABUF IOCTL must be defined and supported by the kernel.
TARGET_GSP_ONTO_FB := true

# AAPT
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := hdpi
PRODUCT_AAPT_PREBUILT_DPI := hdpi mdpi

# Audio
# BOARD_USE_LIBATCHANNEL_WRAPPER := true
# BOARD_USES_SS_VOIP := true
TARGET_NEEDS_VBC_EQ_SYMLINK := true

# HIDL
# This version binderizes more components than the scx35-common
# Comment out this line and the one in device.mk if you don't want that.
DEVICE_MANIFEST_FILE := device/samsung/kanas/configs/manifest.xml

# Bootanimation
TARGET_BOOTANIMATION_HALF_RES := true

# Bluetooth
BOARD_CUSTOM_BT_CONFIG := device/samsung/kanas/bluetooth/libbt_vndcfg.txt
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/samsung/kanas/bluetooth
## Common Overrides
USE_BLUETOOTH_BCM4343 :=

# Kernel
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8 androidboot.selinux=permissive androidboot.hardware=sc8830
BOARD_KERNEL_PAGESIZE := 2048
TARGET_KERNEL_CONFIG := impasta_kanas_lineage_16_defconfig
TARGET_KERNEL_SOURCE := kernel/samsung/kanas
## Common overrides
BOARD_KERNEL_SEPARATED_DT :=
BOARD_CUSTOM_BOOTIMG_MK :=
BOARD_MKBOOTIMG_ARGS :=
BOARD_RAMDISK_OFFSET :=
BOARD_KERNEL_TAGS_OFFSET :=

# Init
## Common overrides
TARGET_INIT_VENDOR_LIB :=
TARGET_UNIFIED_DEVICE :=

# Assert
TARGET_OTA_ASSERT_DEVICE := kanas,kanas3g,kanas3gxx,kanas3gub,kanas3gnfcxx,kanas3gnfc,SM-G355H,SM-G355HN,SM-G355M

# Low memory config
MALLOC_SVELTE := true

# Tell vold that we have a kernel based impl of exfat
TARGET_EXFAT_DRIVER := exfat

# Device and blobs are from KitKat release
PRODUCT_SHIPPING_API_LEVEL := 19

# 64bit Binder
TARGET_USES_64_BIT_BINDER := true

# Camera
TARGET_BOARD_CAMERA_HAL_VERSION := HAL1.0
TARGET_BOARD_CAMERA_ANDROID_ZSL_MODE := false
TARGET_BOARD_BACK_CAMERA_ROTATION := false
TARGET_BOARD_FRONT_CAMERA_ROTATION := false
TARGET_BOARD_CAMERA_ROTATION_CAPTURE := false
CAMERA_SUPPORT_SIZE := 5M
TARGET_BOARD_NO_FRONT_SENSOR := false
TARGET_BOARD_CAMERA_FLASH_CTRL := false
TARGET_BOARD_CAMERA_CAPTURE_MODE := false
TARGET_BOARD_CAMERA_FACE_DETECT := false
TARGET_BOARD_BACK_CAMERA_INTERFACE := mipi
TARGET_BOARD_FRONT_CAMERA_INTERFACE := ccir
TARGET_BOARD_CAMERA_CAF := false
TARGET_BOARD_CAMERA_IOCTL_HAS_SET_FLASH := true
TARGET_BOARD_CAMERA_IOCTL_EXTRA_PARAMS := true
TARGET_BOARD_CAMERA_IOCTL_HAS_POWER_ONOFF := true
TARGET_BOARD_CAMERA_DMA_COPY := false
TARGET_CAMERA_OPEN_SOURCE := true
TARGET_USES_MEDIA_EXTENSIONS := true
# When true, the libcamera HAL will use SPRD's version of easyHDR
# When false, the libcamera HAL will use Morpho INC's easyHDR
# Both have hand written ARM Assembly but Morpho inc's easyHDR
# also have tone mapping, contrast adjustments and does not
# rely on layer blending unlike the SPRD's of easyHDR
# If unsure, say false.
TARGET_BOARD_CAMERA_HDR_CAPTURE := false

# For the camera stock lib, if ever someone wants to try it.
# The opensource version have reached feature parity.
TARGET_LD_SHIM_LIBS += /system/vendor/lib/hw/camera.sc8830.so|libmemoryheapion.so

# LineageOS HAL1.0 compatibility
TARGET_NEEDS_LEGACY_CAMERA_HAL1_DYN_NATIVE_HANDLE := true
# It seems that cameraserver and even the binderized passthrough HAL (HIDL)
# works without merging cameraserver and mediaserver.
# TARGET_HAS_LEGACY_CAMERA_HAL1 := true

# Use our profiles instead of Google's
PGO_ADDITIONAL_PROFILE_DIRS := device/samsung/kanas/pgo-profiles

# Sensors
TARGET_USES_SENSORS_WRAPPER := false

# Light and LEDs
TARGET_HAS_BACKLIT_KEYS := false

# SELinux policy -- common for most SPRD devices
BOARD_SEPOLICY_DIRS += device/samsung/kanas/sepolicy

# Extra shims
TARGET_LD_SHIM_LIBS += \
       /system/vendor/bin/engpc|libengpc_shim.so \

# VSP shim is only applied to those that actually use the vsp ioctls
# Do not apply it to the *_sw_sprd.so versions
TARGET_LD_SHIM_LIBS += \
       /system/vendor/lib/libomx_avcenc_hw_sprd.so|libvsp_shim.so \
       /system/vendor/lib/libomx_avcdec_hw_sprd.so|libvsp_shim.so \
       /system/vendor/lib/libomx_m4vh263enc_hw_sprd.so|libvsp_shim.so \
       /system/vendor/lib/libomx_m4vh263dec_hw_sprd.so|libvsp_shim.so \

# Shims to the missing earlysuspend support in libsuspend
# Use this shim if we're NOT USING the patch that reverts
# the removal of the said support
TARGET_LD_SHIM_LIBS += \
       /system/vendor/bin/lpm|liblpm_shim.so \
       /sbin/charger|liblpm_shim.so \

# Override PowerHal
TARGET_POWERHAL_VARIANT := kanas
SCX35_COMMON_POWERHAL_OVERRIDE := true

# Some of the proprietary blobs we need have text relocations
# This inlcudes all of our HW accelerated video encoders and decoders.
# the fantastic HDR library by Morpho Inc, and the GPS hall blob.
TARGET_PROCESS_SDK_VERSION_OVERRIDE += \
        /system/vendor/bin/mediaserver=19 \
        /system/vendor/bin/hw/android.hardware.camera.provider@2.4-service=19 \
        /system/vendor/bin/hw/android.hardware.media.omx@1.0-service=19 \

# Disable Charger since we would like having the lpm binary do it for us
BOARD_CHARGER_NO_UI := true

# Recovery
BOARD_HAS_DOWNLOAD_MODE := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
TARGET_RECOVERY_FSTAB := device/samsung/kanas/rootdir/fstab.sc8830
