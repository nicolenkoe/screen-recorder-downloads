#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include "audio.h"
#include "wav.h"
#include "samples.h"

UINT64 audio_clock(void) {
    LARGE_INTEGER n,f; QueryPerformanceCounter(&n); QueryPerformanceFrequency(&f);
    return (UINT64)((long double)n.QuadPart*10000000.0L/f.QuadPart);
}
static DWORD WINAPI capture(LPVOID ctx) {
    AudioTrack *a=ctx;
    IMMDeviceEnumerator *devices=NULL; IMMDevice *device=NULL;
    IAudioClient *client=NULL; IAudioCaptureClient *capture=NULL;
    WAVEFORMATEX *format=NULL; Wave wave={0}; FILE *file=NULL;
    BYTE *scaled=NULL;size_t scaledCapacity=0;
    int com=0,started=0; HRESULT hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if(FAILED(hr)) goto cleanup; com=1;
    hr=CoCreateInstance(&CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void**)&devices);
    if(FAILED(hr)) goto cleanup;
    hr=IMMDeviceEnumerator_GetDefaultAudioEndpoint(devices,a->loopback?eRender:eCapture,eConsole,&device);
    if(FAILED(hr)) goto cleanup;
    hr=IMMDevice_Activate(device,&IID_IAudioClient,CLSCTX_ALL,NULL,(void**)&client);
    if(FAILED(hr)) goto cleanup;
    hr=IAudioClient_GetMixFormat(client,&format); if(FAILED(hr)) goto cleanup;
    hr=IAudioClient_Initialize(client,AUDCLNT_SHAREMODE_SHARED,a->loopback?AUDCLNT_STREAMFLAGS_LOOPBACK:0,
                              1000000,0,format,NULL);
    if(FAILED(hr)) goto cleanup;
    hr=IAudioClient_GetService(client,&IID_IAudioCaptureClient,(void**)&capture); if(FAILED(hr)) goto cleanup;
    file=_wfopen(a->path,L"wb"); if(!file) { hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT); goto cleanup; }
    if(!wav_open(&wave,file,format,sizeof(WAVEFORMATEX)+format->cbSize,format->nBlockAlign)) {
        hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT); goto cleanup;
    }
    SetEvent(a->ready);
    HANDLE wait[]={a->stop,a->go};
    if(WaitForMultipleObjects(2,wait,FALSE,INFINITE)!=WAIT_OBJECT_0+1) goto cleanup;
    hr=IAudioClient_Start(client); if(FAILED(hr)) goto cleanup; started=1;
    UINT64 written=0,nextCheckpoint=format->nSamplesPerSec;
    for(;;) {
        int stopping=WaitForSingleObject(a->stop,0)==WAIT_OBJECT_0;
        UINT32 packet=0;
        hr=IAudioCaptureClient_GetNextPacketSize(capture,&packet); if(FAILED(hr)) goto cleanup;
        unsigned int drained=0;
        while(packet && drained++<128) {
            stopping=WaitForSingleObject(a->stop,0)==WAIT_OBJECT_0;
            BYTE *bytes=NULL; UINT32 frames=0; DWORD flags=0; UINT64 devicePos=0,qpc=0;
            hr=IAudioCaptureClient_GetBuffer(capture,&bytes,&frames,&flags,&devicePos,&qpc);
            if(FAILED(hr)) goto cleanup;
            UINT64 at=written;
            UINT64 now=audio_clock();
            if(qpc>now+10000000 || qpc+10000000<a->start100ns)flags|=AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR;
            if(!(flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) && qpc>=a->start100ns)
                at=(UINT64)((long double)(qpc-a->start100ns)*format->nSamplesPerSec/10000000.0L);
            else if(!(flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) && qpc<a->start100ns) at=0;
            UINT64 maxFrames=stopping?(UINT64)((long double)(a->end100ns-a->start100ns)*format->nSamplesPerSec/10000000.0L):UINT64_MAX;
            if(at>maxFrames) at=maxFrames;
            int ok=1;
            if(at>written) { ok=wav_samples(&wave,NULL,at-written); written=at; }
            UINT32 skip=0;
            if(qpc<a->start100ns && !(flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR)) {
                UINT64 before=(UINT64)((long double)(a->start100ns-qpc)*format->nSamplesPerSec/10000000.0L);
                skip=(UINT32)(before>frames?frames:before);
            } else if(at<written) { UINT64 overlap=written-at; skip=(UINT32)(overlap>frames?frames:overlap); }
            UINT64 count=frames-skip;
            if(written>=maxFrames) count=0;
            else if(count>maxFrames-written) count=maxFrames-written;
            if(ok && count){const BYTE *samples=(flags&AUDCLNT_BUFFERFLAGS_SILENT)?NULL:bytes+(size_t)skip*format->nBlockAlign;
                if(!a->loopback && samples){size_t needed=(size_t)count*format->nBlockAlign;if(needed>scaledCapacity){if(scaled)HeapFree(GetProcessHeap(),0,scaled);scaled=HeapAlloc(GetProcessHeap(),0,needed);scaledCapacity=needed;}
                    if(!scaled){hr=E_OUTOFMEMORY;IAudioCaptureClient_ReleaseBuffer(capture,frames);goto cleanup;}memcpy(scaled,samples,needed);
                    int floating=format->wFormatTag==WAVE_FORMAT_IEEE_FLOAT||(format->wFormatTag==WAVE_FORMAT_EXTENSIBLE&&((WAVEFORMATEXTENSIBLE*)format)->SubFormat.Data1==WAVE_FORMAT_IEEE_FLOAT);
                    InterlockedExchange(&a->level,scale_microphone(scaled,(unsigned int)count,format->nChannels,format->wBitsPerSample,floating,(int)a->gain));samples=scaled;
                }else if(!a->loopback)InterlockedExchange(&a->level,0);
                ok=wav_samples(&wave,samples,count);
            }
            written+=count;
            HRESULT released=IAudioCaptureClient_ReleaseBuffer(capture,frames);
            if(!ok) { hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT); goto cleanup; }
            if(FAILED(released)) { hr=released; goto cleanup; }
            if(stopping) break;
            if(written>=nextCheckpoint) {
                if(!wav_checkpoint(&wave)) { hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);goto cleanup; }
                nextCheckpoint=written+format->nSamplesPerSec;
            }
            hr=IAudioCaptureClient_GetNextPacketSize(capture,&packet); if(FAILED(hr)) goto cleanup;
        }
        if(stopping) {
            UINT64 total=(UINT64)((long double)(a->end100ns-a->start100ns)*format->nSamplesPerSec/10000000.0L);
            if(total>written && !wav_samples(&wave,NULL,total-written)) hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            break;
        }
        WaitForSingleObject(a->stop,10);
    }
cleanup:
    if(scaled)HeapFree(GetProcessHeap(),0,scaled);
    if(started) IAudioClient_Stop(client);
    if(wave.f) { if(!wav_close(&wave) && SUCCEEDED(hr)) hr=HRESULT_FROM_WIN32(ERROR_WRITE_FAULT); file=NULL; }
    if(file) fclose(file);
    if(FAILED(hr)) InterlockedExchange(&a->error,(LONG)hr);
    SetEvent(a->ready);
    if(capture) IAudioCaptureClient_Release(capture);
    if(client) IAudioClient_Release(client);
    if(device) IMMDevice_Release(device);
    if(devices) IMMDeviceEnumerator_Release(devices);
    if(format) CoTaskMemFree(format);
    if(com) CoUninitialize();
    return 0;
}
int audio_prepare(AudioTrack *a,int loopback,const wchar_t *path) {
    ZeroMemory(a,sizeof(*a)); a->gain=100;a->loopback=loopback; wcscpy(a->path,path);
    a->ready=CreateEventW(NULL,TRUE,FALSE,NULL); a->go=CreateEventW(NULL,TRUE,FALSE,NULL); a->stop=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(!a->ready || !a->go || !a->stop) { a->error=E_OUTOFMEMORY; return 0; }
    a->thread=CreateThread(NULL,0,capture,a,0,NULL);
    if(!a->thread) { a->error=E_FAIL; return 0; }
    DWORD result=WaitForSingleObject(a->ready,10000);
    if(result!=WAIT_OBJECT_0) { a->error=HRESULT_FROM_WIN32(ERROR_TIMEOUT); SetEvent(a->stop); return 0; }
    return a->error==0;
}
void audio_begin(AudioTrack *a,UINT64 start) { if(a->thread) { a->start100ns=start; SetEvent(a->go); } }
void audio_stop(AudioTrack *a,UINT64 end) { if(a->thread && WaitForSingleObject(a->stop,0)!=WAIT_OBJECT_0) { a->end100ns=end; SetEvent(a->stop); } }
int audio_done(AudioTrack *a) { return !a->thread || WaitForSingleObject(a->thread,0)==WAIT_OBJECT_0; }
void audio_release(AudioTrack *a) {
    if(!audio_done(a)) return;
    if(a->thread) CloseHandle(a->thread); if(a->ready) CloseHandle(a->ready);
    if(a->go) CloseHandle(a->go); if(a->stop) CloseHandle(a->stop);
    a->thread=a->ready=a->go=a->stop=NULL;
}
