#pragma once
#include "core/Config.h"
#include <windows.h>
#include <unordered_set>
#include <winhttp.h>
#include <sstream>

std::string GetHostname();
std::string getPublicIP();
std::string GetWindowsVersion();
bool IsAdmin();
std::string GetUsername();
std::string GetUniversalTime();
int64_t generate_agent_id();
std::unordered_set<int64_t> get_unique_agent_ids(const config::BotConfig& cfg);