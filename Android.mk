LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := wificonf

LOCAL_CFLAGS += -Werror -Wno-unused-parameter

LOCAL_SRC_FILES:= \
    supplicant_config.cpp

LOCAL_SHARED_LIBRARIES := \
        libc \
        liblog \
        libbase \
        libcrypto \
        libcutils

LOCAL_INIT_RC := wificonf.rc

include $(BUILD_EXECUTABLE)
