# NVIDIA Style Monitor — Nintendo Switch

Оверлей производительности в стиле NVIDIA на базе Status Monitor.
Четыре режима, русский и английский интерфейс, настройки позиции, размера,
прозрачности и отображаемых показателей. Ориентирован на Switch / Switch Lite
с Ultrahand или Tesla; для FPS нужен SaltyNX.

**[Скачать релиз](https://github.com/Vichrov/Status-Monitor-Nvidia-Overlay/releases/latest)** ·
**[Установка и настройки](docs/nvidia.md)** · **[История изменений](CHANGELOG.md)**

## Режимы

- **Только FPS** — счётчик кадров.
- **Краткая строка** — FPS, загрузка GPU и CPU.
- **Подробная строка** — частоты, SoC, RAM, Вент./Fan, интервал кадра и поток мощности АКБ.
- **Панель производительности** — вертикальная полупрозрачная панель с зелёными значениями.

Reflex, задержка контроллера и другие отсутствующие на Switch показатели
не отображаются. FT — средний интервал кадра, а не задержка ввода.

## Установка и обновление

Для первой установки распакуйте ZIP из Releases в корень SD.
Для обновления замените только `switch/.overlays/Status-Monitor-Nvidia-Overlay.ovl`:
так сохранятся настройки и уже установленные оригинальные шрифты.
Полностью закройте и заново откройте оверлей.

Публичный полный архив содержит свободные Liberation Sans Regular/Bold.
Собственные NVIDIA Sans можно использовать без пересборки: скопируйте их как
`config/status-monitor/fonts/regular.ttf` и `bold.ttf`.

## Проверки

Версия **0.1.2** исправляет сохранение настроек на SD: повторная запись,
частота обновления после перезапуска и восстановление из резервной копии.
При ошибке записи значение в памяти не изменяется, а меню показывает код ошибки.
В CI проверяются повторные сохранения с поведением файлового API Switch,
отказы записи/переименования и 448 вариантов компоновки RU/EN.
Проверку исправления на физической консоли ещё нужно выполнить.

## English

Unofficial NVIDIA-style performance HUD for Nintendo Switch, based on Status
Monitor. Four presets, RU/EN, configurable metrics, positions, size and opacity.
SaltyNX supplies FPS. Unsupported PC-only latency metrics are omitted.
Download the standalone `.ovl` to upgrade without replacing your custom fonts.
See the [full guide](docs/nvidia.md) for metric definitions and limitations.

## Основа / Credits

Based on masagrator's Status Monitor 1.4.1 and its bundled libtesla fork.
GPL-2.0 project; bundled Liberation fonts use SIL OFL.
[Original upstream documentation and acknowledgments](docs/upstream.md).
Not affiliated with NVIDIA or Nintendo.
