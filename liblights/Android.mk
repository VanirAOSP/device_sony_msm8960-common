LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := device/sony/huashan/include
LOCAL_SRC_FILES := lights.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := lights.msm8960
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
include $(BUILD_SHARED_LIBRARY)
