#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

struct Response {
    int code;
    std::string content_type;
    std::string body;
};

std::string read_file_bin(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string get_mime_type(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css")  != std::string::npos) return "text/css";
    if (path.find(".js")   != std::string::npos) return "application/javascript";
    if (path.find(".png")  != std::string::npos) return "image/png";
    if (path.find(".ico")  != std::string::npos) return "image/x-icon";
    if (path.find(".json") != std::string::npos) return "application/json";
    return "text/plain";
}

std::string build_http_response(int code, const std::string& content_type, const std::string& body) {
    std::stringstream ss;
    ss << "HTTP/1.1 " << code << " OK\r\n"
       << "Content-Type: " << content_type << "\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "Connection: close\r\n\r\n"
       << body;
    return ss.str();
}

std::string url_decode(const std::string& s) {
    std::string res;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int v;
            sscanf(s.substr(i + 1, 2).c_str(), "%x", &v);
            res += static_cast<char>(v);
            i += 2;
        } else if (s[i] == '+') {
            res += ' ';
        } else {
            res += s[i];
        }
    }
    return res;
}

void parse_request(const std::string& raw, std::string& method, std::string& path, std::string& body) {
    if (raw.size() < 4) return;
    
    size_t p1 = raw.find(' ');
    if (p1 == std::string::npos) return;
    
    size_t p2 = raw.find(' ', p1 + 1);
    if (p2 == std::string::npos) return;
    
    size_t p3 = raw.find("\r\n\r\n");
    if (p3 == std::string::npos) return;
    
    method = raw.substr(0, p1);
    path   = raw.substr(p1 + 1, p2 - p1 - 1);
    body   = raw.substr(p3 + 4);
}
