#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include "offsets.h"

#pragma comment(linker, "/SUBSYSTEM:CONSOLE")

HANDLE hPipe = INVALID_HANDLE_VALUE;
uintptr_t luaState = 0;
uintptr_t baseAddr = 0;

void Log(const char* msg) {
    std::ofstream log("executor.log", std::ios::app);
    time_t t = time(nullptr);
    log << ctime(&t) << " " << msg << std::endl;
}

uintptr_t GetLuaState() {
    baseAddr = (uintptr_t)GetModuleHandleA(NULL);
    if (!baseAddr) return 0;
    return *(uintptr_t*)(baseAddr + Offsets::LuaState);
}

bool ExecuteScript(const std::string& script) {
    luaState = GetLuaState();
    if (!luaState) {
        Log("Lua state is null");
        return false;
    }
    
    typedef uintptr_t(__cdecl* luau_load_t)(uintptr_t, const char*, const char*, size_t, int);
    typedef int(__cdecl* lua_pcall_t)(uintptr_t, int, int, int);
    
    luau_load_t luau_load = (luau_load_t)(baseAddr + Offsets::luau_load);
    lua_pcall_t lua_pcall = (lua_pcall_t)(baseAddr + Offsets::lua_pcall);
    
    if (!luau_load || !lua_pcall) {
        Log("Function pointers invalid");
        return false;
    }
    
    std::string wrapped = "spawn(function()\n" + script + "\nend)";
    int loadRes = luau_load(luaState, "@NekoExecute", wrapped.c_str(), wrapped.size(), 0);
    
    if (loadRes == 0) {
        int pcallRes = lua_pcall(luaState, 0, 0, 0);
        return (pcallRes == 0);
    }
    return false;
}

void PipeServer() {
    while (true) {
        hPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\NekoPipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 4096, 4096, 0, NULL
        );
        
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            char buffer[65536] = {0};
            DWORD read = 0;
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &read, NULL)) {
                ExecuteScript(std::string(buffer));
            }
            FlushFileBuffers(hPipe);
            DisconnectNamedPipe(hPipe);
        }
        CloseHandle(hPipe);
        Sleep(100);
    }
}

DWORD WINAPI MainThread(LPVOID) {
    Log("Executor loaded");
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    
    std::cout << "[NekoExecute] Console Mode\nType 'help' for commands\n";
    
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PipeServer, NULL, 0, NULL);
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "exit" || input == "quit") {
            break;
        }
        else if (input == "help") {
            std::cout << "run <file> - Execute script from file\n";
            std::cout << "exec <code> - Execute raw Lua\n";
            std::cout << "state - Show Lua state address\n";
            std::cout << "clear - Clear screen\n";
            std::cout << "exit - Unload\n";
        }
        else if (input.rfind("run ", 0) == 0) {
            std::string path = input.substr(4);
            std::ifstream file(path);
            if (file) {
                std::stringstream ss;
                ss << file.rdbuf();
                if (ExecuteScript(ss.str())) {
                    std::cout << "[+] Executed\n";
                } else {
                    std::cout << "[!] Failed\n";
                }
            } else {
                std::cout << "[!] File not found\n";
            }
        }
        else if (input.rfind("exec ", 0) == 0) {
            std::string code = input.substr(5);
            if (ExecuteScript(code)) {
                std::cout << "[+] Executed\n";
            } else {
                std::cout << "[!] Failed\n";
            }
        }
        else if (input == "state") {
            uintptr_t state = GetLuaState();
            if (state) {
                std::cout << "[+] LuaState: 0x" << std::hex << state << "\n";
            } else {
                std::cout << "[!] Failed to get state\n";
            }
        }
        else if (input == "clear") {
            system("cls");
        }
        else if (!input.empty()) {
            std::cout << "Unknown command. Type 'help'\n";
        }
    }
    
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)GetModuleHandleA(NULL), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    return TRUE;
}
