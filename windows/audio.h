#ifndef RECORDER_AUDIO_H
#define RECORDER_AUDIO_H
#include <windows.h>
typedef struct {
    HANDLE thread,ready,go,stop;
    int loopback;
    volatile LONG error;
    volatile LONG gain,level,muted;
    wchar_t path[4096];
    UINT64 start100ns,end100ns;
} AudioTrack;
int audio_prepare(AudioTrack *a,int loopback,const wchar_t *path);
void audio_begin(AudioTrack *a,UINT64 start);
void audio_stop(AudioTrack *a,UINT64 end);
int audio_done(AudioTrack *a);
void audio_release(AudioTrack *a);
UINT64 audio_clock(void);
#endif
