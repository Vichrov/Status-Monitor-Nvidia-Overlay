#include "../source/NvidiaConfig.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
namespace fs = std::filesystem;
static unsigned calls;
static std::set<unsigned> failCalls;
// Reproduce Horizon rename semantics: Linux would silently overwrite instead.
static int switchRename(const char* from, const char* to) {
    if (failCalls.count(++calls)) { errno = EIO; return -1; }
    struct stat st{};
    if (stat(to, &st) == 0) { errno = EEXIST; return -1; }
    return std::rename(from, to);
}
static void inject(std::set<unsigned> failures = {}) { calls = 0; failCalls = failures; }
static bool equal(const nvstyle::Settings& a, const nvstyle::Settings& b) {
    return a.russian == b.russian && a.corner == b.corner && a.size == b.size &&
           a.opacity == b.opacity && a.stripOpacity == b.stripOpacity &&
           a.refresh == b.refresh && a.groups == b.groups;
}
static std::string bytes(const std::string& path) {
    std::ifstream f(path); return {std::istreambuf_iterator<char>(f), {}};
}
int main() {
    char directory[] = "/tmp/nvidia-config-test-XXXXXX";
    assert(mkdtemp(directory));
    const std::string path = std::string(directory) + "/nvidia.ini";
    nvstyle::Settings current;
    int error = -1;
    inject(); assert(current.save(&error, path.c_str(), switchRename)); assert(error == 0);
    // The original tmp -> existing main sequence fails on Switch.
    std::ofstream(path + ".tmp") << "refresh=1\n";
    assert(switchRename((path + ".tmp").c_str(), path.c_str()) == -1 && errno == EEXIST);
    assert(bytes(path).find("refresh=5") != std::string::npos);
    for (int i = 0; i < 40; ++i) {
        auto candidate = current;
        candidate.russian = !candidate.russian;
        candidate.corner = (candidate.corner + 1) % 4;
        candidate.size = candidate.size >= 28 ? 16 : candidate.size + 2;
        candidate.opacity = (candidate.opacity + 1) % 16;
        candidate.stripOpacity = (candidate.stripOpacity + 1) % 16;
        candidate.groups ^= 1u << (i % 8);
        candidate.refresh = nvstyle::nextRefreshRate(candidate.refresh);
        inject(); assert(nvstyle::commitSettings(current, candidate, &error, path.c_str(), switchRename));
        nvstyle::Settings restarted; restarted.load(path.c_str()); assert(equal(restarted, candidate));
        assert(!fs::exists(path + ".bak") && !fs::exists(path + ".tmp"));
    }
    current.refresh = 10; inject(); assert(current.save(&error, path.c_str(), switchRename));
    for (int expected : {1, 2, 5, 10, 1}) {
        auto next = current; next.refresh = nvstyle::nextRefreshRate(current.refresh);
        inject(); assert(nvstyle::commitSettings(current, next, &error, path.c_str(), switchRename));
        nvstyle::Settings restarted; restarted.load(path.c_str()); assert(restarted.refresh == expected);
    }
    const auto saved = current;
    const auto original = bytes(path);
    auto candidate = current; candidate.refresh = 10; candidate.russian = !current.russian;
    for (auto failures : {std::set<unsigned>{1}, std::set<unsigned>{2}, std::set<unsigned>{2, 3}}) {
        inject(failures);
        assert(!nvstyle::commitSettings(current, candidate, &error, path.c_str(), switchRename));
        assert(error == EIO && equal(current, saved));
        nvstyle::Settings restarted; restarted.load(path.c_str()); assert(equal(restarted, saved));
        inject(); assert(current.save(&error, path.c_str(), switchRename));
        assert(bytes(path) == original);
    }
    // Simulated interruption between renames; never load the uncommitted tmp.
    inject(); assert(switchRename(path.c_str(), (path + ".bak").c_str()) == 0);
    std::ofstream(path + ".tmp") << "refresh=10\n";
    nvstyle::Settings recovered; recovered.load(path.c_str()); assert(equal(recovered, saved));
    inject(); assert(recovered.save(&error, path.c_str(), switchRename));
    assert(bytes(path) == original && !fs::exists(path + ".bak"));
    // Failure to open temporary output must not change memory or saved settings.
    fs::create_directory(path + ".tmp");
    inject(); assert(!nvstyle::commitSettings(current, candidate, &error, path.c_str(), switchRename));
    assert(error != 0 && equal(current, saved) && bytes(path) == original);
    fs::remove(path + ".tmp");
    // Successful main takes precedence over an obsolete backup.
    std::ofstream(path + ".bak") << "refresh=10\n";
    nvstyle::Settings preferred; preferred.load(path.c_str()); assert(equal(preferred, saved));
    inject(); assert(candidate.save(&error, path.c_str(), switchRename));
    preferred.load(path.c_str()); assert(equal(preferred, candidate));
    assert(!fs::exists(path + ".bak"));
    inject(); assert(!candidate.save(&error, (path + "/missing.ini").c_str(), switchRename));
    assert(error != 0);
    fs::remove_all(directory);
    std::cout << "PASS: no-replace regression, 40 edit/reload cycles, refresh cycle from 10, rollback/crash recovery, write errors and in-memory commit semantics\n";
}
