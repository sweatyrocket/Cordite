#pragma once
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <windows.h>
#include <winhttp.h>

class HttpClient {
public:
    HttpClient(const std::wstring& userAgent = L"Discord Client/1.0");
    ~HttpClient();
    bool SendRequest(const std::wstring& url, const std::string& data, const std::string& botToken, const std::string& method,
        const std::string& contentType = "application/json", std::string* response = nullptr,
        DWORD* statusCodeOut = nullptr, DWORD* retryAfterOut = nullptr);

private:
    HINTERNET hSession;
};