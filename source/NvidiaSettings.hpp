#pragma once
#include <array>
#include <string>
#include <vector>

namespace nvstyle {
constexpr const char* configPath = "sdmc:/config/status-monitor/nvidia.ini";
struct Settings {
    bool russian = true;
    int corner = 1; // TL, TR, BL, BR
    int size = 20;
    int stripOpacity = 0;
    int opacity = 10; // RGBA4444 alpha
    int refresh = 5;
    unsigned groups = 255;
    void load() {
        FILE* f = fopen(configPath, "r");
        if (!f) return;
        char line[128], key[64]; int value;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%63[^=]=%d", key, &value) != 2) continue;
            if (!strcmp(key, "russian")) russian = value != 0;
            else if (!strcmp(key, "corner")) corner = std::clamp(value, 0, 3);
            else if (!strcmp(key, "size")) size = std::clamp(value, 16, 28);
            else if (!strcmp(key, "opacity")) opacity = std::clamp(value, 0, 15);
            else if (!strcmp(key, "strip_opacity")) stripOpacity = std::clamp(value, 0, 15);
            else if (!strcmp(key, "refresh")) refresh = std::clamp(value, 1, 10);
            else if (!strcmp(key, "groups")) groups = unsigned(value) & 255;
        }
        fclose(f);
    }
    bool save() const {
        mkdir("sdmc:/config", 0777);
        mkdir("sdmc:/config/status-monitor", 0777);
        const std::string temporary = std::string(configPath) + ".tmp";
        FILE* f = fopen(temporary.c_str(), "w");
        if (!f) return false;
        bool ok = fprintf(f, "russian=%d\ncorner=%d\nsize=%d\nopacity=%d\nstrip_opacity=%d\nrefresh=%d\ngroups=%u\n", russian, corner, size, opacity, stripOpacity, refresh, groups) > 0;
        if (fclose(f)) ok = false;
        return ok && rename(temporary.c_str(), configPath) == 0;
    }
};
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
