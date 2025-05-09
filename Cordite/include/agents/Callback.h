#pragma once
#include <mutex>
#include <windows.h>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct SendMessageResult {
    bool success;
    DWORD httpStatus;
    DWORD retryAfter;
};

std::string CreateJSONPayload(int agentId, const std::string& hostname, const std::string& ip,
    const std::string& lastCheckIn, int color, bool isInitializing,
    int currentPercentage,
    const std::string& statusMsg,
    const std::string& connectionTime);

std::vector<std::pair<std::string, std::string>> GetMessagesFromChannel(const std::string& botToken, const std::string& channelId);
SendMessageResult SendDiscordMessage(const std::string& botToken, const std::string& channelId, const std::string& jsonPayload, std::string& messageId);
bool DeleteDiscordMessage(const std::string& botToken, const std::string& channelId, const std::string& messageId);

extern std::map<std::string, std::string> hostnameMessageIds;
extern std::vector<int64_t> AGENT_IDS;
extern std::mutex agent_ids_mutex;

void UpdateTime(const std::string& botToken, const std::string& channelId, int agentId, const std::string& hostname, const std::string& ip);