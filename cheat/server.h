#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

class HttpServer {
private:
    SOCKET server_socket;
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    
    std::string handle_request(const std::string& method, const std::string& path);
    std::string serve_webui();
    std::string serve_status();
    std::string serve_toggle(const std::string& feature);
    
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    bool start();
    void stop();
    static void server_loop(HttpServer* server);
};
