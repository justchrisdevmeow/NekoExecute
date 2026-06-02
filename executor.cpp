#include <windows.h>
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include "offsets.h"

#pragma comment(lib, "d3d9.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool showMenu = true;
char scriptBuffer[65536] = {0};
HWND robloxWindow = NULL;
LPDIRECT3DDEVICE9 device = NULL;
LPDIRECT3D9 d3d = NULL;

typedef uintptr_t(__cdecl* luau_load_t)(uintptr_t, const char*, const char*, size_t, int);
typedef int(__cdecl* lua_pcall_t)(uintptr_t, int, int, int);

uintptr_t GetLuaState() {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    return *(uintptr_t*)(base + Offsets::LuaState);
}

void ExecuteLua(const std::string& script) {
    uintptr_t L = GetLuaState();
    if (!L) return;

    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    luau_load_t luau_load = (luau_load_t)(base + Offsets::luau_load);
    lua_pcall_t lua_pcall = (lua_pcall_t)(base + Offsets::lua_pcall);

    std::string wrapped = "spawn(function()\n" + script + "\nend)";

    int load = luau_load(L, "@NekoExecute", wrapped.c_str(), wrapped.size(), 0);
    if (load == 0) {
        lua_pcall(L, 0, 0, 0);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Render() {
    if (!showMenu) return;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("NekoExecute", &showMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::InputTextMultiline("##script", scriptBuffer, IM_ARRAYSIZE(scriptBuffer), ImVec2(780, 500));
    
    if (ImGui::Button("EXECUTE", ImVec2(150, 40))) {
        ExecuteLua(scriptBuffer);
    }
    ImGui::SameLine();
    if (ImGui::Button("CLEAR", ImVec2(120, 40))) {
        memset(scriptBuffer, 0, sizeof(scriptBuffer));
    }

    ImGui::End();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

DWORD WINAPI MainThread(LPVOID) {
    while (!robloxWindow) {
        robloxWindow = FindWindowA(NULL, "Roblox");
        Sleep(200);
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = robloxWindow;
    d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, robloxWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &device);

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(robloxWindow);
    ImGui_ImplDX9_Init(device);

    while (true) {
        if (GetAsyncKeyState(VK_INSERT) & 1) showMenu = !showMenu;
        Render();
        Sleep(10);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    return TRUE;
}
