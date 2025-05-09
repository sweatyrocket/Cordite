#pragma once
#ifdef ENABLE_APPLICATIONS
#include <string>
#include <unordered_map>

std::string CreateJSONPayload(const std::unordered_map<std::string, std::string>& app_results, const std::string& agentIdStr, bool success);
std::unordered_map<std::string, std::string> GetInstalledApplications(const std::string& user_profile);
std::string GetUserProfile();
#endif