#ifdef ENABLE_LOCATION
#define _USE_MATH_DEFINES
#include <cmath>
#include "commands/Location.h"
#include <iostream>
#include "nlohmann/json.hpp"

namespace {
    std::wstring StringToWideString(const std::string& str) {
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (size <= 0) return L"";
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
        return wstr;
    }

    std::pair<int, int> LatLonToTile(double lat, double lon, int zoom) {
        double latRad = lat * M_PI / 180.0;
        int n = 1 << zoom;
        int xTile = static_cast<int>((lon + 180.0) / 360.0 * n);
        int yTile = static_cast<int>((1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * n);
        return { xTile, yTile };
    }

    bool HttpGet(const std::string& url, std::string& response) {
        HINTERNET hSession = WinHttpOpen(L"LocationClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            std::cout << "WinHttpOpen failed: " << GetLastError() << std::endl;
            return false;
        }

        std::wstring wideUrl = StringToWideString(url);
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwSchemeLength = (DWORD)-1;
        urlComp.dwHostNameLength = (DWORD)-1;
        urlComp.dwUrlPathLength = (DWORD)-1;

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp)) {
            std::cout << "WinHttpCrackUrl failed: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
        std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

        HINTERNET hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
        if (!hConnect) {
            std::cout << "WinHttpConnect failed: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            std::cout << "WinHttpOpenRequest failed: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            std::cout << "WinHttpSendRequest failed: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            std::cout << "WinHttpReceiveResponse failed: " << GetLastError() << std::endl;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::string responseData;
        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseData.append(buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (responseData.empty()) {
            std::cout << "No data received from request" << std::endl;
            return false;
        }

        response = responseData;
        return true;
    }
}

namespace LocationUtils {
    bool LocationUtils::GetLocationData(const std::string& ip, std::string& country, std::string& city, std::string& mapUrl, double& lat, double& lon) {
        std::string url = "http://ip-api.com/json/" + ip + "?fields=country,city,lat,lon";
        std::string response;

        if (!HttpGet(url, response)) {
            std::cout << "Failed to fetch location data from ip-api.com" << std::endl;
            return false;
        }

        try {
            auto jsonResponse = nlohmann::json::parse(response);

            if (jsonResponse.contains("status") && jsonResponse["status"] == "fail") {
                std::cout << "ip-api.com returned failure: " << jsonResponse["message"].get<std::string>() << std::endl;
                return false;
            }

            country = jsonResponse.value("country", "Unknown");
            city = jsonResponse.value("city", "Unknown");
            lat = jsonResponse.value("lat", 0.0);
            lon = jsonResponse.value("lon", 0.0);

            int zoom = 8;
            auto [xTile, yTile] = LatLonToTile(lat, lon, zoom);
            mapUrl = "https://tile.openstreetmap.org/" + std::to_string(zoom) + "/" +
                std::to_string(xTile) + "/" + std::to_string(yTile) + ".png";

            return true;
        }
        catch (const std::exception& e) {
            std::cout << "Exception in GetLocationData: " << e.what() << std::endl;
            return false;
        }
    }
}
#endif