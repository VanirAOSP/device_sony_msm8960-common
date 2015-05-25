LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ifneq ($(strip $(TARGET_DISPLAY_GAMMA_DISABLED)),true)
    LOCAL_CFLAGS += -DENABLE_GAMMA_CORRECTION
endif
LOCAL_SRC_FILES := lights.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := lights.msm8960
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := als.c
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE := libals
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
