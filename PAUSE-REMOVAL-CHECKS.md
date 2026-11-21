# Automatic shortening of still-picture pauses

The editor has **Убрать паузы…** and a retained-duration setting (default 1 second, allowed 0.1–600 seconds). Analysis replaces the selection after confirmation. The removable tails appear in delete-selection mode and remain editable. Saving creates a separate copy; audio in removed sections is removed too. No silence detector is used.

Windows analyzes the first video track using FFmpeg `setpts=PTS-STARTPTS,freezedetect=n=-60dB`. Analysis runs as a child process, can be cancelled, and does not block the UI. No matches, failure or more than 64 cuts preserves the previous selection.

Android compares 320×180 frames sampled every 0.2 seconds against the current static-run anchor. This is an approximate preview: small or short movement can be missed. The settings dialog discloses the sampling limit. The existing Android exporter cuts at keyframes, so exported boundaries can shift. Review the marked sections before saving. Work runs off the UI thread and can be cancelled.

Checks performed locally:

1. Java interval checks: 5- and 10-second static intervals retain 1 second; short intervals remain; changed pictures differ from identical ones.
2. C parser checks: closed and trailing freezes, threshold equality, and overflow beyond 64 cuts.
3. Actual FFmpeg detection on a synthetic 17-second file (5 seconds static, 2 seconds moving, 10 seconds static): found both static runs, planned removal 13 seconds.
4. Export of remaining sections produced a decodable approximately 4-second video (frame timestamps affect the measured endpoint).
5. Android source build and Windows x64 cross-compile passed. These checks do not replace device testing of the new button or Android detector.

## Sound protection update

Default mode now protects all non-silent audio, rather than attempting to label speech. Windows uses FFmpeg silencedetect (-50 dB, 0.1 s minimum silence); Android decodes the audio track and protects buffers with samples above -50 dB. Both preserve 250 ms at the edges of sound. Very quiet sound below the threshold can still be treated as silence; review the proposed cuts. The alternative mode cuts based on picture alone.

Windows only permits freeze-tail intersections with silence interiors. Android subtracts protected audio intervals from candidate cuts. Failed audio analysis preserves the old selection. Android export now snaps starts backward to a keyframe and merges overlapping snapped pieces: it may retain extra footage, which avoids skipping the start of speech. This remains keyframe-based export, not frame-exact transcoding.

Additional tests cover silent span intersection, 250 ms safety margins, complete sound protection, no audio, overlapping protected spans, and preserving moving sections. Device interaction and microphone hardware have not been tested for this release; publish as experimental/testing.
