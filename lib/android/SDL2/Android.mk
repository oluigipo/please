# https://developer.android.com/ndk/guides/android_mk.html
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)-SDL2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2main
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)-SDL2main.a
include $(PREBUILT_STATIC_LIBRARY)
