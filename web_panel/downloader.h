#pragma once
#include <windows.h>
#include <wininet.h>
#include <string>
#include <fstream>
#include <iostream>

#pragma comment(lib, "wininet.lib")

bool download_file(const std::string& url, const std::string& output_path) {
    HINTERNET h_inet = InternetOpenA("WebPanel", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!h_inet) {
        std::cout << "[-] internetopen failed\n";
        return false;
    }

    HINTERNET h_url = InternetOpenUrlA(h_inet, url.c_str(), NULL, 0,
                                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!h_url) {
        std::cout << "[-] internetopenurl failed\n";
        InternetCloseHandle(h_inet);
        return false;
    }

    char buffer[8192];
    DWORD bytes_read;
    std::ofstream out(output_path, std::ios::binary);

    if (!out.is_open()) {
        std::cout << "[-] failed to create output file\n";
        InternetCloseHandle(h_url);
        InternetCloseHandle(h_inet);
        return false;
    }

    while (InternetReadFile(h_url, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
        out.write(buffer, bytes_read);
    }

    out.close();
    InternetCloseHandle(h_url);
    InternetCloseHandle(h_inet);
    std::cout << "[+] download completo: " << output_path << "\n";
    return true;
}
