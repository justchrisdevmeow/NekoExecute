#include <windows.h>
#include <iostream>
#include <string>
#include "offsets.h"

uintptr_t base;
uintptr_t L;

// Hide thread from Hyperion
void HideThread(HANDLE hThread) {
    typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    pNtSetInformationThread NtSetInformationThread = (pNtSetInformationThread)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    
    // ThreadHideFromDebugger = 0x11
    NtSetInformationThread(hThread, 0x11, 0, 0);
}

// Delayed execution to avoid early detection
DWORD WINAPI DelayedMain(LPVOID) {
    Sleep(15000); // Wait 15 seconds after injection
    
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    
    base = (uintptr_t)GetModuleHandleA(NULL);
    
    // Wait for Lua state to be ready
    while (!L) {
        L = *(uintptr_t*)(base + Offsets::LuaState);
        Sleep(100);
    }
    
    auto luau_load = (uintptr_t(*)(uintptr_t, const char*, const char*, size_t, int))(base + Offsets::luau_load);
    auto lua_pcall = (int(*)(uintptr_t, int, int, int))(base + Offsets::lua_pcall);
    
    std::cout << "[NekoExecute] Ready\n";
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        
        std::string wrapped = "spawn(function() " + input + " end)";
        if (luau_load(L, "@exec", wrapped.c_str(), wrapped.size(), 0) == 0)
            lua_pcall(L, 0, 0, 0);
    }
    
    FreeConsole();
    return 0;
}

DWORD WINAPI MainThread(LPVOID) {
    // Hide this thread from Hyperion
    HideThread(GetCurrentThread());
    
    // Create delayed main thread
    HANDLE hThread = CreateThread(NULL, 0, DelayedMain, NULL, 0, NULL);
    HideThread(hThread);
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        
        // Random delay before creating main thread (30-90 seconds)
        srand(time(NULL));
        int delay = 30000 + (rand() % 60000);
        Sleep(delay);
        
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    return TRUE;
}
