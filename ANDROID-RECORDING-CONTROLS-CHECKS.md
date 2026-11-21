# Android recording controls

- Start, Pause, Resume, and Stop are separate buttons near the top of the home screen. During a session Pause and Resume keep separate positions; only the appropriate action is enabled.
- Both in-app controls and notification actions call RecordingService.command and RecordingControls.dispatch. Pause and resume are explicit idempotent operations, rather than a toggle. Stop is available during startup and pause; controls lock during saving.
- A fixed header remains above the scrolling content on every app screen. Its red dot pulses only after a video sample and MP4 tracks exist. Pause uses a steady amber pause mark. Preparation, saving, and idle have separate labels.
- After Android capture consent, a fullscreen countdown displays 3, 2, 1. Cancel/back cancels the timer. The capture service starts after the countdown is dismissed. The app remains open so its controls are immediately accessible.

Local checks:

1. Pure Java RecordingControlsTest: preparation rejects pause/resume; repeated pause does not resume; repeated resume does not pause; stopping works from recording preparation and pause; duplicate stop and controls during saving are rejected.
2. Android compilation against SDK 35 passed.
3. Existing release selection, editing, pause detection interval and audio mix checks passed.
4. No phone/emulator interaction check has been completed for these changes. Notification handling, MediaProjection, and encoded pause timing still need Android runtime validation before a claim of device verification.
