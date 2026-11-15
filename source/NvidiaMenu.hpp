#pragma once
#include "NvidiaSettings.hpp"

class NvidiaOptions : public tsl::Gui {
    template<class Change, class Display>
    void change(tsl::elm::ListItem* item, Change mutate, Display display) {
        auto candidate = nvstyle::settings;
        mutate(candidate);
        int error = 0;
        if (nvstyle::commitSettings(nvstyle::settings, candidate, &error))
            item->setValue(display(nvstyle::settings));
        else item->setValue(std::string(nvstyle::tr("Save error", "Ошибка записи")) + " (" + std::to_string(error) + ")");
    }
public:
    bool handleInput(uint64_t down, uint64_t, const HidTouchState&, JoystickPosition, JoystickPosition) override {
        if (down & KEY_B) { tsl::goBack(); return true; }
        return false;
    }
    tsl::elm::Element* createUI() override {
        using namespace nvstyle;
        auto frame = new tsl::elm::OverlayFrame("NVIDIA Style", tr("Settings", "Настройки"));
        auto list = new tsl::elm::List();
        auto corner = new tsl::elm::ListItem(tr("Position (A: next)", "Позиция (A: сменить)"));
        const auto cornerName = [](const Settings& value) -> const char* {
            static const char* en[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
            static const char* ru[] = {"Слева сверху", "Справа сверху", "Слева снизу", "Справа снизу"};
            return value.russian ? ru[value.corner] : en[value.corner];
        };
        corner->setValue(cornerName(settings));
        corner->setClickListener([this, corner, cornerName](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            change(corner, [](Settings& s) { s.corner = (s.corner + 1) % 4; }, cornerName); return true;
        }); list->addItem(corner);
        auto size = new tsl::elm::ListItem(tr("Font size", "Размер шрифта"));
        size->setValue(std::to_string(settings.size));
        size->setClickListener([this, size](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            change(size, [](Settings& s) { s.size = s.size >= 28 ? 16 : s.size + 2; }, [](const Settings& s) { return std::to_string(s.size); }); return true;
        }); list->addItem(size);
        auto opacity = new tsl::elm::ListItem(tr("Panel background", "Фон панели"));
        opacity->setValue(std::to_string(settings.opacity) + "/15");
        opacity->setClickListener([this, opacity](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            change(opacity, [](Settings& s) { s.opacity = (s.opacity + 1) % 16; }, [](const Settings& s) { return std::to_string(s.opacity) + "/15"; }); return true;
        }); list->addItem(opacity);
        auto strip = new tsl::elm::ListItem(tr("Horizontal background", "Фон строки"));
        strip->setValue(std::to_string(settings.stripOpacity) + "/15");
        strip->setClickListener([this, strip](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            change(strip, [](Settings& s) { s.stripOpacity = (s.stripOpacity + 1) % 16; }, [](const Settings& s) { return std::to_string(s.stripOpacity) + "/15"; }); return true;
        }); list->addItem(strip);
        auto rate = new tsl::elm::ListItem(tr("Refresh rate", "Обновлений в секунду"));
        rate->setValue(std::to_string(settings.refresh));
        rate->setClickListener([this, rate](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            change(rate, [](Settings& s) { s.refresh = nextRefreshRate(s.refresh); }, [](const Settings& s) { return std::to_string(s.refresh); }); return true;
        }); list->addItem(rate);
        static const char* en[] = {"FPS", "GPU", "CPU", "Temperatures", "RAM", "Fan", "Battery power flow", "Mean frame interval"};
        static const char* ru[] = {"FPS", "GPU", "CPU", "Температуры", "RAM", "Вент.", "Поток мощности АКБ", "Средний интервал кадра"};
        for (unsigned i = 0; i < 8; ++i) {
            auto item = new tsl::elm::ListItem(settings.russian ? ru[i] : en[i]);
            const auto value = [i](const Settings& s) { return (s.groups & (1u << i)) ? tr("On", "Вкл.") : tr("Off", "Выкл."); };
            item->setValue(value(settings));
            item->setClickListener([this, item, i, value](uint64_t keys) {
                if (!(keys & KEY_A)) return false;
                change(item, [i](Settings& s) { s.groups ^= 1u << i; }, value); return true;
            }); list->addItem(item);
        }
        frame->setContent(list); return frame;
    }
};

class NvidiaMenu : public tsl::Gui {
public:
    bool handleInput(uint64_t down, uint64_t, const HidTouchState&, JoystickPosition, JoystickPosition) override {
        if (down & KEY_B) { tsl::goBack(); return true; }
        return false;
    }
    tsl::elm::Element* createUI() override {
        using namespace nvstyle;
        auto frame = new tsl::elm::OverlayFrame("NVIDIA Style", APP_VERSION);
        auto list = new tsl::elm::List();
        for (int mode = 0; mode < 4; ++mode) {
            auto item = new tsl::elm::ListItem(modeName(mode));
            item->setClickListener([mode](uint64_t keys) {
                if (!(keys & KEY_A)) return false;
                launch(mode); return true;
            }); list->addItem(item);
        }
        auto options = new tsl::elm::ListItem(tr("Settings", "Настройки"));
        options->setClickListener([](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            tsl::changeTo<NvidiaOptions>(); return true;
        }); list->addItem(options);
        auto language = new tsl::elm::ListItem("Language / Язык");
        language->setValue(settings.russian ? "Русский" : "English");
        language->setClickListener([language](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            auto candidate = settings;
            candidate.russian = !candidate.russian;
            int error = 0;
            if (!commitSettings(settings, candidate, &error)) {
                language->setValue(std::string(tr("Save error", "Ошибка записи")) + " (" + std::to_string(error) + ")"); return true;
            }
            tsl::goBack(); tsl::changeTo<NvidiaMenu>(); return true;
        }); list->addItem(language);
        frame->setContent(list); return frame;
    }
};
