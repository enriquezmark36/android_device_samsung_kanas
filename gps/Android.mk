LOCAL_PATH:= $(call my-dir)

###
### Wrapper for Marvell's HAL (adapted from Motorola's HAL Wrapper)
###

include $(CLEAR_VARS)

LOCAL_MODULE := gps.default


LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_STRIP_MODULE := false

LOCAL_SRC_FILES := wrapper.c

LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
