#include <windows.h>
#include <iostream>
#include <string>
#include <TlHelp32.h>
#include <fstream>
#include <ctime>

void LogInject(const char* message) {
    std::ofstream log("injector_debug.log", std::ios::app);
    time_t now = time(nullptr);
    log << ctime(&now) << " - " << message << std::endl;
    log.close();
}

DWORD GetProcID(const char* procName) {
    LogInject("Searching for process...");
    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        LogInject("Failed to create snapshot!");
        return 0;
    }
    
    if (Process32First(hSnap, &pe32)) {
        do {
            if (!_stricmp(pe32.szExeFile, procName)) {
                CloseHandle(hSnap);
                LogInject(("Found process: " + std::string(procName) + " with PID: " + std::to_string(pe32.th32ProcessID)).c_str());
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
    LogInject(("Process not found: " + std::string(procName)).c_str());
    return 0;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    LogInject(("Attempting to inject into PID: " + std::to_string(pid)).c_str());
    LogInject(("DLL Path: " + std::string(dllPath)).c_str());
    
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        LogInject("Failed to open process!");
        return false;
    }
    LogInject("Process opened successfully");

    SIZE_T pathLen = strlen(dllPath) + 1;
    LPVOID alloc = VirtualAllocEx(hProc, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!alloc) {
        LogInject("Failed to allocate memory in target process!");
        CloseHandle(hProc);
        return false;
    }
    LogInject(("Memory allocated at: 0x" + std::to_string((uintptr_t)alloc)).c_str());

    if (!WriteProcessMemory(hProc, alloc, dllPath, pathLen, NULL)) {
        LogInject("Failed to write DLL path to process memory!");
        VirtualFreeEx(hProc, alloc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    LogInject("DLL path written to target process");

    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, alloc, 0, NULL);
    if (!hThread) {
        LogInject("Failed to create remote thread!");
        VirtualFreeEx(hProc, alloc, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    LogInject("Remote thread created, waiting for injection...");
    
    WaitForSingleObject(hThread, INFINITE);
    LogInject("Injection completed!");
    
    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}

int main() {
    system("title NekoExecute Injector");
    std::cout << "====================================\n";
    std::cout << "     NekoExecute Roblox Injector     \n";
    std::cout << "====================================\n\n";
    
    LogInject("=== Injector Started ===");
    
    std::cout << "[1] Inject NekoExecute\n";
    std::cout << "[2] Exit\n";
    std::cout << "Choice: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice != 1) {
        LogInject("User chose to exit");
        return 0;
    }
    
    std::cout << "\nSearching for Roblox...\n";
    DWORD pid = GetProcID("RobloxPlayerBeta.exe");
    if (!pid) {
        pid = GetProcID("RobloxPlayer.exe");
    }
    
    if (!pid) {
        std::cout << "Roblox not running. Please start Roblox first.\n";
        system("pause");
        LogInject("Roblox not found, exiting");
        return 1;
    }
    
    std::cout << "Roblox found (PID: " << pid << ")\n";
    std::cout << "Getting DLL path...\n";
    
    char dllPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, dllPath);
    strcat_s(dllPath, "\\NekoExecute.dll");
    
    std::cout << "DLL Path: " << dllPath << "\n";
    std::cout << "Injecting...\n";
    
    if (InjectDLL(pid, dllPath)) {
        std::cout << "\n[SUCCESS] NekoExecute injected!\n";
        std::cout << "Check for the NekoExecute window in Roblox.\n";
        std::cout << "Press INSERT to hide/show the window.\n";
        LogInject("Injection successful!");
    } else {
        std::cout << "\n[FAILED] Injection failed.\n";
        std::cout << "Check injector_debug.log for details.\n";
        LogInject("Injection failed!");
    }
    
    std::cout << "\nPress any key to exit...\n";
    system("pause");
    return 0;
}
