#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <TlHelp32.h>

#pragma comment(lib, "ntdll.lib")

typedef NTSTATUS(NTAPI* pNtCreateThreadEx)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList
);

typedef NTSTATUS(NTAPI* pNtAllocateVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    PSIZE_T RegionSize,
    ULONG AllocationType,
    ULONG Protect
);

typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten
);

typedef NTSTATUS(NTAPI* pNtProtectVirtualMemory)(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    PSIZE_T NumberOfBytesToProtect,
    ULONG NewAccessProtection,
    PULONG OldAccessProtection
);

pNtCreateThreadEx NtCreateThreadEx = nullptr;
pNtAllocateVirtualMemory NtAllocateVirtualMemory = nullptr;
pNtWriteVirtualMemory NtWriteVirtualMemory = nullptr;
pNtProtectVirtualMemory NtProtectVirtualMemory = nullptr;

void InitSyscalls() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtCreateThreadEx = (pNtCreateThreadEx)GetProcAddress(ntdll, "NtCreateThreadEx");
    NtAllocateVirtualMemory = (pNtAllocateVirtualMemory)GetProcAddress(ntdll, "NtAllocateVirtualMemory");
    NtWriteVirtualMemory = (pNtWriteVirtualMemory)GetProcAddress(ntdll, "NtWriteVirtualMemory");
    NtProtectVirtualMemory = (pNtProtectVirtualMemory)GetProcAddress(ntdll, "NtProtectVirtualMemory");
}

void Log(const char* msg) {
    std::ofstream log("injector.log", std::ios::app);
    time_t t = time(nullptr);
    log << ctime(&t) << " " << msg << std::endl;
}

DWORD FindProcess(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

bool ManualMap(DWORD pid, const std::vector<BYTE>& dllData) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;
    
    SIZE_T size = dllData.size();
    PVOID alloc = nullptr;
    NTSTATUS status = NtAllocateVirtualMemory(hProc, &alloc, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (status != 0) {
        CloseHandle(hProc);
        return false;
    }
    
    SIZE_T written = 0;
    status = NtWriteVirtualMemory(hProc, alloc, (PVOID)dllData.data(), dllData.size(), &written);
    if (status != 0) {
        NtAllocateVirtualMemory(hProc, &alloc, 0, &size, MEM_RELEASE, PAGE_READWRITE);
        CloseHandle(hProc);
        return false;
    }
    
    HANDLE hThread = nullptr;
    status = NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, nullptr, hProc, (LPTHREAD_START_ROUTINE)alloc, nullptr, 0, 0, 0, 0, nullptr);
    
    CloseHandle(hProc);
    return (status == 0);
}

int main() {
    system("title NekoInjector");
    InitSyscalls();
    
    std::cout << "[NekoInjector] Manual Map Mode\n";
    std::cout << "[1] Inject\n[2] Exit\n> ";
    
    int choice;
    std::cin >> choice;
    if (choice != 1) return 0;
    
    DWORD pid = FindProcess("RobloxPlayerBeta.exe");
    if (!pid) pid = FindProcess("RobloxPlayer.exe");
    if (!pid) {
        std::cout << "[!] Roblox not running\n";
        system("pause");
        return 1;
    }
    
    std::cout << "[+] Found Roblox PID: " << pid << "\n";
    
    std::ifstream dllFile("NekoExecute.dll", std::ios::binary | std::ios::ate);
    if (!dllFile) {
        std::cout << "[!] DLL not found\n";
        system("pause");
        return 1;
    }
    
    std::vector<BYTE> buffer(dllFile.tellg());
    dllFile.seekg(0, std::ios::beg);
    dllFile.read((char*)buffer.data(), buffer.size());
    dllFile.close();
    
    if (ManualMap(pid, buffer)) {
        std::cout << "[+] Manual map successful\n";
    } else {
        std::cout << "[!] Manual map failed\n";
    }
    
    system("pause");
    return 0;
}
