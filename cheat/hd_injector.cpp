#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

const char* TARGET_PROCESS = "HD-Player.exe";
const char* DLL_NAME = "Client.dll";

bool enable_debug_privilege() {
    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;
    
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!LookupPrivilegeValueA(nullptr, "SeDebugPrivilege", &luid)) {
        CloseHandle(token);
        return false;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    bool result = AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(token);
    return result && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

std::vector<DWORD> find_process_ids(const std::string& name) {
    std::vector<DWORD> pids;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return pids;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name.c_str()) == 0) {
                pids.push_back(pe.th32ProcessID);
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return pids;
}

bool is_process_64_bit(HANDLE process) {
    BOOL is_wow64 = FALSE;
    IsWow64Process(process, &is_wow64);
    
    // se rodando em sistema 64-bit:
    // - se IsWow64Process retorna TRUE -> processo é 32-bit
    // - se IsWow64Process retorna FALSE -> processo é 64-bit
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || 
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        return !is_wow64; // true se for 64-bit nativo
    }
    
    // sistema 32-bit so roda processo 32-bit
    return false;
}

std::string get_dll_path() {
    // o injetor e a dll estao no mesmo diretorio (cheat/)
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    
    // tira o nome do exe
    char* last_slash = strrchr(path, '\\');
    if (last_slash) *(last_slash + 1) = '\0';
    
    // path = diretorio do exe, onde a dll tambem esta
    return std::string(path) + DLL_NAME;
}

bool inject_dll(DWORD pid, const std::string& dll_path) {
    HANDLE process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid
    );
    
    if (!process) {
        DWORD err = GetLastError();
        std::cout << "  [!] erro ao abrir processo (PID: " << pid << "): " << err << "\n";
        if (err == ERROR_ACCESS_DENIED) {
            std::cout << "      -> acesso negado. tente executar como administrador.\n";
        }
        return false;
    }
    
    // verifica arquitetura
    bool target_is_64 = is_process_64_bit(process);
    #ifdef _WIN64
    bool injector_is_64 = true;
    #else
    bool injector_is_64 = false;
    #endif
    
    if (target_is_64 != injector_is_64) {
        std::cout << "  [!] incompatibilidade de arquitetura:\n";
        std::cout << "      injetor: " << (injector_is_64 ? "64-bit" : "32-bit") << "\n";
        std::cout << "      alvo: " << (target_is_64 ? "64-bit" : "32-bit") << "\n";
        std::cout << "      use a versao correta do injetor.\n";
        CloseHandle(process);
        return false;
    }
    
    size_t path_len = dll_path.length() + 1;
    void* remote_mem = VirtualAllocEx(process, nullptr, path_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        std::cout << "  [!] virtualallocex falhou: " << GetLastError() << "\n";
        CloseHandle(process);
        return false;
    }
    
    if (!WriteProcessMemory(process, remote_mem, dll_path.c_str(), path_len, nullptr)) {
        std::cout << "  [!] writeprocessmemory falhou: " << GetLastError() << "\n";
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }
    
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadlib = GetProcAddress(kernel32, "LoadLibraryA");
    
    HANDLE thread = CreateRemoteThread(process, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadlib, remote_mem, 0, nullptr);
    
    if (!thread) {
        std::cout << "  [!] creatremotethread falhou: " << GetLastError() << "\n";
        VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }
    
    std::cout << "  [+] thread remota criada. aguardando...\n";
    
    WaitForSingleObject(thread, 5000);
    
    DWORD exit_code = 0;
    GetExitCodeThread(thread, &exit_code);
    
    VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(process);
    
    std::cout << "  [+] dll injetada. LoadLibrary retornou: 0x" << std::hex << exit_code << std::dec << "\n";
    return exit_code != 0;
}

int main() {
    SetConsoleTitleA("HD-Player Injector");
    
    std::cout << "============================================\n";
    std::cout << "    HD-Player Injector - Remote Panel\n";
    std::cout << "============================================\n\n";
    
    // tenta privilegio de debug
    if (enable_debug_privilege()) {
        std::cout << "[+] SeDebugPrivilege ativado.\n";
    } else {
        std::cout << "[-] SeDebugPrivilege nao disponivel.\n";
        std::cout << "    (pode funcionar mesmo sem, mas recomendado executar como admin)\n";
    }
    
    // descobre o caminho da dll
    std::string dll_path = get_dll_path();
    std::cout << "[+] dll: " << dll_path << "\n\n";
    
    if (!PathFileExistsA(dll_path.c_str())) {
        std::cout << "[!] dll nao encontrada em: " << dll_path << "\n";
        std::cout << "    certifique-se de que " << DLL_NAME << " esta no mesmo diretorio.\n";
        system("pause");
        return 1;
    }
    
    // procura o processo
    std::cout << "[+] procurando processo: " << TARGET_PROCESS << "\n";
    
    auto pids = find_process_ids(TARGET_PROCESS);
    if (pids.empty()) {
        std::cout << "[!] processo '" << TARGET_PROCESS << "' nao encontrado.\n";
        std::cout << "    certifique-se de que o HD-Player esta rodando.\n";
        std::cout << "    tentando novamente em 3 segundos...\n";
        Sleep(3000);
        pids = find_process_ids(TARGET_PROCESS);
        if (pids.empty()) {
            std::cout << "[+] ainda nao encontrado. abrindo navegador...\n";
            system("start http://localhost:8080");
            system("pause");
            return 1;
        }
    }
    
    std::cout << "[+] encontrado(s) " << pids.size() << " processo(s):\n";
    
    int success_count = 0;
    for (size_t i = 0; i < pids.size(); i++) {
        std::cout << "\n[" << (i + 1) << "/" << pids.size() << "] injetando em PID: " << pids[i] << "\n";
        if (inject_dll(pids[i], dll_path)) {
            success_count++;
        }
    }
    
    std::cout << "\n============================================\n";
    if (success_count > 0) {
        std::cout << "[+] sucesso! " << success_count << "/" << pids.size() << " injetado(s).\n";
    } else {
        std::cout << "[!] falha ao injetar em todos os processos.\n";
        std::cout << "    (tentando abrir navegador mesmo assim...)\n";
    }
    
    std::cout << "[+] abrindo painel web...\n";
    std::cout << "    (se nao abrir automaticamente, acesse http://localhost:8080)\n";
    
    system("start http://localhost:8080");
    
    std::cout << "[+] painel web: http://localhost:8080\n";
    std::cout << "    (ou http://SEU_IP:8080 para outros dispositivos)\n";
    std::cout << "============================================\n";
    std::cout << "[+] janela fechara automaticamente em 5 segundos...\n";
    Sleep(5000);
    return success_count > 0 ? 0 : 1;
}
