#pragma once
#include "NvidiaConfig.hpp"
#include <array>
#include <vector>

namespace nvstyle {
inline Settings settings;
inline int launchMode = -1;
inline const char* tr(const char* en, const char* ru) { return settings.russian ? ru : en; }
inline std::string overlayPath() {
    if (filename.find('/') != std::string::npos) return filename;
    return folderpath + filename;
}
inline const char* modeName(int i) {
    static const char* en[] = {"FPS only", "Basic horizontal", "Advanced horizontal", "Performance panel"};
    static const char* ru[] = {"Только FPS", "Краткая строка", "Подробная строка", "Панель производительности"};
    return settings.russian ? ru[i] : en[i];
}
inline void launch(int mode) {
    std::string args = "--nvidia=" + std::to_string(mode);
    tsl::setNextOverlay(overlayPath(), args);
    tsl::Overlay::get()->close();
}
}
