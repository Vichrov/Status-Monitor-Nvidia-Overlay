# NVIDIA Style Monitor 0.1.2 — Nintendo Switch

Unofficial NVIDIA-style performance HUD based on Status Monitor 1.4.1.
Target device: Switch Lite, firmware 22.1.0 / Ultra 3.0, Ultrahand, SaltyNX.
This target has **not been hardware-tested**. A successful build is not a
compatibility certification for a particular firmware or SaltyNX version.

## Установка

1. Скопируйте папки `switch` и `config` из архива в корень SD.
2. В Ultrahand откройте **NVIDIA Style Monitor**, затем **NVIDIA / NVIDIA-стиль**.
3. Выберите один из четырёх режимов. Для выхода в меню удерживайте сочетание
   из `config/status-monitor/config.ini`: по умолчанию **L + вниз + правый стик**.
   Используется именно нажатие правого стика, не отклонение.
4. В меню **Language / Язык** переключается RU/EN. Настройки сохраняются в
   `config/status-monitor/nvidia.ini`; существующие настройки Status Monitor
   не перезаписываются. Первое сохранение происходит при изменении настройки.

Старые режимы Status Monitor оставлены для диагностики. Их интерфейс остаётся
оригинальным; локализованы новые NVIDIA-режимы и их настройки.

## Режимы / Modes

| Mode | Contents |
|---|---|
| FPS only / Только FPS | FPS from SaltyNX |
| Basic horizontal / Краткая строка | FPS, GPU %, average of four CPU cores |
| Advanced horizontal / Подробная строка | Basic + clocks, SoC temperature, RAM, fan duty, mean frame interval, battery flow |
| Performance panel / Панель производительности | Two columns, green values, dark translucent background; also board temperature and memory clock |

The default horizontal background is transparent. Panel opacity defaults to
10/15. Each can be changed independently. Four corners, 16–28 px requested font
size, 1/2/5/10 updates per second and eight metric groups are configurable.
The renderer reduces the font size when needed to fit translated labels;
a very wide horizontal layout wraps into at most two rows.
All CPU/GPU values use stable label/value columns or measured text advances.
No file writes occur in the telemetry loop.

## What the numbers actually mean

- CPU: mean utilization of **all four cores**, including the OS core.
- GPU: hardware load from nvhost. Clocks use real frequency when supplied by a
  compatible sys-clk/Horizon OC; otherwise they are target frequencies.
- SoC temperature is shared by CPU/GPU. No invented separate CPU/GPU readings.
- RAM is shared system memory, not dedicated GPU VRAM. GiB uses 1024³ bytes.
- Fan is duty cycle %, **not RPM**.
- Battery flow is signed battery power: negative discharge, positive charge.
  It is not separate CPU/GPU power or total adapter input power.
- FT / Mean frame interval is **1000 / average FPS**, not render latency,
  controller latency, Reflex, or end-to-end latency.
- FPS displays N/A when no supported running game's FPS data is available.
  Other sensor groups are omitted when their service is unavailable.
- Reflex, mouse/controller/display latency, per-chip voltage/power and fan RPM
  are never displayed. There are no placeholders for unsupported platform metrics.
- 1% low is not implemented: the inherited interface exposes only a short
  frame-tick window without a per-frame sequence number, so polling it would
  duplicate or miss frames. No misleading "1% low" from sampled FPS is shown.

## Шрифты / Fonts

Included: unmodified Liberation Sans Regular and Bold (SIL OFL, license alongside
fonts). These are a usable approximation, **not the original NVIDIA typeface**.
The panel uses a plain green mark, not the official NVIDIA logo. Pixel-identical
matching to the reference is therefore not claimed for this build.

To substitute personally supplied original fonts, replace:

- `config/status-monitor/fonts/regular.ttf`
- `config/status-monitor/fonts/bold.ttf`

Each must be a valid single TTF/OTF sfnt font, <=1 MiB, preferably with Cyrillic.
Despite the configured `.ttf` filenames, OpenType sfnt files are also accepted.
WOFF/WOFF2 must be converted on the PC first. Missing glyphs fall back to the
Switch shared font. If files are absent, the shared font is used and a synthetic
bold weight is applied. Restart the overlay after changing the files.
Do not include proprietary fonts in public redistribution without permission.

## Build and verification

Use devkitPro devkitA64 + libnx, then `make -j2`. The pinned CI container matches
the image used for the local cross-build. GCC treats warnings as errors.

Host layout regression:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter tests/nvidia_layout.cpp -o /tmp/nvidia-layout-test
/tmp/nvidia-layout-test
```

The test compiles the production `NvidiaOverlay`, mocks hardware readings and
uses STB font metrics/rasterization with the supplied fonts. It verifies 448
layouts: RU/EN × four presets × four corners × seven font sizes × present/missing
services. It asserts text/rect bounds and checks that latency labels never appear.
It does **not** emulate libnx, actual shared memory, Ultrahand, or console input.

On-device validation still needed: launching/exiting each preset while a game
runs; game pause/switch/close; sleep/wake; settings persistence; CPU load; battery
charge/discharge; font appearance and readable sizes on the 5.5-inch Lite screen.
If all telemetry except FPS works, check SaltyNX version and game compatibility.

## Исправление сохранения в 0.1.2

Ошибка 0.1.1 возникала при переименовании временного файла поверх уже
существующего nvidia.ini. На Switch такая замена не выполняется. Меню при этом
раньше показывало новое значение из памяти, а после перезапуска HUD считывал
старое значение с SD.

Теперь сохранение использует nvidia.ini.tmp и nvidia.ini.bak без переименования
поверх существующего назначения. При неудаче выполняется откат; если основной
файл отсутствует, при загрузке читается резервный. Меню принимает изменение
только после успешной записи. При ошибке рядом выводится errno.

Для обновления замените только .ovl и перезапустите оверлей. Не удаляйте
nvidia.ini или шрифты. После обновления задайте нужную частоту заново:
несохранённые изменения предыдущей версии восстановить невозможно.

Регрессия сохранения:
```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror tests/nvidia_config.cpp -o /tmp/nvidia-config-test
/tmp/nvidia-config-test
```
Тест использует переименование с отказом при существующем назначении,
40 последовательных изменений всех параметров с загрузкой заново,
цикл частоты начиная с 10, ошибки записи и переименования, откат и чтение .bak.
