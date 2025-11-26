#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/AppDelegate.hpp>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <filesystem>
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

class $modify(LoadingLayer) {
    bool init(bool p0) {
        if (!LoadingLayer::init(p0))
            return false;
        updateWindowTitle();
        return true;
    }
};

$on_mod(Loaded) {
    CopyFromLocal();
    Mod::get()->loadData();
    geode::listenForSettingChanges("windowname", [](std::string) {
        updateWindowTitle();
    });
}

class $modify(AppDelegate) {
    void trySaveGame(bool p0) {
        AppDelegate::trySaveGame(p0);
        CopyFromData();
    }
};
