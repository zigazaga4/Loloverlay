#include <windows.h>
#include <detours.h>
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include <gdiplus.h>
#include <iostream>

#pragma comment(lib, "gdiplus.lib")

// Function pointer for the original BitBlt function
typedef BOOL(WINAPI* BitBlt_t)(HDC, int, int, int, int, HDC, int, int, DWORD);
BitBlt_t Real_BitBlt = BitBlt;

std::vector<std::wstring> imageFiles;

// Initialize GDI+
ULONG_PTR gdiplusToken;
void InitializeGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
        std::cerr << "GDI+ initialization failed" << std::endl;
        exit(1);
    }
}

// Load all image file names from the specified directory
void LoadImageFiles(const std::wstring& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && (entry.path().extension() == L".jpg" || entry.path().extension() == L".jpeg")) {
                imageFiles.push_back(entry.path().wstring());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load image files: " << e.what() << std::endl;
    }
}

// Convert std::wstring to std::string using Windows API
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

// Get a random image from the list
std::wstring GetRandomImage() {
    if (imageFiles.empty()) {
        std::cerr << "No images found in the directory" << std::endl;
        return L"";
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(imageFiles.size() - 1));
    return imageFiles[dis(gen)];
}

// Load an image and draw it onto the HDC
BOOL DrawRandomImage(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight) {
    std::wstring imagePath = GetRandomImage();

    if (imagePath.empty()) {
        std::cerr << "Image path is empty" << std::endl;
        return FALSE;
    }

    Gdiplus::Bitmap bitmap(imagePath.c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        std::cerr << "Failed to load image: " << WStringToString(imagePath) << std::endl;
        return FALSE;
    }

    Gdiplus::Graphics graphics(hdcDest);
    Gdiplus::Rect destRect(nXDest, nYDest, nWidth, nHeight);
    if (graphics.DrawImage(&bitmap, destRect) != Gdiplus::Ok) {
        std::cerr << "Failed to draw image" << std::endl;
        return FALSE;
    }

    return TRUE;
}

// Our hooked BitBlt function
BOOL WINAPI Hooked_BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
    HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop) {
    // Check the calling process
    DWORD processID = GetCurrentProcessId();
    char processName[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, processName, MAX_PATH);

    // List of authorized processes (add your authorized process names here)
    const char* authorizedProcesses[] = { "authorized_app.exe", "another_authorized_app.exe" };

    bool authorized = false;
    for (const auto& authProcess : authorizedProcesses) {
        if (strstr(processName, authProcess) != NULL) {
            authorized = true;
            break;
        }
    }

    if (!authorized) {
        return DrawRandomImage(hdcDest, nXDest, nYDest, nWidth, nHeight);
    }

    // Call the original BitBlt function
    return Real_BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
}

extern "C" __declspec(dllexport) void InitHooks(HMODULE hModule) {
    // Initialize GDI+
    InitializeGDIPlus();

    // Get the directory where the DLL is located
    wchar_t dllPath[MAX_PATH];
    GetModuleFileName(hModule, dllPath, MAX_PATH);
    std::filesystem::path path(dllPath);
    std::wstring directory = path.parent_path().wstring() + L"\\images";

    // Load image files from the images directory where the DLL is located
    LoadImageFiles(directory);

    // Attach the hook
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)Real_BitBlt, Hooked_BitBlt);
    DetourTransactionCommit();
}

extern "C" __declspec(dllexport) void RemoveHooks() {
    // Remove the hook
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)Real_BitBlt, Hooked_BitBlt);
    DetourTransactionCommit();

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        InitHooks(hModule);
        break;
    case DLL_PROCESS_DETACH:
        RemoveHooks();
        break;
    }
    return TRUE;
}