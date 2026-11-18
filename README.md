# Запись экрана — Android и Windows

[Сайт](https://nicolenkoe.github.io/screen-recorder-downloads/) · [Скачать Android APK 0.1.1-experimental](https://github.com/nicolenkoe/screen-recorder-downloads/releases/download/android-0.1.1-experimental/ScreenRecorder.apk) · [Windows 3.0.1 EXE](https://github.com/nicolenkoe/screen-recorder-downloads/releases/download/windows-3.0.1/ScreenRecorder-Windows-3.0.1.exe)

Android создаётся с нуля: запись экрана, микрофон и внутренний звук, громкость микрофона, пауза через уведомление, галерея, просмотр и быстрая обрезка копии. Android 10+. Это первый экспериментальный APK: сборка и подпись проверены, запись на реальном Android ещё не проверена.

Исходники: `android/`, текущая Windows-версия `windows/`. Android проверяет опубликованные GitHub Releases при каждом возвращении в приложение, по умолчанию включает prerelease. Фильтр «Только стабильные версии» находится в настройках обновлений. Установка требует подтверждения Android.

Windows 3.0.1: новое тёмное оформление с мятными кнопками, компактный выбор шрифта, область/окно/экран/все мониторы, громкость микрофона, галерея с просмотром и обрезкой отдельной копии. Плавающий индикатор включается только при подтверждённой поддержке исключения из захвата. Несколько отдельных окон и пауза записи Windows пока не реализованы.

[Видеоинструкция со звуком](https://nicolenkoe.github.io/screen-recorder-downloads/#guide): 1 минута 46 секунд, настоящий интерфейс Windows 3.0.1, женская русская озвучка, красные рамки на элементах управления и субтитры.

## Сборки

[GitHub Actions](https://github.com/nicolenkoe/screen-recorder-downloads/actions): сборка Android/Windows и публикация сайта настроены и проверены. Секреты KEYSTORE_BASE64 и KEYSTORE_PASSWORD работают: подписанный Android-релиз успешно собран и опубликован в Actions. Закрытый ключ и пароль никогда не должны находиться в публичном репозитории.

[Настройка и сборка](SETUP.md) · [Проверки по порядку](CHECKS.md)
