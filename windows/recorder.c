#define UNICODE
#define _UNICODE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include "region.h"
#include "audio.h"

static HWND window,statusLabel,timerLabel,startButton,stopButton,sourceBox,fpsBox,micBox,systemBox;
static HANDLE child=NULL,inputPipe=NULL;
static int stopping=0,closing=0,countdown=0,finalizing=0,muxing=0;
static int wantMic=0,wantSystem=0,recordingFailed=0,selecting=0;
static RECT captureRect;
static ULONGLONG savingSince=0;
static unsigned long long framesWritten=0;
static double videoSeconds=0;
static int recordingFps=15;
static DWORD lastWinError=0,lastExitCode=0;
static wchar_t session[4200],progressPath[4300];
static ULONGLONG started=0;
static AudioTrack mic={0},systemAudio={0};
static wchar_t appDir[MAX_PATH],ffmpeg[MAX_PATH],output[4096],logfile[4200];
static wchar_t videoPath[4200],micPath[4200],systemPath[4200],muxPath[4200];
static const wchar_t mixFilter[]=L"[1:a]aresample=48000:async=1:first_pts=0,aformat=channel_layouts=stereo[m];[2:a]aresample=48000:async=1:first_pts=0,aformat=channel_layouts=stereo[s];[m][s]amix=inputs=2:duration=longest:dropout_transition=0:normalize=0,alimiter=limit=0.95:level=0:latency=1";
static const wchar_t singleFilter[]=L"[1:a]aresample=48000:async=1:first_pts=0,aformat=channel_layouts=stereo";
#include "ui.h"
#include "windowpicker.h"
#include "gallery.h"
static void status(const wchar_t *s) { SetWindowTextW(statusLabel,s); }
static void controls(int idle) {
    uiIdle=idle;for(int i=0;i<4;i++)if(uiMode[i])EnableWindow(uiMode[i],idle);
    if(uiGainSlider)EnableWindow(uiGainSlider,SendMessageW(micBox,BM_GETCHECK,0,0)==BST_CHECKED);
    EnableWindow(startButton,idle); EnableWindow(sourceBox,idle); EnableWindow(fpsBox,idle);
    EnableWindow(micBox,idle); EnableWindow(systemBox,idle);
    EnableWindow(stopButton,!idle && (child || finalizing));
    SetWindowTextW(stopButton,(stopping || muxing || finalizing)?L"Отменить сохранение":L"■ Остановить");
}
static void error(const wchar_t *msg) { MessageBoxW(window,msg,L"Запись экрана",MB_OK|MB_ICONERROR); }
static void stopAudio(void) { UINT64 now=audio_clock(); audio_stop(&mic,now); audio_stop(&systemAudio,now); }
static int audioFinished(void) { return audio_done(&mic) && audio_done(&systemAudio); }
static void releaseAudio(void) { audio_release(&mic); audio_release(&systemAudio); }
static void progress(void) {
    FILE *f=_wfopen(progressPath,L"rb"); if(!f) return;
    _fseeki64(f,0,SEEK_END); long long size=_ftelli64(f); _fseeki64(f,size>8192?size-8192:0,SEEK_SET);
    char buf[8193]; size_t n=fread(buf,1,8192,f);buf[n]=0;fclose(f);
    for(char *line=buf;line && *line;) {
        if(!strncmp(line,"frame=",6)) { unsigned long long count=strtoull(line+6,NULL,10);if(count>framesWritten)framesWritten=count; }
        if(!strncmp(line,"out_time_us=",12)) { double t=(double)strtoull(line+12,NULL,10)/1000000.0+1.0/recordingFps;if(t>videoSeconds)videoSeconds=t; }
        char *end=strchr(line,'\n');line=end?end+1:NULL;
    }
    if(framesWritten && (double)framesWritten/recordingFps>videoSeconds) videoSeconds=(double)framesWritten/recordingFps;
}
static int bundledFFmpeg(void) {
    HRSRC resource=FindResourceW(NULL,MAKEINTRESOURCEW(101),RT_RCDATA);
    HGLOBAL loaded=resource?LoadResource(NULL,resource):NULL;
    DWORD size=resource?SizeofResource(NULL,resource):0; const BYTE *data=loaded?LockResource(loaded):NULL;
    if(!data || !size) { error(L"Не найден встроенный FFmpeg. Скачайте приложение целиком ещё раз."); return 0; }
    WIN32_FILE_ATTRIBUTE_DATA info;
    if(GetFileAttributesExW(ffmpeg,GetFileExInfoStandard,&info) && !info.nFileSizeHigh && info.nFileSizeLow==size) return 1;
    status(L"Подготовка встроенного FFmpeg… Интернет не нужен.");UpdateWindow(window);
    wchar_t tmp[MAX_PATH];swprintf(tmp,MAX_PATH,L"%ls.tmp",ffmpeg);
    HANDLE f=CreateFileW(tmp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE) { error(L"Нет доступа к папке компонентов приложения в LOCALAPPDATA.");return 0; }
    DWORD at=0;int ok=1;
    while(at<size) { DWORD part=min(size-at,1024*1024),written=0;
        if(!WriteFile(f,data+at,part,&written,NULL) || written!=part) { ok=0;break; }at+=written;
    }
    if(ok)ok=FlushFileBuffers(f);CloseHandle(f);
    if(ok)ok=MoveFileExW(tmp,ffmpeg,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok) { DeleteFileW(tmp);error(L"Не удалось сохранить встроенный FFmpeg. Проверьте свободное место."); }
    return ok;
}
static void details(void) {
    wchar_t message[10000],tail[6000]=L"";
    FILE *f=_wfopen(logfile,L"rb");
    if(f) { _fseeki64(f,0,SEEK_END);long long n=_ftelli64(f);_fseeki64(f,n>5000?n-5000:0,SEEK_SET);
        char bytes[5001];size_t len=fread(bytes,1,5000,f);bytes[len]=0;fclose(f);
        MultiByteToWideChar(CP_UTF8,0,bytes,-1,tail,6000);
    }
    swprintf(message,10000,L"Не удалось завершить запись.\nFFmpeg: %lu; Windows: %lu\nМикрофон: 0x%08lx; системный звук: 0x%08lx\n\nПапка диагностики:\n%ls\n\n%ls",lastExitCode,lastWinError,(unsigned long)mic.error,(unsigned long)systemAudio.error,session,tail);
    error(message);
}
static int prepareTrack(AudioTrack *track,int loopback,const wchar_t *path,int *enabled) {
    if(!*enabled || audio_prepare(track,loopback,path)) return 1;
    audio_stop(track,audio_clock()); ShowWindow(window,SW_RESTORE);
    wchar_t text[500];swprintf(text,500,L"%ls недоступен (0x%08lx).\n\nПродолжить запись без этого источника звука?",loopback?L"Звук компьютера":L"Микрофон",(unsigned long)track->error);
    if(MessageBoxW(window,text,L"Источник звука",MB_YESNO|MB_ICONQUESTION)==IDYES) { *enabled=0;return 1; } return 0;
}
static void abortSaving(int exitApp) {
    if(MessageBoxW(window,L"Прервать сохранение? Доступные временные файлы останутся в папке диагностики. Последние секунды могут быть потеряны.",L"Прервать сохранение",MB_YESNO|MB_ICONQUESTION)!=IDYES) return;
    stopAudio();
    if(child) { TerminateProcess(child,2);WaitForSingleObject(child,1000);CloseHandle(child);child=NULL; }
    if(inputPipe) { CloseHandle(inputPipe);inputPipe=NULL; }
    if(exitApp) { DestroyWindow(window);return; }
    recordingFailed=1;finalizing=1;muxing=0;stopping=1;status(L"Отменяем сохранение. Временные файлы сохранены.");
}
static void stop(void) {
    if(stopping || muxing || finalizing) { abortSaving(0);return; }
    if(!child) return;
    uiIndicator(0);stopping=1;savingSince=GetTickCount64();controls(0); status(L"Сохраняем видео и звук… Дождитесь завершения."); stopAudio();
    if(inputPipe) { DWORD n; WriteFile(inputPipe,"q\n",2,&n,NULL); CloseHandle(inputPipe); inputPipe=NULL; }
}
static int launch(wchar_t *cmd,int recording) {
    SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE}; HANDLE readPipe=NULL;
    if(recording) {
        if(!CreatePipe(&readPipe,&inputPipe,&sa,0)) return 0;
        SetHandleInformation(inputPipe,HANDLE_FLAG_INHERIT,0);
    } else readPipe=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
    HANDLE log=CreateFileW(logfile,FILE_APPEND_DATA,FILE_SHARE_READ,&sa,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(log==INVALID_HANDLE_VALUE || !readPipe || readPipe==INVALID_HANDLE_VALUE) {
        if(log!=INVALID_HANDLE_VALUE) CloseHandle(log);
        if(readPipe && readPipe!=INVALID_HANDLE_VALUE) CloseHandle(readPipe);
        if(inputPipe) { CloseHandle(inputPipe); inputPipe=NULL; } return 0;
    }
    STARTUPINFOW si={0}; si.cb=sizeof(si); si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdInput=readPipe; si.hStdOutput=log; si.hStdError=log;
    PROCESS_INFORMATION pi={0};
    BOOL ok=CreateProcessW(ffmpeg,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,appDir,&si,&pi);
    if(!ok)lastWinError=GetLastError();
    CloseHandle(readPipe); CloseHandle(log);
    if(!ok) { if(inputPipe) { CloseHandle(inputPipe); inputPipe=NULL; } return 0; }
    child=pi.hProcess; CloseHandle(pi.hThread); return 1;
}
static void finish(int success) {
    uiIndicator(0);releaseAudio(); finalizing=muxing=stopping=0;
    ShowWindow(window,SW_RESTORE); SetForegroundWindow(window); controls(1);
    if(success) {
        DeleteFileW(videoPath); DeleteFileW(micPath); DeleteFileW(systemPath); DeleteFileW(muxPath); DeleteFileW(logfile);
        status(L"Видео со звуком сохранено. «Показать видео» откроет папку.");
        if(!wantMic && !wantSystem) status(L"Видео сохранено без звука.");
    } else {
        FILE *log=_wfopen(logfile,L"a");
        if(log) { fprintf(log,"\nMicrophone HRESULT: 0x%08lx\nSystem audio HRESULT: 0x%08lx\n",(unsigned long)mic.error,(unsigned long)systemAudio.error); fclose(log); }
        status(L"Ошибка. Временные видео и аудио сохранены рядом с MP4.");
        progress();
        if(framesWritten && GetFileAttributesW(videoPath)!=INVALID_FILE_ATTRIBUTES && GetFileAttributesW(output)==INVALID_FILE_ATTRIBUTES) {
            if(MessageBoxW(window,L"Не удалось собрать звук с видео. Сохранить доступное видео без звука? Исходные аудиофайлы останутся в папке диагностики.",L"Восстановление видео",MB_YESNO|MB_ICONQUESTION)==IDYES && CopyFileW(videoPath,output,TRUE)) status(L"Сохранено восстановленное видео без звука. Исходники и журнал сохранены.");
        }
        details();
    }
    if(success&&!closing)galShow(output);
    if(closing) DestroyWindow(window);
}
static void beginRecording(void) {
    int x=captureRect.left,y=captureRect.top;
    int w=captureRect.right-x,h=captureRect.bottom-y;
    int fpsValues[]={15,24,30,60},index=(int)SendMessageW(fpsBox,CB_GETCURSEL,0,0);
    if(index<0 || index>3) index=2;
    recordingFailed=0;framesWritten=0;videoSeconds=0;recordingFps=fpsValues[index];lastWinError=lastExitCode=0;
    if(!prepareTrack(&mic,0,micPath,&wantMic) || !prepareTrack(&systemAudio,1,systemPath,&wantSystem)) {
        recordingFailed=1;stopAudio();finalizing=1;controls(0);return;
    }
    InterlockedExchange(&mic.gain,uiGain);
    wchar_t input[256];if(SendMessageW(sourceBox,CB_GETCURSEL,0,0)==3){if(!IsWindow(uiPickedWindow)||IsIconic(uiPickedWindow)){recordingFailed=1;stopAudio();finalizing=1;return;}swprintf(input,256,L"-i hwnd=0x%llx",(unsigned long long)(UINT_PTR)uiPickedWindow);}else swprintf(input,256,L"-offset_x %d -offset_y %d -video_size %dx%d -i desktop",x,y,w,h);
    int maxW[]={1280,1280,1920,1920},maxH[]={720,720,1080,1080},crf[]={28,28,23,23};
    wchar_t cmd[16000];
    swprintf(cmd,16000,L"\"%ls\" -hide_banner -n -nostats -stats_period 0.25 -progress \"%ls\" -thread_queue_size 1024 -f gdigrab -framerate %d -draw_mouse 1 %ls -vf \"scale=w='min(%d,iw)':h='min(%d,ih)':force_original_aspect_ratio=decrease:force_divisible_by=2,setsar=1,pad=ceil(iw/2)*2:ceil(ih/2)*2,setpts=PTS-STARTPTS\" -r %d -fps_mode cfr -c:v libx264 -preset ultrafast -crf %d -g %d -pix_fmt yuv420p -flush_packets 1 -movflags +frag_keyframe+empty_moov+default_base_moof \"%ls\"",ffmpeg,progressPath,fpsValues[index],input,maxW[index],maxH[index],fpsValues[index],crf[index],fpsValues[index],videoPath);
    ShowWindow(window,SW_MINIMIZE);GdiFlush();Sleep(100);
    UINT64 origin=audio_clock();
    if(!launch(cmd,1)) { recordingFailed=1; stopAudio(); finalizing=1; return; }
    if(wantMic)audio_begin(&mic,origin);if(wantSystem)audio_begin(&systemAudio,origin);
    started=GetTickCount64(); stopping=0; controls(0); SetWindowTextW(timerLabel,L"00:00:00");
    status(L"Идёт запись. Верните окно и нажмите «Остановить»."); ShowWindow(window,SW_MINIMIZE);
}
static void selected(int x,int y,int w,int h,int accepted) {
    selecting=0;
    if(!accepted) { ShowWindow(window,SW_RESTORE);controls(1);status(L"Выделение отменено. Запись не началась.");return; }
    captureRect.left=x;captureRect.top=y;captureRect.right=x+w;captureRect.bottom=y+h;
    countdown=1;status(L"Область выбрана. Запускаем запись…");SetTimer(window,2,250,NULL);
}
static void start(void) {
    if(child || countdown || selecting || finalizing || !audioFinished()) return;
    releaseAudio(); ZeroMemory(&mic,sizeof(mic)); ZeroMemory(&systemAudio,sizeof(systemAudio));
    if(!bundledFFmpeg()) return;
    wchar_t videos[MAX_PATH]=L"",destination[MAX_PATH];
    if(FAILED(SHGetFolderPathW(NULL,CSIDL_MYVIDEO,NULL,SHGFP_TYPE_CURRENT,videos)) || !videos[0]) {
        if(!GetEnvironmentVariableW(L"USERPROFILE",videos,MAX_PATH-32))wcscpy(videos,appDir);
        wcscat(videos,L"\\Videos");CreateDirectoryW(videos,NULL);
    }
    swprintf(destination,MAX_PATH,L"%ls\\ScreenRecorder",videos);CreateDirectoryW(destination,NULL);
    SYSTEMTIME t;GetLocalTime(&t);
    swprintf(output,4096,L"%ls\\Запись_%04d-%02d-%02d_%02d-%02d-%02d_%llu.mp4",destination,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetTickCount64());
    ULONGLONG id=GetTickCount64();
    wchar_t sessions[MAX_PATH];swprintf(sessions,MAX_PATH,L"%ls\\Sessions",appDir);CreateDirectoryW(sessions,NULL);
    swprintf(session,4200,L"%ls\\%04d%02d%02d-%02d%02d%02d-%llu",sessions,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,id);
    if(!CreateDirectoryW(session,NULL)) { error(L"Не удалось создать папку временной записи.");return; }
    swprintf(videoPath,4200,L"%ls\\video.mp4",session);swprintf(micPath,4200,L"%ls\\mic.wav",session);
    swprintf(systemPath,4200,L"%ls\\system.wav",session);swprintf(muxPath,4200,L"%ls\\result.mp4",session);
    swprintf(logfile,4200,L"%ls\\recording.log",session);swprintf(progressPath,4300,L"%ls\\progress.txt",session);
    wantMic=SendMessageW(micBox,BM_GETCHECK,0,0)==BST_CHECKED;
    wantSystem=SendMessageW(systemBox,BM_GETCHECK,0,0)==BST_CHECKED;
    controls(0);EnableWindow(stopButton,FALSE);
    int source=(int)SendMessageW(sourceBox,CB_GETCURSEL,0,0);
    if(source==0) {
        selecting=1;ShowWindow(window,SW_HIDE);Sleep(150);
        if(!region_show(GetModuleHandleW(NULL),selected)) { selecting=0;ShowWindow(window,SW_RESTORE);controls(1);error(L"Не удалось открыть выделение области."); }
    } else if(source==3){
        uiPickedWindow=uiPickWindow();if(!uiPickedWindow){controls(1);status(L"Выбор окна отменён.");return;}RECT r;GetClientRect(uiPickedWindow,&r);selected(0,0,r.right,r.bottom,1);
    } else {
        captureRect.left=source==2?GetSystemMetrics(SM_XVIRTUALSCREEN):0;
        captureRect.top=source==2?GetSystemMetrics(SM_YVIRTUALSCREEN):0;
        captureRect.right=captureRect.left+GetSystemMetrics(source==2?SM_CXVIRTUALSCREEN:SM_CXSCREEN);
        captureRect.bottom=captureRect.top+GetSystemMetrics(source==2?SM_CYVIRTUALSCREEN:SM_CYSCREEN);
        selected(captureRect.left,captureRect.top,captureRect.right-captureRect.left,captureRect.bottom-captureRect.top,1);
    }
}
static void finalize(void) {
    if(!audioFinished()) return;
    releaseAudio();
    if(recordingFailed || (wantMic && mic.error) || (wantSystem && systemAudio.error)) { finish(0); return; }
    progress();
    if(!framesWritten || videoSeconds<=0) { finish(0);return; }
    wchar_t inputs[17000],cmd[30000];
    if(wantMic && wantSystem) swprintf(inputs,17000,L"-i \"%ls\" -i \"%ls\"",micPath,systemPath);
    else swprintf(inputs,17000,L"-i \"%ls\"",wantMic?micPath:systemPath);
    wchar_t filter[3000];swprintf(filter,3000,L"%ls,apad=whole_dur=%.6f,atrim=duration=%.6f[a]",wantMic&&wantSystem?mixFilter:singleFilter,videoSeconds,videoSeconds);
    if(wantMic || wantSystem)
        swprintf(cmd,30000,L"\"%ls\" -hide_banner -nostdin -n -i \"%ls\" %ls -filter_complex \"%ls\" -map 0:v:0 -map \"[a]\" -c:v copy -c:a aac -b:a 160k -ar 48000 -ac 2 -t %.6f -movflags +faststart \"%ls\"",ffmpeg,videoPath,inputs,filter,videoSeconds,muxPath);
    else swprintf(cmd,30000,L"\"%ls\" -hide_banner -nostdin -n -i \"%ls\" -map 0:v:0 -c:v copy -an -movflags +faststart \"%ls\"",ffmpeg,videoPath,muxPath);
    if(!launch(cmd,0)) { finish(0); return; }
    muxing=1; finalizing=0;savingSince=GetTickCount64();controls(0); status(L"Объединяем видео и звук… Дождитесь сообщения о сохранении.");
}
static void tick(void) {
    uiIndicator(child&&!muxing&&!stopping&&!finalizing&&framesWritten>0);
    if(uiReady){RECT meter={U(620)-uiScrollX,U(350)-uiScrollY,U(990)-uiScrollX,U(389)-uiScrollY};InvalidateRect(window,&meter,FALSE);}
    if((finalizing || muxing || stopping) && savingSince && GetTickCount64()-savingSince>60000) {
        status(L"Сохранение занимает больше минуты. Можно отменить его или закрыть окно.");savingSince=0;
    }
    if(finalizing) { finalize(); return; }
    if(!child) return;
    if(WaitForSingleObject(child,0)==WAIT_OBJECT_0) {
        DWORD code; GetExitCodeProcess(child,&code);lastExitCode=code; CloseHandle(child); child=NULL;
        if(inputPipe) { CloseHandle(inputPipe); inputPipe=NULL; }
        if(muxing) {
            WIN32_FILE_ATTRIBUTE_DATA info;
            int valid=code==0 && GetFileAttributesExW(muxPath,GetFileExInfoStandard,&info) && (info.nFileSizeHigh || info.nFileSizeLow>1024);
            finish(valid && CopyFileW(muxPath,output,TRUE));
        } else {
            progress();stopAudio(); recordingFailed=recordingFailed || code!=0; finalizing=1; stopping=1; controls(0);
            status(L"Завершаем запись звука…");
        }
    } else if(!muxing && !stopping) {
        if((wantMic && mic.error) || (wantSystem && systemAudio.error)) { recordingFailed=1; stop(); return; }
        progress();
        unsigned long s=(unsigned long)((GetTickCount64()-started)/1000);
        wchar_t text[100]; swprintf(text,100,L"%02lu:%02lu:%02lu",s/3600,s/60%60,s%60); SetWindowTextW(timerLabel,text);
    }
}
static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_PAINT:uiPaint();return 0;
    case WM_ERASEBKGND:return 1;
    case WM_DRAWITEM:if(uiDrawItem((DRAWITEMSTRUCT*)lp))return TRUE;break;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:SetTextColor((HDC)wp,UI_TEXT);SetBkColor((HDC)wp,UI_BG);return (LRESULT)uiBgBrush;
    case WM_SIZE:uiLayout(0);return 0;
    case WM_VSCROLL:uiScroll(0,LOWORD(wp),HIWORD(wp));return 0;
    case WM_MOUSEWHEEL:uiScroll(0,GET_WHEEL_DELTA_WPARAM(wp)>0?SB_LINEUP:SB_LINEDOWN,0);return 0;
    case WM_HSCROLL:
        if((HWND)lp==uiGainSlider){uiGain=(int)SendMessageW(uiGainSlider,TBM_GETPOS,0,0);InterlockedExchange(&mic.gain,uiGain);wchar_t text[32];swprintf(text,32,L"%d%%",uiGain);SetWindowTextW(uiGainLabel,text);uiSaveSettings();return 0;}
        uiScroll(1,LOWORD(wp),HIWORD(wp));return 0;
    case WM_DPICHANGED:uiDpi=HIWORD(wp);uiFonts();uiLayout(1);return 0;
    case WM_COMMAND:
        if(LOWORD(wp)==7){uiScaleMenu();return 0;}
        if(LOWORD(wp)>=20&&LOWORD(wp)<=23&&uiIdle){int map[]={0,3,1,2};SendMessageW(sourceBox,CB_SETCURSEL,map[LOWORD(wp)-20],0);for(int i=0;i<4;i++)InvalidateRect(uiMode[i],NULL,FALSE);InvalidateRect(window,NULL,FALSE);return 0;}
        if(LOWORD(wp)==4||LOWORD(wp)==5){HWND toggle=LOWORD(wp)==4?micBox:systemBox;SendMessageW(toggle,BM_SETCHECK,SendMessageW(toggle,BM_GETCHECK,0,0)==BST_CHECKED?BST_UNCHECKED:BST_CHECKED,0);EnableWindow(uiGainSlider,SendMessageW(micBox,BM_GETCHECK,0,0)==BST_CHECKED);return 0;}
        if(LOWORD(wp)==1) start(); if(LOWORD(wp)==2) stop();
        if(LOWORD(wp)==6) { ShellExecuteW(hwnd,L"open",L"explorer.exe",session[0]?session:appDir,NULL,SW_SHOWNORMAL); }
        if(LOWORD(wp)==3){if(bundledFFmpeg())galShow(output);return 0;}return 0;
    case WM_TIMER:
        if(wp==2) { KillTimer(hwnd,2);countdown=0;beginRecording(); } else tick();return 0;
    case WM_HOTKEY: if(wp==1)stop();return 0;
    case WM_CLOSE:
        if(finalizing || muxing || stopping) { abortSaving(1);return 0; }
        if(countdown) { KillTimer(hwnd,2);countdown=0;DestroyWindow(hwnd);return 0; }
        if(child) { if(MessageBoxW(hwnd,L"Сохранить видео и выйти?",L"Завершение записи",MB_YESNO|MB_ICONQUESTION)==IDYES) { closing=1; stop(); } return 0; }
        DestroyWindow(hwnd); return 0;
    case WM_DESTROY: uiSaveSettings();uiDestroy();KillTimer(hwnd,1);KillTimer(hwnd,2);UnregisterHotKey(hwnd,1); PostQuitMessage(0); return 0;
    } return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE prev,PWSTR line,int show) {
    typedef BOOL (WINAPI *DpiContext)(HANDLE);
    DpiContext dpi=(DpiContext)(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetProcessDpiAwarenessContext");
    if(!dpi || !dpi((HANDLE)(INT_PTR)-4))SetProcessDPIAware();
    HANDLE mutex=CreateMutexW(NULL,TRUE,L"Local\\ScreenRecorderNativeSession");
    if(GetLastError()==ERROR_ALREADY_EXISTS) { MessageBoxW(NULL,L"Приложение уже запущено. Проверьте панель задач.",L"Запись экрана",MB_OK); return 0; }
    if(!GetEnvironmentVariableW(L"LOCALAPPDATA",appDir,MAX_PATH-32)) return 1;
    wcscat(appDir,L"\\ScreenRecorder"); CreateDirectoryW(appDir,NULL); swprintf(ffmpeg,MAX_PATH,L"%ls\\ffmpeg-7.1-bundled.exe",appDir);
    CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX common={sizeof(common),ICC_BAR_CLASSES};InitCommonControlsEx(&common);uiReadSettings();uiBgBrush=CreateSolidBrush(UI_BG);uiSurfaceBrush=CreateSolidBrush(UI_SURFACE);
    WNDCLASSW wc={0}; wc.lpfnWndProc=proc; wc.hInstance=instance; wc.lpszClassName=L"ScreenRecorderNative";
    wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(201));wc.hbrBackground=uiBgBrush; wc.hCursor=LoadCursorW(NULL,IDC_ARROW); RegisterClassW(&wc);
    window=CreateWindowExW(0,wc.lpszClassName,L"Запись экрана 3.0 — новый интерфейс",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_HSCROLL|WS_VSCROLL,CW_USEDEFAULT,CW_USEDEFAULT,1040,800,NULL,NULL,instance,NULL);
    uiFontButton=uiAdd(L"Шрифт",890,14,100,32,7,L"BUTTON",WS_TABSTOP);
    sourceBox=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|CBS_DROPDOWNLIST,0,0,1,1,window,NULL,instance,NULL);
    const wchar_t *sources[]={L"Область",L"Основной монитор",L"Все мониторы",L"Окно"};for(int i=0;i<4;i++)SendMessageW(sourceBox,CB_ADDSTRING,0,(LPARAM)sources[i]);SendMessageW(sourceBox,CB_SETCURSEL,0,0);
    const wchar_t *modes[]={L"Область",L"Окно",L"Весь экран",L"Все мониторы",L"Несколько окон"};for(int i=0;i<5;i++)uiMode[i]=uiAdd(modes[i],30+194*i,186,184,48,20+i,L"BUTTON",WS_TABSTOP);EnableWindow(uiMode[4],FALSE);
    micBox=uiAdd(L"Микрофон",625,264,355,42,4,L"BUTTON",BS_AUTOCHECKBOX|WS_TABSTOP);
    systemBox=uiAdd(L"Звук компьютера",625,398,355,42,5,L"BUTTON",BS_AUTOCHECKBOX|WS_TABSTOP);
    SendMessageW(micBox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(systemBox,BM_SETCHECK,BST_CHECKED,0);
    uiGainSlider=uiAdd(L"",625,312,275,32,8,TRACKBAR_CLASSW,WS_TABSTOP|TBS_NOTICKS);SendMessageW(uiGainSlider,TBM_SETRANGE,TRUE,MAKELPARAM(0,100));SendMessageW(uiGainSlider,TBM_SETPOS,TRUE,uiGain);
    wchar_t gainText[32];swprintf(gainText,32,L"%d%%",uiGain);uiGainLabel=uiAdd(gainText,910,310,70,36,0,L"STATIC",SS_RIGHT|SS_CENTERIMAGE);
    fpsBox=uiAdd(L"",751,443,230,180,9,L"COMBOBOX",CBS_DROPDOWNLIST|WS_TABSTOP);const wchar_t *qualities[]={L"720p · 15 к/с",L"720p · 24 к/с",L"1080p · 30 к/с",L"1080p · 60 к/с"};for(int i=0;i<4;i++)SendMessageW(fpsBox,CB_ADDSTRING,0,(LPARAM)qualities[i]);SendMessageW(fpsBox,CB_SETCURSEL,0,0);
    statusLabel=uiAdd(L"Готово к записи. Выберите режим захвата.",30,522,760,48,0,L"STATIC",SS_LEFT);
    timerLabel=uiAdd(L"00:00:00",804,522,178,44,0,L"STATIC",SS_RIGHT|SS_CENTERIMAGE);
    startButton=uiAdd(L"Захватить экран",30,607,620,58,1,L"BUTTON",WS_TABSTOP);
    stopButton=uiAdd(L"■ Остановить",670,607,320,58,2,L"BUTTON",WS_TABSTOP);
    uiAdd(L"Галерея и обрезка",30,682,460,44,3,L"BUTTON",WS_TABSTOP);uiAdd(L"Открыть диагностику",510,682,480,44,6,L"BUTTON",WS_TABSTOP);
    uiReady=1;uiFonts();uiLayout(1);controls(1);RegisterHotKey(window,1,MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT,VK_F12);SetTimer(window,1,100,NULL);ShowWindow(window,show);UpdateWindow(window);
    MSG m;while(GetMessageW(&m,NULL,0,0)>0){if(!IsDialogMessageW(window,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}
    if(mutex)CloseHandle(mutex);CoUninitialize();return 0;
}
