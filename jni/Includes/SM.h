#pragma once

// Helper to wait for libil2cpp.so and initialize Il2cpp
void SMAttach(const char* smLibName);

// Helper to find a method pointer
void* getFun(const char* klassName, const char* Sname, int argsCount);
