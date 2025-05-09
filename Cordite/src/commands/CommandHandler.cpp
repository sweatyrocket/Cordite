#include "commands/CommandHandler.h"

void CommandHandler::RegisterCommand(const Command& cmd) {
    commands[cmd.name] = cmd;
}

void CommandHandler::HandleInteraction(const json& interaction,
    std::function<bool(const std::string&, const std::string&)> defer,
    std::function<bool(const std::string&, const std::string&)> followUp) {
    std::string commandName = interaction["d"]["data"]["name"].get<std::string>();
    std::string id = interaction["d"]["id"].get<std::string>();
    std::string token = interaction["d"]["token"].get<std::string>();

    auto it = commands.find(commandName);
    if (it != commands.end()) {
        std::cout << "Handling command: " << commandName << std::endl;
        it->second.execute(interaction, id, token);
    }
    else {
        std::cerr << "Unknown command: " << commandName << std::endl;
    }
}

json CommandHandler::GetCommandJson(const std::string& name) const {
    auto it = commands.find(name);
    if (it != commands.end()) {
        return {
            {"name", it->second.name},
            {"type", 1},
            {"description", it->second.description},
            {"options", it->second.options}
        };
    }
    return {};
}