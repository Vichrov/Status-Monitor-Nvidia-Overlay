#pragma once
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

namespace nvstyle {
constexpr const char* configPath = "sdmc:/config/status-monitor/nvidia.ini";
using RenameFile = int (*)(const char*, const char*);
struct Settings {
    bool russian = true;
    int corner = 1;
    int size = 20;
    int stripOpacity = 0;
    int opacity = 10;
    int refresh = 5;
    unsigned groups = 255;

    void load(const char* path = configPath) {
        FILE* f = fopen(path, "r");
        // Recover the last committed settings if interrupted between renames.
        if (!f && errno == ENOENT) f = fopen((std::string(path) + ".bak").c_str(), "r");
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

    // Horizon/libnx rename does not replace an existing destination. Keep the
    // previous file until the complete temporary file has been closed, then
    // move it aside. Each rename below targets an absent file.
    bool save(int* error = nullptr, const char* path = configPath,
              RenameFile move = std::rename) const {
        if (error) *error = 0;
        const std::string temporary = std::string(path) + ".tmp";
        const std::string backup = std::string(path) + ".bak";
        const auto fail = [&](int code) {
            if (error) *error = code ? code : EIO;
            return false;
        };
        if (!strcmp(path, configPath)) {
            for (const char* directory : {"sdmc:/config", "sdmc:/config/status-monitor"}) {
                if (mkdir(directory, 0777) != 0) {
                    if (errno != EEXIST) return fail(errno);
                    struct stat info{};
                    if (stat(directory, &info) != 0) return fail(errno);
                    if (!S_ISDIR(info.st_mode)) return fail(ENOTDIR);
                }
            }
        }
        struct stat info{};
        const bool hadOriginal = stat(path, &info) == 0;
        if (!hadOriginal && errno != ENOENT) return fail(errno);
        if (hadOriginal && !S_ISREG(info.st_mode)) return fail(EISDIR);
        FILE* f = fopen(temporary.c_str(), "w");
        if (!f) return fail(errno);
        int writeError = 0;
        if (fprintf(f, "russian=%d\ncorner=%d\nsize=%d\nopacity=%d\nstrip_opacity=%d\nrefresh=%d\ngroups=%u\n",
                    russian, corner, size, opacity, stripOpacity, refresh, groups) < 0)
            writeError = errno ? errno : EIO;
        if (fclose(f) != 0 && !writeError) writeError = errno ? errno : EIO;
        const auto abandon = [&](int code) { std::remove(temporary.c_str()); return fail(code); };
        if (writeError) return abandon(writeError);
        if (hadOriginal) {
            if (std::remove(backup.c_str()) != 0 && errno != ENOENT) return abandon(errno);
            if (move(path, backup.c_str()) != 0) return abandon(errno);
        }
        if (move(temporary.c_str(), path) != 0) {
            const int code = errno;
            // If rollback itself fails, leave .bak available to load().
            if (hadOriginal) move(backup.c_str(), path);
            return abandon(code);
        }
        // Cleanup is not a failed commit: the new main file is already installed.
        std::remove(backup.c_str());
        return true;
    }
};
inline int nextRefreshRate(int rate) {
    return rate == 1 ? 2 : rate == 2 ? 5 : rate == 5 ? 10 : 1;
}
inline bool commitSettings(Settings& current, const Settings& candidate, int* error = nullptr,
                           const char* path = configPath, RenameFile move = std::rename) {
    if (!candidate.save(error, path, move)) return false;
    current = candidate;
    return true;
}
}
