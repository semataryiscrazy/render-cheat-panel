#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>

// ─── achar processos por nome (ex: "aow.exe" = Android no BlueStacks) ──
static DWORD find_process(const char* name) {
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

// ─── achar processos filho de um PID ──────────────────────────
static DWORD find_child_process(DWORD parent_pid, const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(snap, &pe)) do {
        if (pe.th32ParentProcessID == parent_pid &&
            lstrcmpiA(pe.szExeFile, name) == 0) {
            CloseHandle(snap);
            return pe.th32ProcessID;
        }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return 0;
}

// ─── ler memoria ─────────────────────────────────────────────
static bool read_memory(HANDLE hProcess, ULONG64 addr, void* buf, SIZE_T size) {
    SIZE_T bytes_read = 0;
    return ReadProcessMemory(hProcess, (LPCVOID)addr, buf, size, &bytes_read) && bytes_read == size;
}

// ─── escrever memoria ────────────────────────────────────────
static bool write_memory(HANDLE hProcess, ULONG64 addr, const void* buf, SIZE_T size) {
    SIZE_T bytes_written = 0;
    return WriteProcessMemory(hProcess, (LPVOID)addr, buf, size, &bytes_written) && bytes_written == size;
}

// ─── achar biblioteca/modulo num processo ─────────────────────
static ULONG64 get_module_base(DWORD pid, const char* module_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
    if (Module32First(snap, &me)) do {
        if (lstrcmpiA(me.szModule, module_name) == 0) {
            CloseHandle(snap);
            return (ULONG64)me.modBaseAddr;
        }
    } while (Module32Next(snap, &me));
    CloseHandle(snap);
    return 0;
}
