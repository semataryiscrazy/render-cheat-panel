#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>

DWORD get_pid(const char* process_name) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(snapshot, &pe)) {
        do {
            if (lstrcmpiA(pe.szExeFile, process_name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe));
    }
    CloseHandle(snapshot);
    return pid;
}

bool inject_dll(DWORD pid, const std::string& dll_path) {
    HANDLE h_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!h_process) {
        std::cout << "[-] openprocess failed\n";
        return false;
    }

    size_t path_size = dll_path.size() + 1;
    void* remote_addr = VirtualAllocEx(h_process, NULL, path_size,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_addr) {
        std::cout << "[-] virtualallocex failed\n";
        CloseHandle(h_process);
        return false;
    }

    WriteProcessMemory(h_process, remote_addr, dll_path.c_str(), path_size, NULL);

    HANDLE h_thread = CreateRemoteThread(h_process, NULL, 0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA, remote_addr, 0, NULL);
    if (!h_thread) {
        std::cout << "[-] creatremotethread failed\n";
        VirtualFreeEx(h_process, remote_addr, 0, MEM_RELEASE);
        CloseHandle(h_process);
        return false;
    }

    WaitForSingleObject(h_thread, INFINITE);
    VirtualFreeEx(h_process, remote_addr, 0, MEM_RELEASE);
    CloseHandle(h_thread);
    CloseHandle(h_process);
    return true;
}
