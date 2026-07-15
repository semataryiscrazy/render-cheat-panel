#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>

#pragma comment(lib, "ws2_32.lib")

class WebServer {
private:
    SOCKET server_socket;
    int port;
    std::atomic<bool> running;
    std::thread server_thread;

    std::string handle_request(const std::string& method, const std::string& path, const std::string& body);
    std::string serve_frontend();
    std::string serve_options_json();
    std::string serve_update_option(const std::string& body);
public:
    WebServer(int port = 8080);
    ~WebServer();
    bool start();
    void stop();
    static void server_loop(WebServer* server);
};
