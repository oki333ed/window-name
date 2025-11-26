#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/AppDelegate.hpp>

#if defined(_WIN32)
    #include <windows.h>
    #include <string>
#else
    #include <filesystem>
    #include <fstream>
    #include <objc/objc.h>
    #include <objc/message.h>
#endif

using namespace geode::prelude;

inline bool fileExists(const std::string& path) {
#if defined(_WIN32)
    std::wstring wpath(path.begin(), path.end());
    DWORD attrs = GetFileAttributesW(wpath.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    return std::filesystem::exists(path);
#endif
}

inline void copyFile(const std::string& from, const std::string& to) {
#if defined(_WIN32)
    std::wstring wfrom(from.begin(), from.end());
    std::wstring wto(to.begin(), to.end());
    (void)CopyFileW(wfrom.c_str(), wto.c_str(), FALSE);
#else
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
#endif
}

inline std::string makePath(const std::filesystem::path& base, const std::string& file) {
#if defined(_WIN32)
    std::wstring wpath = base.wstring() + L"\\" + std::wstring(file.begin(), file.end());
    return std::string(wpath.begin(), wpath.end());
#else
    return (base / file).string();
#endif
}

void CopyFromLocal() {
    std::string gameFile = makePath(Mod::get()->getConfigDir(), "settings.json");
    std::string saveFile = makePath(Mod::get()->getSaveDir(), "settings.json");

    if (!fileExists(gameFile)) return;
    copyFile(gameFile, saveFile);
}

void CopyFromData() {
    std::string gameFile = makePath(Mod::get()->getConfigDir(), "settings.json");
    std::string saveFile = makePath(Mod::get()->getSaveDir(), "settings.json");

    if (!fileExists(saveFile)) return;
    copyFile(saveFile, gameFile);
}

void updateWindowTitle() {
    std::string name = Mod::get()->getSettingValue<std::string>("windowname");

#if defined(_WIN32)
    HWND hwnd = GetActiveWindow();
    if (hwnd) SetWindowTextA(hwnd, name.c_str());
#else
    id nsApp = objc_msgSend((id)objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
    id mainWindow = objc_msgSend(nsApp, sel_getUid("mainWindow"));
    if (mainWindow) {
        id nsString = objc_msgSend((id)objc_getClass("NSString"), sel_getUid("stringWithUTF8String:"), name.c_str());
        objc_msgSend(mainWindow, sel_getUid("setTitle:"), nsString);
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
