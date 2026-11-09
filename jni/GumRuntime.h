
#pragma once

bool GumRuntime_Init();
bool GumRuntime_LoadAgent();
void GumRuntime_UnloadAgent();
bool GumRuntime_IsLoaded();
bool GumRuntime_Post(const char *message);
void GumRuntime_SetMessageHandler(void (*handler)(const char *message));
void GumRuntime_Shutdown();
