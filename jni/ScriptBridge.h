
#pragma once

#include <cstddef>

bool ScriptBridge_Init();
bool ScriptBridge_LoadAgent();
bool ScriptBridge_SendCommand(const char *json);
bool ScriptBridge_IsAgentLoaded();
bool ScriptBridge_GetLastMessage(char *buffer, size_t size);
void ScriptBridge_UnloadAgent();
void ScriptBridge_Shutdown();
