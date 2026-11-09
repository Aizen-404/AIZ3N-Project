
#include "ScriptBridge.h"
#include "GumRuntime.h"
#include <mutex>
#include <string>
#include <cstring>

static std::mutex message_mutex;
static std::string last_message;

static void OnAgentMessage(const char *message) {
    if (!message) {
        return;
    }

    std::lock_guard<std::mutex> lock(message_mutex);
    last_message = message;
}

bool ScriptBridge_Init() {
    GumRuntime_SetMessageHandler(OnAgentMessage);
    return GumRuntime_Init();
}

bool ScriptBridge_LoadAgent() {
    {
        std::lock_guard<std::mutex> lock(message_mutex);
        last_message.clear();
    }

    return GumRuntime_LoadAgent();
}

bool ScriptBridge_SendCommand(const char *json) {
    if (!json || !GumRuntime_IsLoaded()) {
        return false;
    }

    return GumRuntime_Post(json);
}

bool ScriptBridge_IsAgentLoaded() {
    return GumRuntime_IsLoaded();
}

bool ScriptBridge_GetLastMessage(
    char *buffer,
    size_t size
) {
    if (!buffer || size == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(message_mutex);

    if (last_message.empty()) {
        buffer[0] = '\0';
        return false;
    }

    std::strncpy(
        buffer,
        last_message.c_str(),
        size - 1
    );

    buffer[size - 1] = '\0';

    return true;
}

void ScriptBridge_UnloadAgent() {
    GumRuntime_UnloadAgent();
}

void ScriptBridge_Shutdown() {
    GumRuntime_Shutdown();

    std::lock_guard<std::mutex> lock(message_mutex);
    last_message.clear();
}
