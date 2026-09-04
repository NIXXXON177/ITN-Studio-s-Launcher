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

Лаунчер при старте проверяет [GitHub Releases](https://github.com/NIXXXON177/ITN-Studio-s-Launcher/releases):

1. Сравнивает локальный `itn-version.txt` с тегом latest
2. Скачивает `ITNLauncher-windows*.zip`
3. Ставит поверх, **сохраняя** `accounts.json`, сейвы и скриншоты
4. Перезапускается

Пока нет свежей пересборки `ITNLauncher.exe`, можно вручную: `ITN-update.ps1` в portable-папке.

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
