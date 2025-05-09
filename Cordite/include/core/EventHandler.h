#pragma once
#include "core/HttpClient.h"
#include "core/WebSocketClient.h"
#include "core/DiscordClient.h"
#include "core/Config.h"
#include "commands/CommandHandler.h"
#include "commands/FileUtils.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class EventHandler {
public:
    EventHandler(
        DiscordClient& discordClient,
        const config::BotConfig& cfg,
        HttpClient& httpClient,
        WebSocketClient& wsClient,
        CommandHandler& cmdHandler,
        FileUtils& fileUtils,
        const std::string& commandsChannelId
    );
    void ProcessEvents(bool& running);

private:
    DiscordClient& discordClient_;
    const config::BotConfig& cfg_;
    HttpClient& httpClient;
    WebSocketClient& wsClient;
    CommandHandler& cmdHandler;
    FileUtils& fileUtils;
    std::string commands_channel_id;
    int heartbeatIntervalMs;

    void SendHeartbeat(bool& running);
    bool SendFollowUp(const std::string& token, const std::string& content);
    bool RespondDeferred(const std::string& id, const std::string& token);
    bool SendChannelMessage(
        const std::string& channel_id,
        const std::string& content,
        const std::string& filePath = "",
        const std::string& fileContent = ""
    );
};