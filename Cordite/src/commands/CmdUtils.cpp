#include "commands/CmdUtils.h"
#include <windows.h>
#include <sstream>
#include <array>
#include <algorithm>
#include <TlHelp32.h>

std::string ExecuteCommand(const std::string& cmd) {
    std::string result;
    FILE* pipe = _popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) return "Error executing command";

    char buffer[128];
    std::string rawOutput;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        rawOutput += buffer;
    }
    _pclose(pipe);
    if (rawOutput.empty()) return "No output";
    int wideSize = MultiByteToWideChar(CP_OEMCP, 0, rawOutput.c_str(), -1, nullptr, 0);
    if (wideSize == 0) return "Error converting output to wide chars";
    std::wstring wideOutput(wideSize, L'\0');
    MultiByteToWideChar(CP_OEMCP, 0, rawOutput.c_str(), -1, &wideOutput[0], wideSize);
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideOutput.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0) return "Error converting output to UTF-8";
    std::string utf8Output(utf8Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wideOutput.c_str(), -1, &utf8Output[0], utf8Size, nullptr, nullptr);
    if (!utf8Output.empty() && utf8Output.back() == '\0') {
        utf8Output.pop_back();
    }

    return utf8Output;
}

std::vector<std::string> SplitAndSanitizeOutput(const std::string& output, size_t maxLength, size_t maxMessages) {
    std::vector<std::string> messages;
    if (output.length() <= maxLength) {
        messages.push_back(output);
        return messages;
    }
    std::string currentChunk;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line) && messages.size() < maxMessages) {
        line += "\n";

        if (currentChunk.length() + line.length() > maxLength) {
            if (!currentChunk.empty()) {
                messages.push_back(currentChunk);
                currentChunk.clear();
            }
            while (line.length() > maxLength && messages.size() < maxMessages) {
                messages.push_back(line.substr(0, maxLength));
                line = line.substr(maxLength);
            }
            if (messages.size() < maxMessages) {
                currentChunk = line;
            }
        }
        else {
            currentChunk += line;
        }
    }
    if (!currentChunk.empty() && messages.size() < maxMessages) {
        messages.push_back(currentChunk);
    }

    if (stream.rdbuf()->in_avail() > 0 || line.length() > maxLength) {
        if (!messages.empty()) {
            messages.back() = messages.back().substr(0, maxLength - 50) + "\n[Output truncated; too long]";
        }
    }

    return messages;
}

#ifdef ENABLE_POWERSHELL
bool CmdUtils::ExecutePowerShellCommand(const std::string& command, std::string& output, std::string& error) {
    std::string psCommand = "powershell -NoProfile -ExecutionPolicy Bypass -Command \"" + command + "\" 2>&1";

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(psCommand.c_str(), "r"), _pclose);

    if (!pipe) {
        error = "Failed to open pipe for PowerShell command";
        return false;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    int exitCode = _pclose(pipe.release());
    pipe.reset();

    if (exitCode != 0 && result.empty()) {
        error = "PowerShell command failed with exit code: " + std::to_string(exitCode);
        return false;
    }

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    output = result;
    return true;
}
#endif

#ifdef ENABLE_KILL
namespace {
    std::string WideCharToString(const WCHAR* wideStr) {
        if (!wideStr) return "";

        int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return "";

        std::string narrowStr(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &narrowStr[0], size, nullptr, nullptr);

        if (!narrowStr.empty() && narrowStr.back() == '\0') {
            narrowStr.pop_back();
        }

        return narrowStr;
    }
}

bool CmdUtils::KillProcess(const std::string& processName, std::string& error) {
    std::string targetName = processName;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        error = "Failed to create process snapshot: " + std::to_string(GetLastError());
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    bool found = false;

    if (Process32First(snapshot, &pe32)) {
        do {
            std::string currentName = WideCharToString(pe32.szExeFile);
            std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower);

            if (currentName == targetName) {
                HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (process == NULL) {
                    error = "Failed to open process " + WideCharToString(pe32.szExeFile) + " (PID: " + std::to_string(pe32.th32ProcessID) + "): " + std::to_string(GetLastError());
                    continue;
                }

                if (!TerminateProcess(process, 1)) {
                    error = "Failed to terminate process " + WideCharToString(pe32.szExeFile) + " (PID: " + std::to_string(pe32.th32ProcessID) + "): " + std::to_string(GetLastError());
                    CloseHandle(process);
                    continue;
                }

                found = true;
                CloseHandle(process);
            }
        } while (Process32Next(snapshot, &pe32));
    }
    else {
        error = "Failed to enumerate processes: " + std::to_string(GetLastError());
    }

    CloseHandle(snapshot);

    if (!found) {
        error = "No process named '" + processName + "' was found";
        return false;
    }

    return true;
}
#endif