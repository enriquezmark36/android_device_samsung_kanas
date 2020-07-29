DEVICE_TREE := device/samsung/kanas

CM_PLATFORM_SDK_VERSION := 7	# Required for libf2fs.so
override TARGET_OUT_VENDOR_SHARED_LIBRARIES = $(TARGET_OUT_SHARED_LIBRARIES)

# Platform
TARGET_ARCH := arm
TARGET_BOARD_PLATFORM := sc8830
TARGET_BOARD_PLATFORM_GPU := mali-400 MP
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a7
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_BOOTLOADER_BOARD_NAME := SC7735S
TARGET_CPU_SMP := true
BOARD_VENDOR := samsung

BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8 androidboot.selinux=permissive androidboot.hardware=sc8830
BOARD_KERNEL_PAGESIZE := 2048
TARGET_PREBUILT_KERNEL := device/samsung/kanas/kernel
#TARGET_KERNEL_CONFIG := impasta_kanas_cm14_1_defconfig
#TARGET_KERNEL_SOURCE := kernel/samsung/kanas

# fix this up by examining /proc/mtd on a running device
BOARD_BOOTIMAGE_PARTITION_SIZE := 15728640
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 15728640
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1195376640
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2415919104
BOARD_CACHEIMAGE_PARTITION_SIZE := 125829120
BOARD_FLASH_BLOCK_SIZE := 4096
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
BOARD_HAS_LARGE_FILESYSTEM := true

# TWRP specific build flags
TW_THEME := portrait_mdpi
TW_EXTRA_LANGUAGES := true
TW_CUSTOM_CPU_TEMP_PATH := "/sys/devices/platform/sec-thermistor/temperature"
TW_BRIGHTNESS_PATH := "/sys/devices/platform/panel/backlight/panel/brightness"
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun/file"
TW_INCLUDE_NTFS_3G := true
TW_EXCLUDE_SUPERSU := true
RECOVERY_SDCARD_ON_DATA := true
TARGET_RECOVERY_PIXEL_FORMAT := "RGBX_8888"
TW_HAS_DOWNLOAD_MODE := true

TW_INCLUDE_CRYPTO := true

# Backlight kernel driver code specifies that the default is at 148
TW_MAX_BRIGHTNESS := 255
TW_DEFAULT_BRIGHTNESS := 148

# On stock kernels, the vibration will often get stuck.
# On some custom kernels, this problem has been fixed.
# The kernel that comes with this has this problem fixed.
# So if ever, you're using an unpatched kernel
# Uncomment this out.
# TW_NO_HAPTICS := true

# Do not allow logcat to work, or you may not be able to
# unmount /system when it's mounted (even if it's on RO mode)
TARGET_USES_LOGD := false
TWRP_INCLUDE_LOGCAT := false

# Rely on the kernel driver instead of the FUSE implementation
TW_NO_EXFAT_FUSE := true

# Use Busybox instead of toolbox/toybox
# Busybox has a wider set of tools compared to the latter
TW_USE_TOOLBOX := false

# Rebooting to bootloader does not make sense to this device
# We can have reboot to download mode but not the bootloader.
TW_NO_REBOOT_BOOTLOADER := true
