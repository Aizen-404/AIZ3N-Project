
#include "EGL/egl.h"
#include "GLES3/gl3.h"
#include <android/input.h>
#include <android/native_window.h>
#include <pthread.h>
#include <unistd.h>
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

static float ManualFlyGravity = 0.01f;
static bool FlyActive = false;
static bool AgentLoaded = false;
static bool RuntimeReady = false;
static char AgentMessage[512] = "Runtime not initialized";
static MethodInfo *TouchCountMethod = nullptr;
static MethodInfo *GetTouchMethodInfo = nullptr;

static void BeginDraw() {
    ImGui::SetNextWindowSize(
        ImVec2(540, 560),
        ImGuiCond_FirstUseEver
    );

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 7.0f);
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(12, 14)
    );

    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.035f, 0.045f, 0.075f, 0.98f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.0f, 0.75f, 1.0f, 0.55f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_TitleBg,
        ImVec4(0.025f, 0.035f, 0.065f, 1.0f)
    );
    ImGui::PushStyleColor(
        ImGuiCol_TitleBgActive,
        ImVec4(0.025f, 0.09f, 0.15f, 1.0f)
    );

    if (ImGui::Begin(
        "AIZ3N | FLY MOD",
        nullptr,
        ImGuiWindowFlags_NoSavedSettings
    )) {
        ImGui::Spacing();

        ImGui::TextColored(
            ImVec4(0.0f, 0.85f, 1.0f, 1.0f),
            "A I Z 3 N"
        );

        ImGui::SameLine();
        ImGui::TextDisabled("| PREMIUM CONTROL");

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(
            ImVec4(0.0f, 1.0f, 0.85f, 1.0f),
            "[ FRIDA GUMJS SYSTEM ]"
        );

        ImGui::Spacing();

        ImGui::BeginChild(
            "GravityStatus",
            ImVec2(0, 105),
            true
        );

        ImGui::TextDisabled("RUNTIME STATUS");

        ImGui::SameLine();

        ImGui::TextColored(
            RuntimeReady
                ? ImVec4(0.0f, 1.0f, 0.8f, 1.0f)
                : ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
            RuntimeReady ? "INITIALIZED" : "NOT INITIALIZED"
        );

        ImGui::Spacing();

        ImGui::TextDisabled("AGENT STATUS");

        ImGui::SameLine();

        ImGui::TextColored(
            AgentLoaded
                ? ImVec4(0.0f, 1.0f, 0.8f, 1.0f)
                : ImVec4(0.65f, 0.7f, 0.8f, 1.0f),
            AgentLoaded ? "LOADED" : "NOT LOADED"
        );

        ImGui::Spacing();

        ImGui::TextColored(
            FlyActive
                ? ImVec4(0.0f, 1.0f, 0.7f, 1.0f)
                : ImVec4(0.65f, 0.7f, 0.8f, 1.0f),
            FlyActive ? "FLY MODE ACTIVE" : "NORMAL GRAVITY"
        );

        ImGui::EndChild();

        ImGui::Spacing();

        ImGui::Text("RUNTIME CONTROLS");
        ImGui::Separator();

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.12f, 0.25f, 0.42f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.15f, 0.42f, 0.68f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.05f, 0.3f, 0.55f, 1.0f)
        );

        if (ImGui::Button(
            "INITIALIZE RUNTIME",
            ImVec2(-1.0f, 48)
        )) {
            RuntimeReady = ScriptBridge_Init();

            snprintf(
                AgentMessage,
                sizeof(AgentMessage),
                "%s",
                RuntimeReady
                    ? "Runtime initialized"
                    : "Runtime initialization failed"
            );
        }

        if (ImGui::Button(
            "LOAD AGENT",
            ImVec2(-1.0f, 48)
        )) {
            if (RuntimeReady) {
                AgentLoaded = ScriptBridge_LoadAgent();

                snprintf(
                    AgentMessage,
                    sizeof(AgentMessage),
                    "%s",
                    AgentLoaded
                        ? "Agent loaded"
                        : "Agent loading failed"
                );
            }
        }

        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        ImGui::Text("FLIGHT CONTROLS");
        ImGui::Separator();

        const float buttonWidth =
            (ImGui::GetContentRegionAvail().x - 12.0f) / 2.0f;

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.0f, 0.38f, 0.43f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.0f, 0.65f, 0.7f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.0f, 0.3f, 0.35f, 1.0f)
        );

        if (ImGui::Button(
            "FLY MODE",
            ImVec2(buttonWidth, 52)
        )) {
            if (AgentLoaded) {
                if (ScriptBridge_SendCommand(
                    "{\"type\":\"command\",\"payload\":{\"action\":\"enable\"}}"
                )) {
                    FlyActive = true;
                }
            }
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.12f, 0.25f, 0.42f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.15f, 0.42f, 0.68f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.05f, 0.3f, 0.55f, 1.0f)
        );

        if (ImGui::Button(
            "RESET GRAVITY",
            ImVec2(buttonWidth, 52)
        )) {
            if (AgentLoaded) {
                if (ScriptBridge_SendCommand(
                    "{\"type\":\"command\",\"payload\":{\"action\":\"disable\"}}"
                )) {
                    FlyActive = false;
                }
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        ImGui::Text("CUSTOM FLY GRAVITY");
        ImGui::Separator();

        ImGui::SetNextItemWidth(-1.0f);

        ImGui::InputFloat(
            "##ManualFlyGravity",
            &ManualFlyGravity,
            0.001f,
            0.01f,
            "%.4f"
        );

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.35f, 0.18f, 0.48f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.55f, 0.25f, 0.7f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.3f, 0.12f, 0.4f, 1.0f)
        );

        if (ImGui::Button(
            "SET FLY VALUE",
            ImVec2(-1.0f, 48)
        )) {
            if (AgentLoaded) {
                char command[160];

                snprintf(
                    command,
                    sizeof(command),
                    "{\"type\":\"command\",\"payload\":{\"action\":\"set_gravity\",\"value\":%.6f}}",
                    ManualFlyGravity
                );

                ScriptBridge_SendCommand(command);
            }
        }

        ImGui::PopStyleColor(3);

        ImGui::Spacing();

        if (ImGui::Button(
            "GET AGENT STATUS",
            ImVec2(-1.0f, 45)
        )) {
            if (AgentLoaded) {
                ScriptBridge_SendCommand(
                    "{\"type\":\"command\",\"payload\":{\"action\":\"status\"}}"
                );
            }
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.45f, 0.12f, 0.16f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.65f, 0.16f, 0.2f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.35f, 0.08f, 0.12f, 1.0f)
        );

        if (ImGui::Button(
            "UNLOAD AGENT",
            ImVec2(-1.0f, 45)
        )) {
            if (AgentLoaded) {
                ScriptBridge_UnloadAgent();
                AgentLoaded = false;
                FlyActive = false;
            }
        }

        ImGui::PopStyleColor(3);

        ScriptBridge_GetLastMessage(
            AgentMessage,
            sizeof(AgentMessage)
        );

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled("AGENT MESSAGE");
        ImGui::TextWrapped("%s", AgentMessage);

        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextDisabled("AIZ3N NATIVE | FRIDA GUMJS");
    }

    ImGui::End();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
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
            40.0f,
            &font_cfg,
            io.Fonts->GetGlyphRangesCyrillic()
        );

        ImGui::GetStyle().ScaleAllSizes(2.0f);

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

    RuntimeReady = ScriptBridge_Init();

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
