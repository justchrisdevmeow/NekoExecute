#include <windows.h>
#include <string>
#include <fstream>
#include <ctime>
#include "offsets.h"

#pragma comment(lib, "user32.lib")

bool showMenu = true;
char scriptBuffer[65536] = {0};
HWND overlayWindow = NULL;
HWND editControl = NULL;

// Debug logging
void LogDebug(const char* message) {
    char logPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, logPath);
    strcat_s(logPath, "\\neko_debug.log");
    
    std::ofstream log(logPath, std::ios::app);
    time_t now = time(nullptr);
    log << ctime(&now) << " - " << message << std::endl;
    log.close();
}

typedef uintptr_t(__cdecl* luau_load_t)(uintptr_t, const char*, const char*, size_t, int);
typedef int(__cdecl* lua_pcall_t)(uintptr_t, int, int, int);

uintptr_t GetLuaState() {
    LogDebug("Getting Lua state...");
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    if (!base) {
        LogDebug("Failed to get module base!");
        return 0;
    }
    uintptr_t state = *(uintptr_t*)(base + Offsets::LuaState);
    LogDebug(("Lua state: 0x" + std::to_string(state)).c_str());
    return state;
}

void ExecuteLua(const std::string& script) {
    LogDebug("Executing Lua script...");
    LogDebug(("Script length: " + std::to_string(script.length())).c_str());
    
    uintptr_t L = GetLuaState();
    if (!L) {
        LogDebug("Lua state is NULL!");
        MessageBoxA(overlayWindow, "Failed to get Lua state!", "Error", MB_OK);
        return;
    }

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    luau_load_t luau_load = (luau_load_t)(base + Offsets::luau_load);
    lua_pcall_t lua_pcall = (lua_pcall_t)(base + Offsets::lua_pcall);

    std::string wrapped = "spawn(function()\n" + script + "\nend)";
    LogDebug(("Wrapped script: " + wrapped.substr(0, 100)).c_str());

    int load = luau_load(L, "@NekoExecute", wrapped.c_str(), wrapped.size(), 0);
    LogDebug(("luau_load result: " + std::to_string(load)).c_str());
    
    if (load == 0) {
        int pcallResult = lua_pcall(L, 0, 0, 0);
        LogDebug(("lua_pcall result: " + std::to_string(pcallResult)).c_str());
        if (pcallResult == 0) {
            MessageBoxA(overlayWindow, "Script executed successfully!", "Success", MB_OK);
        } else {
            MessageBoxA(overlayWindow, "Script execution failed!", "Error", MB_OK);
        }
    } else {
        MessageBoxA(overlayWindow, "luau_load failed! Check script syntax.", "Error", MB_OK);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            LogDebug("Window destroyed");
            PostQuitMessage(0);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // Execute button
                LogDebug("Execute button clicked");
                GetWindowTextA(editControl, scriptBuffer, sizeof(scriptBuffer));
                if (strlen(scriptBuffer) > 0) {
                    ExecuteLua(std::string(scriptBuffer));
                } else {
                    MessageBoxA(hWnd, "Please enter a script!", "Error", MB_OK);
                }
            }
            else if (LOWORD(wParam) == 2) { // Clear button
                LogDebug("Clear button clicked");
                memset(scriptBuffer, 0, sizeof(scriptBuffer));
                SetWindowTextA(editControl, "");
            }
            return 0;
        case WM_SIZE:
            // Resize edit control when window resizes
            if (editControl) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                SetWindowPos(editControl, NULL, 10, 35, rc.right - 20, rc.bottom - 100, SWP_NOZORDER);
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

DWORD WINAPI MainThread(LPVOID) {
    // Log start
    LogDebug("=== NekoExecute DLL Loaded ===");
    
    // Show injection confirmation
    MessageBoxA(NULL, "NekoExecute injected successfully!\nCheck neko_debug.log for details.", "Injection Confirmation", MB_OK | MB_ICONINFORMATION);
    LogDebug("Injection confirmed by user");
    
    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NekoExecuteClass";
    
    if (!RegisterClassExA(&wc)) {
        DWORD error = GetLastError();
        LogDebug(("Failed to register window class! Error: " + std::to_string(error)).c_str());
        MessageBoxA(NULL, "Failed to register window class!", "Error", MB_OK);
        return 1;
    }
    LogDebug("Window class registered");

    // Create window
    overlayWindow = CreateWindowExA(
        0,
        "NekoExecuteClass",
        "NekoExecute - Roblox Executor",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        850, 650,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!overlayWindow) {
        DWORD error = GetLastError();
        LogDebug(("Failed to create window! Error: " + std::to_string(error)).c_str());
        MessageBoxA(NULL, "Failed to create window!", "Error", MB_OK);
        return 1;
    }
    LogDebug("Window created successfully");

    // Create UI controls
    CreateWindowExA(0, "STATIC", "Script Editor:", WS_CHILD | WS_VISIBLE,
                    10, 10, 100, 20, overlayWindow, NULL, wc.hInstance, NULL);
    
    editControl = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", 
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                  10, 35, 810, 500, overlayWindow, (HMENU)3, wc.hInstance, NULL);
    
    CreateWindowExA(0, "BUTTON", "EXECUTE", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    10, 550, 120, 40, overlayWindow, (HMENU)1, wc.hInstance, NULL);
    
    CreateWindowExA(0, "BUTTON", "CLEAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    140, 550, 100, 40, overlayWindow, (HMENU)2, wc.hInstance, NULL);
    
    CreateWindowExA(0, "BUTTON", "TEST (print)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    250, 550, 120, 40, overlayWindow, (HMENU)4, wc.hInstance, NULL);
    
    // Status text
    HWND statusText = CreateWindowExA(0, "STATIC", "Ready", WS_CHILD | WS_VISIBLE,
                                      400, 560, 200, 20, overlayWindow, NULL, wc.hInstance, NULL);
    
    LogDebug("UI controls created");

    // Show window
    ShowWindow(overlayWindow, SW_SHOW);
    UpdateWindow(overlayWindow);
    SetFocus(editControl);
    LogDebug("Window shown and focused");
    
    // Update status
    SetWindowTextA(statusText, "Status: Ready - Press INSERT to toggle");
    
    // Message loop
    MSG msg;
    bool running = true;
    while (running && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        
        // Handle button clicks via command messages
        if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == 4) {
            LogDebug("Test button clicked");
            const char* testScript = "print('NekoExecute is working!\\nMade by you')";
            ExecuteLua(std::string(testScript));
            SetWindowTextA(statusText, "Status: Test script executed");
        }
        
        // Toggle menu/show with INSERT
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            LogDebug("INSERT key pressed");
            if (IsWindowVisible(overlayWindow)) {
                ShowWindow(overlayWindow, SW_HIDE);
                SetWindowTextA(statusText, "Status: Hidden (INSERT to show)");
            } else {
                ShowWindow(overlayWindow, SW_SHOW);
                SetWindowTextA(statusText, "Status: Visible");
            }
        }
    }
    
    LogDebug("=== NekoExecute Unloading ===");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hMod);
            CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
            break;
        case DLL_PROCESS_DETACH:
            LogDebug("DLL detaching");
            break;
    }
    return TRUE;
}
