#include <windows.h>
#include <iostream>
#include <string>
#include <TlHelp32.h>

DWORD GetProcID(const char* procName) {
    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hSnap, &pe32)) {
        do {
            if (!_stricmp(pe32.szExeFile, procName)) {
                CloseHandle(hSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
    return 0;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;

    SIZE_T pathLen = strlen(dllPath) + 1;
    LPVOID alloc = VirtualAllocEx(hProc, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(hProc, alloc, dllPath, pathLen, NULL);

    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, alloc, 0, NULL);
    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}

int main() {
    DWORD pid = GetProcID("RobloxPlayerBeta.exe");
    if (!pid) {
        std::cout << "Roblox not running.\n";
        system("pause");
        return 1;
    }

    if (InjectDLL(pid, "NekoExecute.dll")) {
        std::cout << "NekoExecute injected.\n";
    } else {
        std::cout << "Injection failed.\n";
    }
    system("pause");
    return 0;
}
