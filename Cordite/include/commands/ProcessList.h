#pragma once
#ifdef ENABLE_PROCESSES
#include <string>
#include <unordered_map>
#include <windows.h>

std::unordered_map<std::string, std::string> GetRunningProcesses(bool appsOnly = false);
void SendNewFollowUp(const std::string& token, const std::string& payload);
#endif