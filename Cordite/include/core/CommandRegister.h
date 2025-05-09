#pragma once
#include "core/HttpClient.h"
#include <unordered_set>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class CommandRegister {
public:
    CommandRegister(HttpClient& httpClient);
    bool RegisterCommand(const std::unordered_set<int64_t>& agentIds);

private:
    HttpClient& httpClient;
};