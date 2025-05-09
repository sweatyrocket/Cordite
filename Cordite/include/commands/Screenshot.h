#pragma once
#ifdef ENABLE_SCREENSHOT
#include <fstream>
#include "nlohmann/json.hpp"
#include <thread>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <gdiplus.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "gdiplus.lib")

using json = nlohmann::json;
using namespace Gdiplus;

class GdiPlusInit {
public:
    GdiPlusInit() { GdiplusStartup(&token, &startupInput, nullptr); }
    ~GdiPlusInit() { GdiplusShutdown(token); }
private:
    GdiplusStartupInput startupInput;
    ULONG_PTR token;
};

template<typename T>
class ComPtr {
public:
    ComPtr() : ptr(nullptr) {}
    ~ComPtr() { if (ptr) ptr->Release(); }
    T** operator&() { return &ptr; }
    T* operator->() { return ptr; }
    T* get() { return ptr; }
private:
    T* ptr;
};

bool CaptureScreenshot(const std::string& filePath);

#endif