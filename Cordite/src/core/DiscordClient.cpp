#include "core/DiscordClient.h"
#include "core/EventHandler.h"
#include <thread>
#include "core/InitializationStatus.h"

DiscordClient::DiscordClient(const config::BotConfig& cfg)
    : cfg_(cfg),
    httpClient(),
    wsClient(),
    cmdHandler(),
    fileUtils(),
    commandRegister(httpClient),
    eventHandler(nullptr),
    running(true),
    commands_channel_id(cfg.commands_chanel_id)
{
    eventHandler = new EventHandler(
        *this,
        cfg_,
        httpClient,
        wsClient,
        cmdHandler,
        fileUtils,
        cfg_.commands_chanel_id
    );
}

void DiscordClient::Start() {
    while (running) {
        if (wsClient.Connect("gateway.discord.gg", "443", "/?v=10&encoding=json")) {
            std::cout << "Bot connected. Local agent ID: " << cfg_.agent_id << ". Use slash commands in Discord..." << std::endl;
            if (eventHandler) {
                eventHandler->ProcessEvents(running);
            }
            wsClient.Disconnect();
            std::cout << "Disconnected. Reconnecting in 2 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        else {
            std::cout << "Failed to connect. Retrying in 2 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

bool DiscordClient::RegisterCommands(const std::unordered_set<int64_t>& agentIds) {
    return commandRegister.RegisterCommand(agentIds);
}

DiscordClient::~DiscordClient() {
    delete eventHandler;
}