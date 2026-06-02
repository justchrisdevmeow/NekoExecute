#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <fstream>
#include <iostream>

typedef NTSTATUS(NTAPI* pNtCreateThreadEx)(PHANDLE, ACCESS_MASK, PVOID, HANDLE, LPTHREAD_START_ROUTINE, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID);
typedef NTSTATUS(NTAPI* pNtAllocateVirtualMemory)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);

DWORD FindProc(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(snap, &pe)) {
        do if (!_stricmp(pe.szExeFile, name)) { CloseHandle(snap); return pe.th32ProcessID; }
        while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

int main() {
    DWORD pid = FindProc("RobloxPlayerBeta.exe");
    if (!pid) pid = FindProc("RobloxPlayer.exe");
    if (!pid) { std::cout << "Roblox not running\n"; system("pause"); return 1; }
    
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) { std::cout << "OpenProcess failed. Run as admin.\n"; system("pause"); return 1; }
    
    std::ifstream dll("NekoExecute.dll", std::ios::binary | std::ios::ate);
    if (!dll) { std::cout << "DLL not found\n"; system("pause"); return 1; }
    
    std::vector<char> buf(dll.tellg());
    dll.seekg(0);
    dll.read(buf.data(), buf.size());
    dll.close();
    
    PVOID alloc = nullptr;
    SIZE_T size = buf.size();
    
    auto NtAllocateVirtualMemory = (pNtAllocateVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtAllocateVirtualMemory");
    auto NtCreateThreadEx = (pNtCreateThreadEx)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx");
    
    NtAllocateVirtualMemory(hProc, &alloc, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(hProc, alloc, buf.data(), buf.size(), nullptr);
    
    HANDLE hThread;
    NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, hProc, (LPTHREAD_START_ROUTINE)alloc, nullptr, 0, 0, 0, 0, nullptr);
    
    std::cout << "Injected\n";
    system("pause");
    return 0;
}
