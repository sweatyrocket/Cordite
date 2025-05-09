#pragma once
#include <string>
#include <vector>

std::string ExecuteCommand(const std::string& cmd);
std::vector<std::string> SplitAndSanitizeOutput(const std::string& output, size_t maxLength, size_t maxMessages);

namespace CmdUtils {
#ifdef ENABLE_POWERSHELL
    bool ExecutePowerShellCommand(const std::string& command, std::string& output, std::string& error);
#endif
#ifdef ENABLE_KILL
    bool KillProcess(const std::string& processName, std::string& error);
#endif
}
