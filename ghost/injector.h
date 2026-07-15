#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "psapi.lib")

struct InjectionResult {
    bool success;
    DWORD pid;
    HMODULE module_handle;
    char msg[256];
};

// ─── find process ──────────────────────────────────────────────
DWORD find_process(const char* name) {
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

// ─── enable debug privilege ───────────────────────────────────
bool enable_debug_privilege() {
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

// ─── check architecture ──────────────────────────────────────
bool is_process_64(HANDLE process) {
    BOOL wow64 = FALSE;
    IsWow64Process(process, &wow64);
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        return !wow64;
    }
    return false;
}

// ─── write DLL to temp ────────────────────────────────────────
bool write_dll_to_disk(const unsigned char* data, size_t size, const char* path) {
    FILE* f = NULL;
    if (fopen_s(&f, path, "wb") != 0 || !f) return false;
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return written == size;
}

// ─── inject DLL ───────────────────────────────────────────────
InjectionResult inject_dll(const char* process_name, const char* dll_path) {
    InjectionResult result = { false, 0, NULL, "" };
    
    if (GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        sprintf_s(result.msg, "dll not found: %s", dll_path);
        return result;
    }
    
    DWORD pid = find_process(process_name);
    if (!pid) {
        sprintf_s(result.msg, "process not found: %s", process_name);
        return result;
    }
    result.pid = pid;

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProcess) {
        sprintf_s(result.msg, "OpenProcess failed: %lu", GetLastError());
        return result;
    }

    // arch check
#ifdef _WIN64
    bool injector_64 = true;
#else
    bool injector_64 = false;
#endif
    if (is_process_64(hProcess) != injector_64) {
        sprintf_s(result.msg, "architecture mismatch");
        CloseHandle(hProcess);
        return result;
    }
    
    size_t path_len = strlen(dll_path) + 1;
    void* remote_mem = VirtualAllocEx(hProcess, NULL, path_len,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        sprintf_s(result.msg, "VirtualAllocEx failed: %lu", GetLastError());
        CloseHandle(hProcess);
        return result;
    }
    
    WriteProcessMemory(hProcess, remote_mem, dll_path, path_len, NULL);
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadlib = GetProcAddress(kernel32, "LoadLibraryA");
    
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadlib, remote_mem, 0, NULL);
    if (!hThread) {
        sprintf_s(result.msg, "CreateRemoteThread failed: %lu", GetLastError());
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return result;
    }
    
    WaitForSingleObject(hThread, 10000);
    
    DWORD exit_code = 0;
    GetExitCodeThread(hThread, &exit_code);
    
    VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    
    if (exit_code == 0) {
        sprintf_s(result.msg, "LoadLibrary returned 0");
        CloseHandle(hProcess);
        return result;
    }
    
    result.module_handle = (HMODULE)exit_code;
    result.success = true;
    sprintf_s(result.msg, "injected OK (PID: %lu, handle: 0x%p)", pid, (void*)exit_code);
    CloseHandle(hProcess);
    return result;
}

// ─── unload DLL ───────────────────────────────────────────────
InjectionResult unload_dll(const char* process_name, const char* dll_name) {
    InjectionResult result = { false, 0, NULL, "" };
    
    DWORD pid = find_process(process_name);
    if (!pid) {
        sprintf_s(result.msg, "process not found: %s", process_name);
        return result;
    }
    result.pid = pid;
    
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProcess) {
        sprintf_s(result.msg, "OpenProcess failed: %lu", GetLastError());
        return result;
    }
    
    // find module in target process
    HMODULE hMods[1024];
    DWORD cbNeeded;
    HMODULE target_mod = NULL;
    
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        int count = cbNeeded / sizeof(HMODULE);
        for (int i = 0; i < count; i++) {
            char mod_name[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hMods[i], mod_name, MAX_PATH)) {
                // extract filename from path
                const char* fname = strrchr(mod_name, '\\');
                if (!fname) fname = mod_name;
                else fname++;
                
                if (lstrcmpiA(fname, dll_name) == 0) {
                    target_mod = hMods[i];
                    break;
                }
            }
        }
    }
    
    if (!target_mod) {
        sprintf_s(result.msg, "module not found in target: %s", dll_name);
        CloseHandle(hProcess);
        return result;
    }
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC freelib = GetProcAddress(kernel32, "FreeLibrary");
    
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)freelib, target_mod, 0, NULL);
    if (!hThread) {
        sprintf_s(result.msg, "CreateRemoteThread(FreeLibrary) failed: %lu", GetLastError());
        CloseHandle(hProcess);
        return result;
    }
    
    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    
    result.success = true;
    sprintf_s(result.msg, "unloaded: %s (handle: 0x%p)", dll_name, (void*)target_mod);
    CloseHandle(hProcess);
    return result;
}
