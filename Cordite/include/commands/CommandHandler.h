#pragma once
#include "nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

class CommandHandler {
public:
    struct Command {
        std::string name;
        std::string description;
        json options;
        std::function<void(const json&, const std::string&, const std::string&)> execute;
    };

    void RegisterCommand(const Command& cmd);
    void HandleInteraction(const json& interaction,
        std::function<bool(const std::string&, const std::string&)> defer,
        std::function<bool(const std::string&, const std::string&)> followUp);
    json GetCommandJson(const std::string& name) const;

private:
    std::map<std::string, Command> commands;
};