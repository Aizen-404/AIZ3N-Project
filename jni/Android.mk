
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfrida-gumjs
LOCAL_SRC_FILES := third_party/frida-gumjs/libfrida-gumjs.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/third_party/frida-gumjs
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := aizen

LOCAL_CFLAGS := -Wno-error=format-security \
                -fvisibility=hidden \
                -ffunction-sections \
                -fdata-sections \
                -w

LOCAL_CPPFLAGS := -Wno-error=format-security \
                  -fvisibility=hidden \
                  -ffunction-sections \
                  -fdata-sections \
                  -fexceptions \
                  -std=c++17 \
                  -fno-rtti \
                  -fpermissive \
                  -w

LOCAL_LDFLAGS := -Wl,--gc-sections,--strip-all

LOCAL_LDLIBS := -llog \
                -landroid \
                -lEGL \
                -lGLESv3 \
                -lGLESv2 \
                -ldl \
                -lm \
                -pthread

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/ImGui \
    $(LOCAL_PATH)/ImGui/backends \
    $(LOCAL_PATH)/third_party/frida-gumjs \
    $(LOCAL_PATH)/build

LOCAL_SRC_FILES := \
    Main.cpp \
    GumRuntime.cpp \
    ScriptBridge.cpp \
    Includes/SM.cpp \
    ImGui/imgui.cpp \
    ImGui/imgui_draw.cpp \
    ImGui/imgui_tables.cpp \
    ImGui/imgui_widgets.cpp \
    ImGui/imgui_stdlib.cpp \
    ImGui/backends/imgui_impl_opengl3.cpp \
    ImGui/backends/imgui_impl_android.cpp

LOCAL_STATIC_LIBRARIES := \
    libfrida-gumjs

include $(BUILD_SHARED_LIBRARY)
