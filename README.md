# Запись экрана — Android и Windows

[Сайт](https://nicolenkoe.github.io/screen-recorder-downloads/) · [Скачать Android APK 0.1.1-experimental](https://raw.githubusercontent.com/nicolenkoe/screen-recorder-downloads/main/downloads/ScreenRecorder-0.1.1-experimental.apk) · [Windows 2.1 EXE](https://github.com/nicolenkoe/screen-recorder-downloads/releases/download/windows-2.1/ScreenRecorder-Windows-2.1.exe)

Android создаётся с нуля: запись экрана, микрофон и внутренний звук, громкость микрофона, пауза через уведомление, галерея, просмотр и быстрая обрезка копии. Android 10+. Это первый экспериментальный APK: сборка и подпись проверены, запись на реальном Android ещё не проверена.

Исходники: `android/`, текущая Windows-версия `windows/`. Android проверяет опубликованные GitHub Releases при каждом возвращении в приложение, по умолчанию включает prerelease. Фильтр «Только стабильные версии» находится в настройках обновлений. Установка требует подтверждения Android.

Windows 2.1: выделение области, микрофон и звук компьютера. Новая галерея, оформление и исключённый плавающий индикатор ещё не реализованы. Видеоинструкция ещё не опубликована, на сайте это явно отмечено.

## Сборки

[GitHub Actions](https://github.com/nicolenkoe/screen-recorder-downloads/actions): сборка Android/Windows и публикация сайта настроены и проверены. Для следующих подписанных Android-релизов нужно добавить два секрета из приватной резервной копии: KEYSTORE_BASE64 и KEYSTORE_PASSWORD. Закрытый ключ и пароль никогда не должны находиться в публичном репозитории.

[Настройка и сборка](SETUP.md) · [Проверки по порядку](CHECKS.md)
