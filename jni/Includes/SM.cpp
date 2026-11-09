#include "SM.h"
#include "../KittyMemory/MemoryPatch.h"
#include "../Il2cpp/il2cpp-class.h"
#include "../Il2cpp/Il2cpp.h"
#include <unistd.h>

void SMAttach(const char* smLibName) {
    ProcMap il2cppMap;
    while (!il2cppMap.isValid()) {
        il2cppMap = KittyMemory::getLibraryBaseMap(smLibName);
        sleep(1);
    }
    Il2cpp::Init();
}

void* getFun(const char *klassName, const char *Sname, int argsCount) {
    Il2CppClass* smeClass = Il2cpp::FindClass(klassName);
    
    if(!smeClass) return nullptr;

    MethodInfo* smeMethod = smeClass->getMethod(Sname, argsCount);
    
    if(!smeMethod) return nullptr;
    
    return smeMethod->methodPointer;
}
