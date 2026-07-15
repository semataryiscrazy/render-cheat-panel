#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <cstdio>
#include <cstring>

// saída JSON pura, sem console
// uso: injector_cli.exe --process "HD-Player.exe" --dll "C:\path\to\cheat.dll"

void json_out(bool success, const char* msg, DWORD pid = 0) {
    printf("{\"success\":%s,\"msg\":\"%s\",\"pid\":%lu}",
        success ? "true" : "false",
        msg,
        pid);
}

DWORD find_pid(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(snap, &pe)) do {
        if (lstrcmpiA(pe.szExeFile, name) == 0) {
            CloseHandle(snap);
            return pe.th32ProcessID;
        }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return 0;
}

bool enable_debug() {
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid)) {
        CloseHandle(token);
        return false;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool ok = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(token);
    return ok && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

// ─── verificação de arquitetura ──────────────────────────────
bool is_process_64_bit(HANDLE process) {
    BOOL is_wow64 = FALSE;
    IsWow64Process(process, &is_wow64);
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        return !is_wow64;
    }
    return false;
}

bool check_compatibility(HANDLE process, DWORD pid) {
    bool target_64 = is_process_64_bit(process);
#ifdef _WIN64
    bool injector_64 = true;
#else
    bool injector_64 = false;
#endif
    if (target_64 != injector_64) {
        char msg[128];
        sprintf_s(msg, "architecture mismatch: target=%s injector=%s",
            target_64 ? "64-bit" : "32-bit",
            injector_64 ? "64-bit" : "32-bit");
        json_out(false, msg, pid);
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    const char* process_name = NULL;
    const char* dll_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--process") == 0 && i + 1 < argc)
            process_name = argv[++i];
        if (strcmp(argv[i], "--dll") == 0 && i + 1 < argc)
            dll_path = argv[++i];
    }

    if (!process_name || !dll_path) {
        json_out(false, "usage: injector_cli --process <name> --dll <path>");
        return 1;
    }

    // verifica se a dll existe
    if (GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        char msg[128];
        sprintf_s(msg, "dll not found: %s", dll_path);
        json_out(false, msg);
        return 1;
    }

    enable_debug();

    DWORD pid = find_pid(process_name);
    if (!pid) {
        json_out(false, "process not found, make sure the emulator is running");
        return 1;
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProcess) {
        char err[128];
        sprintf_s(err, "OpenProcess failed: %lu. try running as admin.", GetLastError());
        json_out(false, err, pid);
        return 1;
    }

    // verifica compatibilidade de arquitetura
    if (!check_compatibility(hProcess, pid)) {
        CloseHandle(hProcess);
        return 1;
    }

    size_t path_len = strlen(dll_path) + 1;
    void* remote_mem = VirtualAllocEx(hProcess, NULL, path_len,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        char err[64];
        sprintf_s(err, "VirtualAllocEx failed: %lu", GetLastError());
        json_out(false, err, pid);
        CloseHandle(hProcess);
        return 1;
    }

    WriteProcessMemory(hProcess, remote_mem, dll_path, path_len, NULL);

    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadlib = GetProcAddress(kernel32, "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadlib, remote_mem, 0, NULL);
    if (!hThread) {
        char err[64];
        sprintf_s(err, "CreateRemoteThread failed: %lu", GetLastError());
        json_out(false, err, pid);
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // espera a thread completar (timeout 10s pra dar tempo do DllMain executar)
    DWORD wait_result = WaitForSingleObject(hThread, 10000);

    DWORD exit_code = 0;
    GetExitCodeThread(hThread, &exit_code);

    VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    if (wait_result == WAIT_TIMEOUT) {
        json_out(true, "injected (thread still running)", pid);
        return 0;
    }

    if (exit_code == 0) {
        json_out(false, "DllMain returned FALSE, check if DLL is compatible with this process", pid);
        return 1;
    }

    char msg[64];
    sprintf_s(msg, "injected successfully");
    json_out(true, msg, pid);
    return 0;
}
