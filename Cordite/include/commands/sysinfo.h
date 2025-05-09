#ifdef ENABLE_SYSINFO
#pragma once
#include "nlohmann/json.hpp"
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

using json = nlohmann::json;

typedef LONG NTSTATUS;
#define UNLEN 256

typedef NTSTATUS(NTAPI* RtlGetVersionPtr)(OSVERSIONINFOEXW*);
extern RtlGetVersionPtr RtlGetVersion;

std::string CreateJSONPayload2(const std::unordered_map<std::string, std::string>& sys_info, const std::string& agentIdStr, bool success);
std::unordered_map<std::string, std::string> GetSystemInfo();
std::string GetAudioDevices();
#endif