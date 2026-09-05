# ITN Studio Launcher

Кастомный Minecraft-лаунчер серверов **ITN** на базе [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher) + Ely.by.

- Модовый сервер: `isnix.ru`
- Ванильный: `vanilla.isnix.ru:20045`
- Discord: https://discord.gg/mUq5MgnMx

## Скачать (игрокам)

Готовая **portable** сборка Windows — в [Releases](https://github.com/NIXXXON177/ITN-Studio-s-Launcher/releases):

1. Скачай `ITNLauncher-windows.zip`
2. Распакуй и запусти `ITNLauncher.exe`
3. Войди через Ely.by → Play

Внутри уже есть **ITN Modded**, **ITN Vanilla** и Java 17 (`jre/`).

## Автообновление

При запуске сначала открывается экран проверки обновлений:

1. Проверка / скачивание / установка **лаунчера** (GitHub Releases → `ITNLauncher-windows*.zip`)
2. Проверка / скачивание **игры** (asset `ITN-Modded*.mrpack` в том же релизе, если есть)
3. После успеха — основной интерфейс (с лёгкими звуками наведения на кнопки)

Локальные метки: `itn-version.txt` (лаунчер), `itn-game-version.txt` (модпак). Сейвы и `accounts.json` сохраняются.

## Сборка из исходников

Стек: **C++ / Qt 6**, CMake + vcpkg (как у Prism). См. upstream README и `CMakePresets.json`.

Основные ITN-изменения:

- брендинг `ITNLauncher`
- Ely.by auth
- автоимпорт `ITN-Modded.mrpack`
- встроенная Java (`./jre`)
- страница игровых настроек / импорт из других лаунчеров (миграция)

## Лицензия

GPL-3.0 (наследование от Prism / MultiMC). См. `LICENSE` / `COPYING.md`.
