#include "core/HttpClient.h"
#include <iostream>

HttpClient::HttpClient(const std::wstring& userAgent) {
    hSession = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "WinHttpOpen failed: " << GetLastError() << std::endl;
    }
}

HttpClient::~HttpClient() {
    if (hSession) WinHttpCloseHandle(hSession);
}

bool HttpClient::SendRequest(const std::wstring& url, const std::string& data, const std::string& botToken, const std::string& method,
    const std::string& contentType, std::string* response, DWORD* statusCodeOut, DWORD* retryAfterOut) {
    if (!hSession) return false;

    URL_COMPONENTS urlComp = { 0 };
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256], urlPath[1024];
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        std::cerr << "WinHttpCrackUrl failed: " << GetLastError() << std::endl;
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        std::cerr << "WinHttpConnect failed: " << GetLastError() << std::endl;
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, std::wstring(method.begin(), method.end()).c_str(), urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        std::cerr << "WinHttpOpenRequest failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hConnect);
        return false;
    }

    std::wstring headers = L"Content-Type: " + std::wstring(contentType.begin(), contentType.end());
    if (!botToken.empty()) {
        headers += L"\r\nAuthorization: Bot " + std::wstring(botToken.begin(), botToken.end());
    }
    if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) {
        std::cerr << "WinHttpAddRequestHeaders failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    LPVOID requestData = data.empty() ? NULL : (LPVOID)data.c_str();
    DWORD dataLength = data.empty() ? 0 : data.length();
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestData, dataLength, dataLength, 0)) {
        std::cerr << "WinHttpSendRequest failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        std::cerr << "WinHttpReceiveResponse failed: " << GetLastError() << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return false;
    }

    bool success = false;
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL);
    if (statusCodeOut) *statusCodeOut = statusCode;

    if (statusCode == 429 && retryAfterOut) {
        DWORD retryAfter = 0;
        DWORD retryAfterSize = sizeof(retryAfter);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_NUMBER, L"Retry-After", &retryAfter, &retryAfterSize, NULL)) {
            *retryAfterOut = retryAfter;
        }
        else {
            *retryAfterOut = 5;
        }
    }

    if (response) {
        std::string responseBody;
        DWORD bytesAvailable, bytesRead;
        BYTE buffer[4096];
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            if (WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead)) {
                responseBody.append(reinterpret_cast<char*>(buffer), bytesRead);
            }
        }
        *response = responseBody;
    }

    success = (statusCode >= 200 && statusCode < 300);
    if (!success) {
        std::cerr << "Request failed with status code: " << statusCode << std::endl;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    return success;
}