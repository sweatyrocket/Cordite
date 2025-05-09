#pragma once
#include "nlohmann/json.hpp"
#include <vector>
#include <string>

using json = nlohmann::json;

class FileUtils {
public:
    FileUtils() = default;

    bool PrepareLocalFileUpload(const std::string& filePath, const std::string& agentId,
        std::string& fileContent, std::string& errorMessage);

    bool DownloadAndSaveFile(const std::string& channelMessagesJson, const std::string& destination, const std::string& agentId,
        std::string& fileUrl, std::string& messageId, std::string& errorMessage);

    bool SaveFileContent(const std::string& destination, const std::string& fileContent, std::string& errorMessage);

private:
    static const size_t MAX_FILE_SIZE = 10 * 1024 * 1024;
};