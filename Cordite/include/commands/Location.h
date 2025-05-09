#pragma once
#ifdef ENABLE_LOCATION
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <windows.h>
#include <winhttp.h>

namespace LocationUtils {
    bool GetLocationData(const std::string& ip, std::string& country, std::string& city, std::string& mapUrl, double& lat, double& lon);
}
#endif