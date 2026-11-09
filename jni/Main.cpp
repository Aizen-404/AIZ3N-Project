
#include "EGL/egl.h"
#include "GLES3/gl3.h"
#include <android/input.h>
#include <android/native_window.h>
#include <pthread.h>
#include <cstdio>
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
#include "And64InlineHook/And64InlineHook.hpp"
#include "Hook.h"
#include "ScriptBridge.h"

static bool FlyActive = false;
static bool AgentLoaded = false;
static MethodInfo *TouchCountMethod = nullptr;
static MethodInfo *GetTouchMethodInfo = nullptr;

static void BeginDraw() {
    ImGui::SetNextWindowSize(
        ImVec2(340, 205),
        ImGuiCond_FirstUseEver
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 9.0f);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(16, 14)
    );
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(10, 12)
    );

    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.018f, 0.025f, 0.045f, 0.98f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.0f, 0.78f, 1.0f, 0.85f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_TitleBg,
        ImVec4(0.025f, 0.045f, 0.075f, 1.0f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_TitleBgActive,
        ImVec4(0.025f, 0.09f, 0.15f, 1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.0f, 0.32f, 0.42f, 1.0f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.0f, 0.52f, 0.68f, 1.0f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.0f, 0.25f, 0.38f, 1.0f)
    );

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.88f, 0.96f, 1.0f, 1.0f)
    );

    if (ImGui::Begin(
        "AIZ3N | VIP FLY",
        nullptr,
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse
    )) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImVec4(0.0f, 0.88f, 1.0f, 1.0f)
        );

        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted("A I Z 3 N");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::PopStyleColor();

        ImGui::SameLine();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);

        ImGui::TextDisabled(
            AgentLoaded ? "[ SYSTEM ONLINE ]" : "[ LOADING SYSTEM ]"
        );

        ImGui::Spacing();

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            FlyActive
                ? ImVec4(0.0f, 0.42f, 0.38f, 1.0f)
                : ImVec4(0.0f, 0.32f, 0.42f, 1.0f)
        );

        if (ImGui::Button(
            "FLY MODE  |  0.01",
            ImVec2(-1.0f, 52)
        )) {
            if (AgentLoaded && ScriptBridge_SendCommand(
                "{\"type\":\"command\",\"payload\":{\"action\":\"set_gravity\",\"value\":0.01}}"
            )) {
                FlyActive = true;
            }
        }

        ImGui::PopStyleColor();

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.35f, 0.25f, 0.08f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.55f, 0.38f, 0.08f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.3f, 0.2f, 0.05f, 1.0f)
        );

        if (ImGui::Button(
            "RESET GRAVITY  |  1.0",
            ImVec2(-1.0f, 52)
        )) {
            if (AgentLoaded && ScriptBridge_SendCommand(
                "{\"type\":\"command\",\"payload\":{\"action\":\"set_gravity\",\"value\":1.0}}"
            )) {
                FlyActive = false;
            }
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::End();

    ImGui::PopStyleColor(8);
    ImGui::PopStyleVar(5);
}

EGLBoolean (*orig_eglSwapBuffers)(
    EGLDisplay,
    EGLSurface
) = nullptr;

static EGLBoolean _eglSwapBuffers(
    EGLDisplay dpy,
    EGLSurface surface
) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (glWidth <= 0 || glHeight <= 0) {
        return orig_eglSwapBuffers
            ? orig_eglSwapBuffers(dpy, surface)
            : EGL_FALSE;
    }

    if (!setup) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        ImGui::GetStyle().WindowTitleAlign =
            ImVec2(0.5f, 0.5f);

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

    io.DisplaySize = ImVec2(
        (float)glWidth,
        (float)glHeight
    );

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
        int touchCount =
            TouchCountMethod->invoke_static<int>();

        if (touchCount > 0) {
            auto touch = GetTouchMethodInfo
                ->invoke_static<UnityEngine_Touch_Fields>(0);

            float reverseY =
                io.DisplaySize.y - touch.m_Position.fields.y;

            switch (touch.m_Phase) {
                case TouchPhase::Began:
                case TouchPhase::Stationary:
                    io.MousePos = ImVec2(
                        touch.m_Position.fields.x,
                        reverseY
                    );
                    io.MouseDown[0] = true;
                    break;

                case TouchPhase::Ended:
                case TouchPhase::Canceled:
                    io.MouseDown[0] = false;
                    should_clear_mouse_pos = true;
                    break;

                case TouchPhase::Moved:
                    io.MousePos = ImVec2(
                        touch.m_Position.fields.x,
                        reverseY
                    );
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

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(
        ImGui::GetDrawData()
    );

    return orig_eglSwapBuffers
        ? orig_eglSwapBuffers(dpy, surface)
        : EGL_FALSE;
}

static void *Init(void *) {
    SMAttach(targetLibName);

    if (ScriptBridge_Init()) {
        AgentLoaded = ScriptBridge_LoadAgent();
    }

    auto swapBuffers = DobbySymbolResolver(
        OBFUSCATE("libEGL.so"),
        OBFUSCATE("eglSwapBuffers")
    );

    if (swapBuffers) {
        DobbyHook(
            (void *)swapBuffers,
            (void *)_eglSwapBuffers,
            (void **)&orig_eglSwapBuffers
        );
    }

    return nullptr;
}

JNIEXPORT jint JNICALL JNI_OnLoad(
    JavaVM *vm,
    void *reserved
) {
    jvm = vm;

    JNIEnv *env = nullptr;

    LoadDex(imgui_dex, imgui_dex_len);

    vm->GetEnv(
        (void **)&env,
        JNI_VERSION_1_6
    );

    pthread_t thread;

    if (pthread_create(
        &thread,
        nullptr,
        Init,
        nullptr
    ) == 0) {
        pthread_detach(thread);
    }

    return JNI_VERSION_1_6;
}
