#pragma once
#include "NvidiaSettings.hpp"

class NvidiaOptions : public tsl::Gui {
    void save(tsl::elm::ListItem* item, const std::string& value) {
        item->setValue(nvstyle::settings.save() ? value : nvstyle::tr("Save failed", "Ошибка записи"));
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
        const auto cornerName = []() -> const char* {
            static const char* en[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
            static const char* ru[] = {"Слева сверху", "Справа сверху", "Слева снизу", "Справа снизу"};
            return settings.russian ? ru[settings.corner] : en[settings.corner];
        };
        corner->setValue(cornerName());
        corner->setClickListener([this, corner, cornerName](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            settings.corner = (settings.corner + 1) % 4; save(corner, cornerName()); return true;
        }); list->addItem(corner);
        auto size = new tsl::elm::ListItem(tr("Font size", "Размер шрифта"));
        size->setValue(std::to_string(settings.size));
        size->setClickListener([this, size](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            settings.size = settings.size >= 28 ? 16 : settings.size + 2; save(size, std::to_string(settings.size)); return true;
        }); list->addItem(size);
        auto opacity = new tsl::elm::ListItem(tr("Panel background", "Фон панели"));
        opacity->setValue(std::to_string(settings.opacity) + "/15");
        opacity->setClickListener([this, opacity](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            settings.opacity = (settings.opacity + 1) % 16; save(opacity, std::to_string(settings.opacity) + "/15"); return true;
        }); list->addItem(opacity);
        auto strip = new tsl::elm::ListItem(tr("Horizontal background", "Фон строки"));
        strip->setValue(std::to_string(settings.stripOpacity) + "/15");
        strip->setClickListener([this, strip](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            settings.stripOpacity = (settings.stripOpacity + 1) % 16;
            save(strip, std::to_string(settings.stripOpacity) + "/15"); return true;
        }); list->addItem(strip);
        auto rate = new tsl::elm::ListItem(tr("Refresh rate", "Обновлений в секунду"));
        rate->setValue(std::to_string(settings.refresh));
        rate->setClickListener([this, rate](uint64_t keys) {
            if (!(keys & KEY_A)) return false;
            settings.refresh = settings.refresh == 1 ? 2 : settings.refresh == 2 ? 5 : settings.refresh == 5 ? 10 : 1;
            save(rate, std::to_string(settings.refresh)); return true;
        }); list->addItem(rate);
        static const char* en[] = {"FPS", "GPU", "CPU", "Temperatures", "RAM", "Fan", "Battery power flow", "Mean frame interval"};
        static const char* ru[] = {"FPS", "GPU", "CPU", "Температуры", "RAM", "Вентилятор", "Поток мощности АКБ", "Средний интервал кадра"};
        for (unsigned i = 0; i < 8; ++i) {
            auto item = new tsl::elm::ListItem(settings.russian ? ru[i] : en[i]);
            const auto value = [i]() { return (settings.groups & (1u << i)) ? tr("On", "Вкл.") : tr("Off", "Выкл."); };
            item->setValue(value());
            item->setClickListener([this, item, i, value](uint64_t keys) {
                if (!(keys & KEY_A)) return false;
                settings.groups ^= 1u << i; save(item, value()); return true;
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
            settings.russian = !settings.russian;
            if (!settings.save()) { language->setValue("Save failed"); return true; }
            tsl::goBack(); tsl::changeTo<NvidiaMenu>(); return true;
        }); list->addItem(language);
        frame->setContent(list); return frame;
    }
};
