
#include "EGL/egl.h"
#include "GLES3/gl3.h"
#include <android/input.h>
#include <android/native_window.h>
#include <pthread.h>
#include <cstdio>
#include <atomic>
#include <chrono>
#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "ImGui/misc/fonts/Roboto-Regular.h"
#include "Includes/obfuscate.h"
#include "Includes/Macros.h"
#include "Includes/SM.h"
#include "Includes/dex_data.h"
#include "Includes/JNIStuff.h"
#include "Il2cpp/il2cpp-class.h"
#include "Il2cpp/Il2cpp.h"
#include "ScriptBridge.h"
#include "frida-gumjs.h"

static bool FlyActive = false;
static std::atomic<bool> AgentReady(false);
static char ToastMessage[96] = "";
static std::chrono::steady_clock::time_point ToastTime;
static MethodInfo *TouchCountMethod = nullptr;
static MethodInfo *GetTouchMethodInfo = nullptr;

static GumInterceptor *Interceptor = nullptr;
static GumInvocationListener *SwapListener = nullptr;

static void ShowToast(const char *message) {
    snprintf(ToastMessage, sizeof(ToastMessage), "%s", message);
    ToastTime = std::chrono::steady_clock::now();
}

static void DrawToast() {
    if (ToastMessage[0] == '\0') return;

    auto elapsed = std::chrono::steady_clock::now() - ToastTime;

    if (elapsed >= std::chrono::milliseconds(1800)) {
        ToastMessage[0] = '\0';
        return;
    }

    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, 55.0f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.0f)
    );

    ImGui::SetNextWindowBgAlpha(0.94f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 9.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(17.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.055f, 0.085f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.85f, 1.0f, 0.85f));

    ImGui::Begin(
        "##AIZ3N_TOAST",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav
    );

    ImGui::TextColored(
        ImVec4(0.0f, 1.0f, 0.85f, 1.0f),
        "%s",
        ToastMessage
    );

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

static void BeginDraw() {
    ImGui::SetNextWindowSize(ImVec2(340.0f, 205.0f), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 13.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 12.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.018f, 0.025f, 0.045f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.78f, 1.0f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.025f, 0.035f, 0.065f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.025f, 0.09f, 0.15f, 1.0f));

    if (ImGui::Begin("AIZ3N | VIP FLY", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.85f, 1.0f, 1.0f));
        ImGui::TextUnformatted("A I Z 3 N");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextDisabled("| PREMIUM CONTROL");
        ImGui::Separator();

        const float buttonWidth = (ImGui::GetContentRegionAvail().x - 10.0f) / 2.0f;

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            FlyActive ? ImVec4(0.0f, 0.42f, 0.38f, 1.0f) : ImVec4(0.0f, 0.32f, 0.42f, 1.0f)
        );
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.58f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.25f, 0.35f, 1.0f));

        if (ImGui::Button("FLY", ImVec2(buttonWidth, 52.0f))) {
            if (AgentReady.load() && ScriptBridge_SendCommand(
                "{\"type\":\"command\",\"payload\":{\"action\":\"enable\"}}"
            )) {
                FlyActive = true;
                ShowToast("Fly applied successfully");
            } else {
                ShowToast("Agent not ready");
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.25f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.38f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.2f, 0.05f, 1.0f));

        if (ImGui::Button("RESET", ImVec2(buttonWidth, 52.0f))) {
            if (AgentReady.load() && ScriptBridge_SendCommand(
                "{\"type\":\"command\",\"payload\":{\"action\":\"set_gravity\",\"value\":1.0}}"
            )) {
                FlyActive = false;
                ShowToast("Reset successfully");
            } else {
                ShowToast("Agent not ready");
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::Separator();
        ImGui::TextDisabled("AIZ3N NATIVE | FRIDA GUMJS");
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(5);
}

static void OnSwapEnter(GumInvocationContext *context, gpointer user_data) {
    (void) context;
    (void) user_data;
}

static void OnSwapLeave(GumInvocationContext *context, gpointer user_data) {
    (void) context;
    (void) user_data;
}

static GumInvocationListenerCallbacks SwapCallbacks = {
    OnSwapEnter,
    OnSwapLeave
};

static void RenderFrame(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (glWidth <= 0 || glHeight <= 0) return;

    if (!setup) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);

        ImGui_ImplOpenGL3_Init("#version 300 es");

        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;

        io.Fonts->AddFontFromMemoryTTF(
            (void *)&Roboto_Regular,
            sizeof(Roboto_Regular),
            22.0f,
            &font_cfg,
            io.Fonts->GetGlyphRangesCyrillic()
        );

        setup = true;
    }

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);

    static bool lastKeyboardState = false;
    bool keyboardState = io.WantTextInput;

    if (keyboardState != lastKeyboardState) {
        lastKeyboardState = keyboardState;
        ShowKeyboard(keyboardState);
    }

    if (!TouchCountMethod) {
        TouchCountMethod = Il2cpp::FindMethod(
            OBFUSCATE("UnityEngine.Input"),
            OBFUSCATE("get_touchCount"),
            0
        );
    }

    if (!GetTouchMethodInfo) {
        GetTouchMethodInfo = Il2cpp::FindMethod(
            OBFUSCATE("UnityEngine.Input"),
            OBFUSCATE("GetTouch"),
            1
        );
    }

    if (TouchCountMethod && GetTouchMethodInfo) {
        int touchCount = TouchCountMethod->invoke_static<int>();

        if (touchCount > 0) {
            auto touch = GetTouchMethodInfo->invoke_static<UnityEngine_Touch_Fields>(0);
            float reverseY = io.DisplaySize.y - touch.m_Position.fields.y;

            switch (touch.m_Phase) {
                case TouchPhase::Began:
                case TouchPhase::Stationary:
                    io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
                    io.MouseDown[0] = true;
                    break;

                case TouchPhase::Ended:
                case TouchPhase::Canceled:
                    io.MouseDown[0] = false;
                    should_clear_mouse_pos = true;
                    break;

                case TouchPhase::Moved:
                    io.MousePos = ImVec2(touch.m_Position.fields.x, reverseY);
                    break;

                default:
                    break;
            }
        } else {
            io.MouseDown[0] = false;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    BeginDraw();
    DrawToast();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static void *Init(void *) {
    SMAttach(targetLibName);

    bool runtime = ScriptBridge_Init();

    if (runtime) {
        AgentReady.store(ScriptBridge_LoadAgent());
    }

    gum_init_embedded();

    Interceptor = gum_interceptor_obtain();

    gpointer swapBuffers = gum_module_find_export_by_name(
        "libEGL.so",
        "eglSwapBuffers"
    );

    if (swapBuffers) {
        GumInvocationListener *listener = gum_make_call_listener(
            (GumInvocationListenerCallbacks *) &SwapCallbacks,
            nullptr,
            nullptr
        );

        if (listener) {
            gum_interceptor_attach(
                Interceptor,
                swapBuffers,
                listener,
                nullptr,
                GUM_ATTACH_FLAGS_NONE
            );

            SwapListener = listener;
        }
    }

    return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;

    JNIEnv *env = nullptr;

    LoadDex(imgui_dex, imgui_dex_len);
    vm->GetEnv((void **)&env, JNI_VERSION_1_6);

    pthread_t thread;

    if (pthread_create(&thread, nullptr, Init, nullptr) == 0) {
        pthread_detach(thread);
    }

    return JNI_VERSION_1_6;
}
