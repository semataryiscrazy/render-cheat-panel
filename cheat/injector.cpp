#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

DWORD get_process_id(const std::string& name) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (std::string(pe.szExeFile) == name) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return pid;
}

bool inject_dll(DWORD pid, const std::string& dll_path) {
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!process) {
        std::cout << "erro: nao foi possivel abrir o processo (" << GetLastError() << ")\n";
        return false;
    }
    
    size_t path_len = dll_path.length() + 1;
    void* remote_mem = VirtualAllocEx(process, nullptr, path_len, MEM_COMMIT, PAGE_READWRITE);
    if (!remote_mem) {
        std::cout << "erro: virtualallocex (" << GetLastError() << ")\n";
        CloseHandle(process);
        return false;
    }
    
    WriteProcessMemory(process, remote_mem, dll_path.c_str(), path_len, nullptr);
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadlib = GetProcAddress(kernel32, "LoadLibraryA");
    
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, 
        (LPTHREAD_START_ROUTINE)loadlib, remote_mem, 0, nullptr);
    
    if (!thread) {
        std::cout << "erro: creatremotethread (" << GetLastError() << ")\n";
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }
    
    WaitForSingleObject(thread, INFINITE);
    VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(process);
    
    return true;
}

int main(int argc, char* argv[]) {
    std::string target_name;
    std::string dll_path = "cheat.dll";
    
    if (argc < 2) {
        std::cout << "uso: injector.exe <processo.exe> [dll_path]\n";
        std::cout << "ex:  injector.exe game.exe\n";
        std::cout << "ex:  injector.exe game.exe C:\\cheat.dll\n";
        return 1;
    }
    
    target_name = argv[1];
    if (argc >= 3) dll_path = argv[2];
    
    DWORD pid = get_process_id(target_name);
    if (!pid) {
        std::cout << "erro: processo \"" << target_name << "\" nao encontrado\n";
        return 1;
    }
    
    std::cout << "injetando em " << target_name << " (PID: " << pid << ")...\n";
    
    if (inject_dll(pid, dll_path)) {
        std::cout << "sucesso! painel em http://localhost:8080\n";
    } else {
        std::cout << "falha na injecao.\n";
        return 1;
    }
    
    return 0;
}
