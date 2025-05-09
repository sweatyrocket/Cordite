#ifdef ENABLE_CLIPBOARD
#include <windows.h>
#include <string>

std::string GetClipboardData() {
    std::string result;

    if (!OpenClipboard(nullptr)) {
        return result;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr) {
        CloseClipboard();
        return result;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText != nullptr) {
        result = std::string(pszText);
        GlobalUnlock(hData);
    }

    CloseClipboard();
    return result;
}
#endif