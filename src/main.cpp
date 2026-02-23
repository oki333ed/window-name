#include <Geode/Geode.hpp>
#include <Geode/modify/AppDelegate.hpp>
#include <filesystem>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <objc/objc.h>
    #include <objc/message.h>
    #include <CoreFoundation/CoreFoundation.h>
#endif

using namespace geode::prelude;

inline bool fileExists(const std::filesystem::path& path) {
#if defined(_WIN32)
    std::wstring wpath = path.wstring();
    DWORD attrs = GetFileAttributesW(wpath.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    return std::filesystem::exists(path);
#endif
}

inline void copyFile(const std::filesystem::path& from, const std::filesystem::path& to) {
#if defined(_WIN32)
    std::wstring wfrom = from.wstring();
    std::wstring wto = to.wstring();
    (void)CopyFileW(wfrom.c_str(), wto.c_str(), FALSE);
#else
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
#endif
}

inline std::filesystem::path makePath(const std::filesystem::path& base, const std::string& file) {
    return base / file;
}

void CopyFromLocal() {
    auto gameFile = makePath(Mod::get()->getConfigDir(), "settings.json");
    auto saveFile = makePath(Mod::get()->getSaveDir(), "settings.json");

    if (!fileExists(gameFile)) return;
    copyFile(gameFile, saveFile);
}

void CopyFromData() {
    auto gameFile = makePath(Mod::get()->getConfigDir(), "settings.json");
    auto saveFile = makePath(Mod::get()->getSaveDir(), "settings.json");

    if (!fileExists(saveFile)) return;
    copyFile(saveFile, gameFile);
}

inline void updateWindowTitle() {
    std::string name = Mod::get()->getSettingValue<std::string>("windowname");

#if defined(_WIN32)
    HWND hwnd = GetActiveWindow();
    if (hwnd) SetWindowTextA(hwnd, name.c_str());
#else
    id (*msgSend_id)(id, SEL) = (id (*)(id, SEL))objc_msgSend;
    id (*msgSend_id_id)(id, SEL, id) = (id (*)(id, SEL, id))objc_msgSend;

    id nsApp = msgSend_id((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
    id mainWindow = msgSend_id(nsApp, sel_getUid("mainWindow"));

    if (mainWindow) {
        CFStringRef cfStr = CFStringCreateWithCString(NULL, name.c_str(), kCFStringEncodingUTF8);
        id nsString = (id)cfStr;
        msgSend_id_id(mainWindow, sel_getUid("setTitle:"), nsString);
        CFRelease(cfStr);
    }
#endif
}

#if defined(_WIN32)
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#endif

inline void updateWindowIcon() {
    auto iconPath = Mod::get()->getSettingValue<std::filesystem::path>("windowicon");

#if defined(_WIN32)
    HWND hwnd = GetActiveWindow();
    if (!hwnd || !fileExists(iconPath)) return;

    static ULONG_PTR gdiplusToken = 0;
    if (!gdiplusToken) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    }

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(iconPath.wstring().c_str(), FALSE);
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        delete bitmap;
        return;
    }

    HICON hIcon = nullptr;
    if (bitmap->GetHICON(&hIcon) == Gdiplus::Ok && hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    delete bitmap;

#else
    if (!fileExists(iconPath)) return;

    id (*msgSend_id)(id, SEL) = (id (*)(id, SEL))objc_msgSend;
    id (*msgSend_id_id)(id, SEL, id) = (id (*)(id, SEL, id))objc_msgSend;

    id nsApp = msgSend_id((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
    id mainWindow = msgSend_id(nsApp, sel_getUid("mainWindow"));
    if (!mainWindow) return;

    CFStringRef pathStr = CFStringCreateWithCString(NULL, iconPath.string().c_str(), kCFStringEncodingUTF8);
    id nsPath = (id)CFBridgingRelease(CFURLCreateWithFileSystemPath(NULL, pathStr, kCFURLPOSIXPathStyle, false));
    CFRelease(pathStr);

    id nsImageClass = (id)objc_getClass("NSImage");
    id iconImage = msgSend_id_id(nsImageClass, sel_getUid("alloc"), nullptr);
    iconImage = msgSend_id_id(iconImage, sel_getUid("initWithContentsOfURL:"), nsPath);

    if (iconImage) {
        msgSend_id_id(mainWindow, sel_getUid("setRepresentedURL:"), nsPath);
        msgSend_id_id(mainWindow, sel_getUid("setStandardWindowButton:forButtonType:"), iconImage);
    }
#endif
}

#include <Geode/modify/LoadingLayer.hpp>
class $modify(LoadingLayer) {
    bool init(bool p0) {
        if (!LoadingLayer::init(p0))
            return false;
        updateWindowTitle();
        updateWindowIcon();
        return true;
    }
};

$on_mod(Loaded) {
    CopyFromLocal();
    Mod::get()->loadData();
    listenForSettingChanges<std::string>("windowname", [](std::string) {
        updateWindowTitle();
    });
    listenForSettingChanges<std::filesystem::path>("windowicon", [](const std::filesystem::path&) {
        updateWindowIcon();
    });
}

class $modify(AppDelegate) {
    void trySaveGame(bool p0) {
        AppDelegate::trySaveGame(p0);
        CopyFromData();
    }
};
