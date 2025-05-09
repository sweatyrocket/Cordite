#ifdef ENABLE_SCREENSHOT
#include "commands/screenshot.h"

bool CaptureScreenshot(const std::string& filePath) {
    HRESULT hr = S_OK;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 1,
        D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIOutput1> output1;
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IDXGIOutputDuplication> duplication;
    hr = output1->DuplicateOutput(device.get(), &duplication);
    if (FAILED(hr)) {
        return false;
    }

    DXGI_OUTDUPL_DESC desc;
    duplication->GetDesc(&desc);
    UINT width = desc.ModeDesc.Width;
    UINT height = desc.ModeDesc.Height;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ComPtr<IDXGIResource> desktopResource;
    hr = duplication->AcquireNextFrame(1000, &frameInfo, &desktopResource);
    if (FAILED(hr)) {
        return false;
    }

    if (frameInfo.TotalMetadataBufferSize == 0) {
        duplication->ReleaseFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        hr = duplication->AcquireNextFrame(1000, &frameInfo, &desktopResource);
        if (FAILED(hr) || frameInfo.TotalMetadataBufferSize == 0) {
            if (!FAILED(hr)) duplication->ReleaseFrame();
            return false;
        }
    }

    ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
    if (FAILED(hr)) {
        duplication->ReleaseFrame();
        return false;
    }

    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = width;
    stagingDesc.Height = height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.BindFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    hr = device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        duplication->ReleaseFrame();
        return false;
    }

    context->CopyResource(stagingTexture.get(), desktopTexture.get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = context->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        duplication->ReleaseFrame();
        return false;
    }

    GdiPlusInit gdiInit;
    Bitmap bitmap(width, height, mapped.RowPitch, PixelFormat32bppARGB, (BYTE*)mapped.pData);

    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    std::vector<BYTE> buffer(size);
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)buffer.data();
    GetImageEncoders(num, size, pImageCodecInfo);
    CLSID pngClsid;
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[j].Clsid;
            break;
        }
    }

    std::wstring wFilePath(filePath.begin(), filePath.end());
    Status stat = bitmap.Save(wFilePath.c_str(), &pngClsid, nullptr);
    if (stat != Ok) {
        context->Unmap(stagingTexture.get(), 0);
        duplication->ReleaseFrame();
        return false;
    }

    context->Unmap(stagingTexture.get(), 0);
    duplication->ReleaseFrame();

    return true;
}

#endif