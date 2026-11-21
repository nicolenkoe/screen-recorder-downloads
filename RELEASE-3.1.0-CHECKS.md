# New test releases — 21 November 2026

- Windows: [3.1.0 EXE](https://github.com/nicolenkoe/screen-recorder-downloads/releases/download/windows-3.1.0/ScreenRecorder-Windows-3.1.0.exe)
- Android: [0.1.2 experimental APK](https://github.com/nicolenkoe/screen-recorder-downloads/releases/download/android-0.1.2-experimental/ScreenRecorder-Android-0.1.2-experimental.apk)
- Source implementation: 0692618869be1c25992d0718caf8d51e33636407

## Behavior

Default pause removal protects non-silent audio above -50 dB and 250 ms margins. It protects speech, music and other sounds rather than classifying speech. Very quiet sound below the threshold may be considered silence. Picture-only mode deliberately removes sound in deleted sections. Candidate cuts are shown for review before saving a separate copy.

Android detection samples images every 0.2 seconds. Android keyframe-based export can retain extra footage; snapped overlapping pieces merge to prevent repeated content. Neither platform's audio protection is a guarantee against inaudible or corrupted input: review the selection.

## Checks in order

1. C: gain and PCM formats, time/interval editing, freeze log parsing, silent-range intersection, 250 ms audio margins, 64-cut limits — passed.
2. Java: pause removal intervals, protected sound overlaps, independent audio mixing, explicit pause/resume and stop state transitions — passed.
3. Actual FFmpeg synthetic fixture: 17 seconds with two static runs and two sound intervals. Freeze-only plan removes 13 seconds; sound-protected plan removes about 7 seconds, preserving both sound intervals and margins — passed. Earlier video-only export decoded successfully.
4. GitHub Actions built Windows x64 and Android, signed and published the APK — all succeeded. This is compilation and automated validation, not hardware interaction verification.
5. Downloaded actual released APK: package ru.screenrecorder.app, versionCode 20002, versionName 0.1.2. Size 66006 bytes and SHA-256 match update.json; certificate SHA-256 db805aeefa44bcff4f9cbd567f977ecc5784a942b0d08649fcfd3b6011c9615e matches the prior release.
6. Downloaded actual released EXE: 87812608 bytes; SHA-256 2b7ce931cf1e0521301edf7966e974dca0dc1e6fa32a60a5bdcac1563d4cadb4 matches GitHub asset digest.
7. Eight website release-selection tests passed. Selection against the live GitHub release catalogue chooses Android 0.1.2 experimental and Windows 3.1.0 test as the newest downloads, using their versioned filenames.

## Remaining runtime checks

Real Android pause/resume/stop interaction and phone capture have not been verified for this candidate. Actual Windows capture exclusion and microphone/system audio hardware have not been verified. Releases are explicitly experimental/test. They should not be described as stable or fully device-tested.
