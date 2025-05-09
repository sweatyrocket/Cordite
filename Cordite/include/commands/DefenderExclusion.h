#pragma once
#ifdef ENABLE_DEFENDER_EXCLUSION
#include <string>

namespace commands {
    bool AddDefenderExclusion(const std::string& path, std::string& statusContents);
}
#endif