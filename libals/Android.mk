LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := device/sony/huashan/include
LOCAL_SRC_FILES := als.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := libals
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
