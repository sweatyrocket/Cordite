#pragma once
#include "core/WebSocketClient.h"
#include "core/CommandRegister.h"
#include "agents/Callback_utils.h"
#include "commands/CommandHandler.h"
#include "commands/FileUtils.h"
#include "core/Config.h"

using json = nlohmann::json;

class EventHandler;

class DiscordClient {
public:
    DiscordClient(const config::BotConfig& cfg);
    void Start();
    bool RegisterCommands(const std::unordered_set<int64_t>& agentIds);
    ~DiscordClient();

private:
    config::BotConfig cfg_;
    HttpClient httpClient;
    WebSocketClient wsClient;
    CommandHandler cmdHandler;
    FileUtils fileUtils;
    CommandRegister commandRegister;
    EventHandler* eventHandler;
    bool running = true;
    std::string commands_channel_id;
};