#pragma once
#include "../NvidiaSettings.hpp"
#include <cmath>

// A dedicated framebuffer keeps the horizontal presets cheap to compose.
// Shared-memory collection remains owned by Status Monitor's worker threads.
class NvidiaOverlay : public tsl::Gui {
    struct Row { std::string label, value; };
    std::vector<Row> rows;
    uint64_t lastUpdate = 0;
    uint64_t buttons = MapButtons(keyCombo);
    bool released = false;
    bool foregroundReleased = false;
    bool panel() const { return nvstyle::launchMode == 3; }
    template<class... Args> void add(const char* label, const char* format, Args... args) {
        char buffer[96]; snprintf(buffer, sizeof buffer, format, args...);
        rows.push_back({label, buffer});
    }
    void collect() {
        using namespace nvstyle;
        rows.clear();
        const bool extended = launchMode >= 2;
        const auto enabled = [](unsigned bit) { return (settings.groups & (1u << bit)) != 0; };
        mutexLock(&mutex_Misc);
        const float fps = useOldFPSavg ? FPSavg_old : FPSavg;
        const bool validFPS = GameRunning && NxFps && fps > 0 && fps < 254;
        if (enabled(0) || launchMode == 0) {
            if (validFPS) add("FPS", "%.0f", double(fps));
            else add("FPS", "%s", "N/A");
        }
        if (launchMode != 0) {
            if (enabled(1) && R_SUCCEEDED(nvCheck)) {
                add(panel() ? tr("GPU utilization", "Загрузка GPU") : "GPU", "%.0f%%", GPU_Load_u / 10.0);
                if (extended && GPU_Hz) add(tr("GPU clock", "Частота GPU"), tr("%u MHz", "%u МГц"), unsigned((realGPU_Hz ? realGPU_Hz : GPU_Hz) / 1000000));
            }
            if (enabled(2)) {
                const double interval = double(systemtickfrequency) / settings.refresh;
                double usage = 0;
                for (uint64_t idle : {idletick0, idletick1, idletick2, idletick3})
                    usage += std::clamp(1.0 - double(idle) / interval, 0.0, 1.0) * 25.0;
                add(panel() ? tr("CPU utilization", "Загрузка CPU") : "CPU", "%.0f%%", usage);
                if (extended && CPU_Hz) add(tr("CPU clock", "Частота CPU"), tr("%u MHz", "%u МГц"), unsigned((realCPU_Hz ? realCPU_Hz : CPU_Hz) / 1000000));
            }
            if (extended && enabled(3) && R_SUCCEEDED(i2cCheck)) {
                add(tr("SoC temperature", "Температура SoC"), "%.0f °C", double(SOC_temperatureF));
                if (panel()) add(tr("Board temperature", "Температура платы"), "%.0f °C", double(PCB_temperatureF));
            }
            if (extended && enabled(4) && R_SUCCEEDED(Hinted)) {
                const double used = double(RAM_Used_application_u) + RAM_Used_applet_u + RAM_Used_system_u + RAM_Used_systemunsafe_u;
                const double total = double(RAM_Total_application_u) + RAM_Total_applet_u + RAM_Total_system_u + RAM_Total_systemunsafe_u;
                if (total > 0) add("RAM", tr("%.2f/%.2f GiB", "%.2f/%.2f ГиБ"), used / 1073741824.0, total / 1073741824.0);
                if (panel() && RAM_Hz) add(tr("Memory clock", "Частота памяти"), tr("%u MHz", "%u МГц"), unsigned((realRAM_Hz ? realRAM_Hz : RAM_Hz) / 1000000));
            }
            if (extended && enabled(5) && R_SUCCEEDED(pwmCheck) && Rotation_Duty > 0)
                add(tr("Fan", "Вент."), "%.0f%%", double(Rotation_Duty));
            if (extended && enabled(7) && validFPS)
                add(tr("Mean frame interval", "Средний интервал кадра"), tr("%.1f ms", "%.1f мс"), 1000.0 / fps);
        }
        mutexUnlock(&mutex_Misc);
        if (extended && enabled(6) && R_SUCCEEDED(psmCheck) && R_SUCCEEDED(i2cCheck)) {
            mutexLock(&mutex_BatteryChecker);
            add(tr("Battery flow", "Поток АКБ"), tr("%+.1f W", "%+.1f Вт"), double(PowerConsumption));
            mutexUnlock(&mutex_BatteryChecker);
        }
    }
    unsigned width(tsl::gfx::Renderer* r, const std::string& s, int size, bool bold = false) {
        return r->drawString(s.c_str(), false, 0, 0, size, tsl::gfx::Color(0x0000), bold).first;
    }
    void text(tsl::gfx::Renderer* r, const std::string& s, int x, int y, int size, tsl::gfx::Color color, bool bold = false) {
        // One-pixel dark shadow keeps white text legible over a bright game.
        r->drawString(s.c_str(), false, x + 1, y + 1, size, r->a(0xC000), bold);
        r->drawString(s.c_str(), false, x, y, size, r->a(color), bold);
    }
    void drawMetrics(tsl::gfx::Renderer* r) {
        using namespace nvstyle;
        if (!foregroundReleased) { tsl::hlp::requestForeground(false); foregroundReleased = true; }
        const int margin = 16, gap = 20;
        int size = settings.size;
        if (panel()) {
            const int boxWidth = 424;
            // Adapt font to translated labels and user-selected size.
            for (; size > 14; --size) {
                bool fits = true;
                for (const auto& row : rows)
                    fits &= width(r, row.label, size) + width(r, row.value, size, true) + 48 <= boxWidth;
                if (fits) break;
            }
            const int line = size + 14;
            const int boxHeight = 54 + int(rows.size()) * line + 14;
            const int y = settings.corner >= 2 ? 720 - boxHeight - margin : margin;
            r->drawRect(12, y, boxWidth, boxHeight, r->a(tsl::gfx::Color(0, 0, 0, settings.opacity)));
            r->drawRect(12, y, boxWidth, 42, r->a(tsl::gfx::Color(1, 1, 1, settings.opacity)));
            r->drawRect(28, y + 15, 12, 12, r->a(tsl::gfx::Color(7, 11, 0, 15)));
            text(r, tr("Performance", "Производительность"), 52, y + 29, 22, tsl::gfx::Color(0xFFFF), true);
            int baseline = y + 54 + size;
            for (const auto& row : rows) {
                text(r, row.label, 28, baseline, size, tsl::gfx::Color(0xFDDD));
                text(r, row.value, 420 - width(r, row.value, size, true), baseline, size, tsl::gfx::Color(7, 11, 0, 15), true);
                baseline += line;
            }
        } else {
            // Advanced uses compact labels; never silently clip metrics off screen.
            std::vector<Row> strip = rows;
            for (auto& row : strip) {
                if (row.label == tr("GPU clock", "Частота GPU")) row.label = "";
                if (row.label == tr("CPU clock", "Частота CPU")) row.label = "";
                if (row.label == tr("SoC temperature", "Температура SoC")) row.label = "SoC";
                if (row.label == tr("Mean frame interval", "Средний интервал кадра")) row.label = "FT";
                if (row.label == tr("Battery flow", "Поток АКБ")) row.label = tr("BAT", "АКБ");
            }
            const auto totalWidth = [&](int fs) {
                int total = 0;
                for (const auto& row : strip) total += width(r, row.label.empty() ? "" : row.label + " ", fs) + width(r, row.value, fs, true) + gap;
                return std::max(0, total - gap);
            };
            while (size > 14 && totalWidth(size) > 1280 - 2 * margin) --size;
            // Wrap only if even the minimum font cannot fit (e.g. all groups enabled).
            const bool wrap = totalWidth(size) > 1280 - 2 * margin;
            int x = (!wrap && (settings.corner % 2)) ? 1280 - margin - totalWidth(size) : margin;
            int y = size + 8;
            if (settings.stripOpacity) r->drawRect(0, 0, 1280, wrap ? 2 * (size + 8) + 4 : size + 16, r->a(tsl::gfx::Color(0, 0, 0, settings.stripOpacity)));
            for (const auto& row : strip) {
                const std::string label = row.label.empty() ? "" : row.label + " ";
                const int lw = width(r, label, size), vw = width(r, row.value, size, true);
                if (x + lw + vw > 1280 - margin) { x = margin; y += size + 8; }
                text(r, label, x, y, size, tsl::gfx::Color(0xFDDD));
                text(r, row.value, x + lw, y, size, tsl::gfx::Color(0xFFFF), true);
                x += lw + vw + gap;
            }
        }
    }
public:
    NvidiaOverlay() {
        FullMode = false; alphabackground = 0; deactivateOriginalFooter = true;
        TeslaFPS = nvstyle::settings.refresh;
        mutexInit(&mutex_BatteryChecker); mutexInit(&mutex_Misc);
        idletick0 = idletick1 = idletick2 = idletick3 = systemtickfrequency / TeslaFPS;
        if (panel()) tsl::gfx::Renderer::getRenderer().setLayerPos(nvstyle::settings.corner % 2 ? 1248 : 0, 0);
        else tsl::gfx::Renderer::getRenderer().setLayerPos(0, nvstyle::settings.corner >= 2 ? (720 - 80) * 3 / 2 : 0);
        StartThreads(nullptr);
        collect();
    }
    ~NvidiaOverlay() { CloseThreads(); }
    tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("", "");
        frame->setContent(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, u16, u16, u16, u16) { drawMetrics(r); }));
        return frame;
    }
    void update() override {
        const uint64_t now = armTicksToNs(svcGetSystemTick());
        if (now - lastUpdate >= 1000000000ULL / nvstyle::settings.refresh) { collect(); lastUpdate = now; }
    }
    bool handleInput(uint64_t down, uint64_t held, const HidTouchState&, JoystickPosition, JoystickPosition) override {
        if ((held & buttons) != buttons) released = true;
        if (released && isKeyComboPressed(held, down, buttons)) {
            tsl::setNextOverlay(nvstyle::overlayPath(), "");
            tsl::Overlay::get()->close();
            return true;
        }
        svcSleepThread(10000000); // Poll the exit chord without running a busy loop.
        return false;
    }
};
