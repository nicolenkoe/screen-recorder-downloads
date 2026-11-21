#ifndef RECORDER_GALLERY_EDITOR_H
#define RECORDER_GALLERY_EDITOR_H
#include "edit_model.h"
#include "pause_model.h"
#include <windowsx.h>
typedef struct {wchar_t name[260],path[4096];ULONGLONG size,date;} GalFile;
static GalFile *galFiles;static int galFileCount,galView=2,galSelecting=0,galBuilding=0;
static HWND galClock,galRuler,galFilm,galKeep,galRemove,galRangesList,galAddRange,galDropRange,galViewBox,galSelectButton,galDeleteButton,galActionsButton;
static HIMAGELIST galImages;static EditRange galRanges[EDIT_MAX_RANGES];static int galRangeCount=1,galActiveRange=0,galRemoveMode=0,galEditing=0;
static HWND galPauseButton,galPauseSeconds,galPauseMode;static int galProtectAudio=1;static int galAnalyzing=0;static double galPauseLimit=1;
static double galPosition=0,galExportLength=0;static int galSingleExport=0;
static wchar_t galFilterPath[4200],galProgressPath[4200];
static void galLoad(const wchar_t *path);
static void galRefresh(const wchar_t *selected);
static void galLayout(void);
static void galUpdateClock(double seconds);
static void galSyncRangeFields(void);
static void galBrowserSelection(void);
static LRESULT CALLBACK galProc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK galVideoProc(HWND,UINT,WPARAM,LPARAM);
static HWND galControl(const wchar_t *cls,const wchar_t *text,int id,DWORD style){
 if(!wcscmp(cls,L"BUTTON"))style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
 if(!wcscmp(cls,TRACKBAR_CLASSW))cls=L"RecorderSlider";
 HWND w=CreateWindowExW(!wcscmp(cls,L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,1,1,gal,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
 SendMessageW(w,WM_SETFONT,(WPARAM)uiFont,TRUE);return w;
}
static double galCurrentSeconds(void){
 if(galPlayer){PROPVARIANT p;PropVariantInit(&p);double at=galPosition;
  if(SUCCEEDED(IMFPMediaPlayer_GetPosition(galPlayer,&galPosition100ns,&p))){if(p.vt==VT_I8)at=p.hVal.QuadPart/10000000.;else if(p.vt==VT_UI8)at=p.uhVal.QuadPart/10000000.;}PropVariantClear(&p);return edit_clamp(at,galDuration);}
 double at=0;if(galFrameLockReady){EnterCriticalSection(&galFrameLock);at=galPreviewPosition;LeaveCriticalSection(&galFrameLock);}else at=galPreviewPosition;return edit_clamp(at,galDuration);
}
static void galUpdateClock(double seconds){
 galPosition=edit_clamp(seconds,galDuration);wchar_t a[64],b[64],text[200];edit_time(a,64,galPosition,1);edit_time(b,64,galDuration,1);
 swprintf(text,200,L"%ls / %ls   ·   %.3f с",a,b,galPosition);SetWindowTextW(galClock,text);
 if(!galDragging)SendMessageW(galSeek,TBM_SETPOS,TRUE,galDuration>0?(LPARAM)(galPosition*10000/galDuration):0);
 InvalidateRect(galFilm,NULL,FALSE);
}
static void galPositionTo(double at){
 at=edit_clamp(at,galDuration);if(galPlayer)IMFPMediaPlayer_Pause(galPlayer);galSeekTo(at>=galDuration&&galDuration>0?max(0,galDuration-.04):at);galUpdateClock(at);
}
static void galFillRanges(void){
 SendMessageW(galRangesList,LB_RESETCONTENT,0,0);
 for(int i=0;i<galRangeCount;i++){wchar_t a[64],b[64],line[180];edit_time(a,64,galRanges[i].start,1);edit_time(b,64,galRanges[i].end,1);swprintf(line,180,L"%d. %ls — %ls",i+1,a,b);SendMessageW(galRangesList,LB_ADDSTRING,0,(LPARAM)line);}
 SendMessageW(galRangesList,LB_SETCURSEL,galActiveRange,0);
}
static void galSyncRangeFields(void){
 galEditing=1;galSetSeconds(galFromEdit,galRanges[galActiveRange].start);galSetSeconds(galToEdit,galRanges[galActiveRange].end);galEditing=0;galFillRanges();InvalidateRect(galFilm,NULL,FALSE);
}
static void galSetMode(int remove){
 galRemoveMode=remove;SetWindowTextW(galKeep,L"Оставить выделенное");SetWindowTextW(galRemove,L"Удалить выделенное");SetPropW(galKeep,L"RecorderSelected",(HANDLE)(INT_PTR)!remove);SetPropW(galRemove,L"RecorderSelected",(HANDLE)(INT_PTR)remove);
 SetWindowTextW(galStatus,remove?L"Все отмеченные кусочки удалятся. Оставшиеся части склеятся.":L"Сохранятся отмеченные фрагменты. Исходник останется.");
 InvalidateRect(galKeep,NULL,FALSE);InvalidateRect(galRemove,NULL,FALSE);InvalidateRect(galFilm,NULL,FALSE);
}
typedef struct{HANDLE thread,stop,child;CRITICAL_SECTION lock;int initialized;ULONGLONG since;}GalImageJob;
static GalImageJob galFilmJob,galBrowserJob;
#define GAL_THUMBS 12
#define GAL_THUMB_W 128
#define GAL_THUMB_H 72
static BYTE galThumbs[GAL_THUMBS][GAL_THUMB_W*GAL_THUMB_H*4];static int galThumbReady[GAL_THUMBS];
typedef struct{int index;BYTE pixels[GAL_THUMB_W*GAL_THUMB_H*4];}GalBrowserImage;
static void galImageInit(GalImageJob *j){if(!j->initialized){InitializeCriticalSection(&j->lock);j->initialized=1;}}
static void galImageStop(GalImageJob *j){
 if(j->stop)SetEvent(j->stop);if(j->initialized){EnterCriticalSection(&j->lock);if(j->child)TerminateProcess(j->child,2);LeaveCriticalSection(&j->lock);}
 if(j->thread){WaitForSingleObject(j->thread,INFINITE);CloseHandle(j->thread);}if(j->stop)CloseHandle(j->stop);j->thread=j->stop=NULL;
}
static int galReadThumb(GalImageJob *job,const wchar_t *path,double at,BYTE *pixels){
 if(WaitForSingleObject(job->stop,0)==WAIT_OBJECT_0)return 0;
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE read=NULL,write=NULL;if(!CreatePipe(&read,&write,&sa,65536))return 0;SetHandleInformation(read,HANDLE_FLAG_INHERIT,0);
 HANDLE nul=CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
 wchar_t cmd[12000];swprintf(cmd,12000,L"\"%ls\" -hide_banner -loglevel error -nostdin -threads 2 -ss %.6f -i \"%ls\" -frames:v 1 -an -vf \"scale=128:72:force_original_aspect_ratio=decrease,pad=128:72:(ow-iw)/2:(oh-ih)/2\" -pix_fmt bgr0 -f rawvideo pipe:1",ffmpeg,at,path);
 STARTUPINFOW si={0};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=si.hStdError=nul;si.hStdOutput=write;PROCESS_INFORMATION p={0};
 int ok=CreateProcessW(ffmpeg,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,appDir,&si,&p);CloseHandle(write);CloseHandle(nul);if(!ok){CloseHandle(read);return 0;}appOwnProcess(p.hProcess);CloseHandle(p.hThread);
 EnterCriticalSection(&job->lock);job->child=p.hProcess;job->since=GetTickCount64();if(WaitForSingleObject(job->stop,0)==WAIT_OBJECT_0)TerminateProcess(job->child,2);LeaveCriticalSection(&job->lock);
 DWORD total=0,n=0,bytes=GAL_THUMB_W*GAL_THUMB_H*4;
 while(total<bytes&&ReadFile(read,pixels+total,bytes-total,&n,NULL)&&n)total+=n;
 if(WaitForSingleObject(p.hProcess,3000)!=WAIT_OBJECT_0)TerminateProcess(p.hProcess,2);
 EnterCriticalSection(&job->lock);job->child=NULL;CloseHandle(p.hProcess);LeaveCriticalSection(&job->lock);CloseHandle(read);
 if(total!=bytes)return 0;for(DWORD i=3;i<bytes;i+=4)pixels[i]=255;return 1;
}
static void galImageWatch(GalImageJob *job){if(job->initialized){EnterCriticalSection(&job->lock);if(job->child&&GetTickCount64()-job->since>10000)TerminateProcess(job->child,2);LeaveCriticalSection(&job->lock);}}
static DWORD WINAPI galFilmWorker(LPVOID context){
 BYTE pixels[GAL_THUMB_W*GAL_THUMB_H*4];
 for(int i=0;i<GAL_THUMBS&&WaitForSingleObject(galFilmJob.stop,0)!=WAIT_OBJECT_0;i++){
  double at=galDuration>0?galDuration*(i+.5)/GAL_THUMBS:0;
  if(galReadThumb(&galFilmJob,galInput,at,pixels)){EnterCriticalSection(&galFilmJob.lock);memcpy(galThumbs[i],pixels,sizeof(pixels));galThumbReady[i]=1;LeaveCriticalSection(&galFilmJob.lock);PostMessageW(gal,WM_APP+54,0,0);}
 }return 0;
}
static DWORD WINAPI galBrowserWorker(LPVOID context){
 for(int i=0;i<galFileCount&&WaitForSingleObject(galBrowserJob.stop,0)!=WAIT_OBJECT_0;i++){
  GalBrowserImage *image=HeapAlloc(GetProcessHeap(),0,sizeof(*image));if(!image)break;image->index=i;
  if(galReadThumb(&galBrowserJob,galFiles[i].path,0,image->pixels)&&WaitForSingleObject(galBrowserJob.stop,0)!=WAIT_OBJECT_0){if(!PostMessageW(gal,WM_APP+55,0,(LPARAM)image))HeapFree(GetProcessHeap(),0,image);}else HeapFree(GetProcessHeap(),0,image);
 }return 0;
}
static void galFilmStart(void){galImageStop(&galFilmJob);galImageInit(&galFilmJob);ZeroMemory(galThumbReady,sizeof(galThumbReady));galFilmJob.stop=CreateEventW(NULL,TRUE,FALSE,NULL);galFilmJob.thread=CreateThread(NULL,0,galFilmWorker,NULL,0,NULL);}
static void galBrowserStop(void){galImageStop(&galBrowserJob);MSG m;while(PeekMessageW(&m,gal,WM_APP+55,WM_APP+55,PM_REMOVE))HeapFree(GetProcessHeap(),0,(void*)m.lParam);}
static HBITMAP galImageBitmap(const BYTE *pixels){BITMAPINFO b={0};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=GAL_THUMB_W;b.bmiHeader.biHeight=-GAL_THUMB_H;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;void *bits=NULL;HDC dc=GetDC(NULL);HBITMAP bitmap=CreateDIBSection(dc,&b,DIB_RGB_COLORS,&bits,NULL,0);ReleaseDC(NULL,dc);if(bitmap&&bits)memcpy(bits,pixels,GAL_THUMB_W*GAL_THUMB_H*4);return bitmap;}
static void galTint(HDC dc,const RECT *r,COLORREF color,BYTE alpha){
 HDC src=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,1,1);HGDIOBJ old=SelectObject(src,b);SetPixel(src,0,0,color);BLENDFUNCTION blend={AC_SRC_OVER,0,alpha,0};AlphaBlend(dc,r->left,r->top,r->right-r->left,r->bottom-r->top,src,0,0,1,1,blend);SelectObject(src,old);DeleteObject(b);DeleteDC(src);
}
static void galDrawTicks(HDC dc,RECT r,int ends){
 SetBkMode(dc,TRANSPARENT);SetTextColor(dc,UI_MUTED);SelectObject(dc,uiSmallFont);int pad=D(12),width=max(1,r.right-pad*2);
 double step=edit_tick_step(galDuration,max(1,width/D(85)));
 HPEN pen=CreatePen(PS_SOLID,1,UI_LINE);HGDIOBJ old=SelectObject(dc,pen);
 if(galDuration>0)for(double t=0;t<galDuration;t+=step){int x=pad+(int)(width*t/galDuration);if(t>0&&r.right-pad-x<D(72))continue;MoveToEx(dc,x,0,NULL);LineTo(dc,x,D(5));wchar_t s[64];edit_time(s,64,t,0);RECT text={x-D(40),D(5),x+D(45),r.bottom};if(t==0){text.left=pad;text.right=pad+D(85);}DrawTextW(dc,s,-1,&text,(t==0?DT_LEFT:DT_CENTER)|DT_SINGLELINE);}
 if(ends&&galDuration>0){wchar_t s[64];edit_time(s,64,galDuration,0);RECT text={r.right-pad-D(90),D(5),r.right-pad,r.bottom};DrawTextW(dc,s,-1,&text,DT_RIGHT|DT_SINGLELINE);MoveToEx(dc,r.right-pad,0,NULL);LineTo(dc,r.right-pad,D(5));}
 SelectObject(dc,old);DeleteObject(pen);
}
static LRESULT CALLBACK galRulerProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);RECT r;GetClientRect(w,&r);FillRect(dc,&r,uiBgBrush);galDrawTicks(dc,r,1);EndPaint(w,&p);return 0;}return DefWindowProcW(w,msg,wp,lp);
}
static int galFilmDrag=0,galFilmHandle=0;
static int galFilmX(double seconds,int width){return D(15)+(galDuration>0?(int)((width-D(30))*seconds/galDuration):0);}
static LRESULT CALLBACK galFilmProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_CONTEXTMENU)return SendMessageW(gal,WM_CONTEXTMENU,(WPARAM)w,lp);
 if(msg==WM_GETDLGCODE)return DLGC_WANTARROWS;
 if(msg==WM_LBUTTONDOWN&&galDuration>0&&!galJob){
  RECT r;GetClientRect(w,&r);int x=GET_X_LPARAM(lp),a=galFilmX(galRanges[galActiveRange].start,r.right),b=galFilmX(galRanges[galActiveRange].end,r.right);
  SetFocus(w);if(abs(x-a)<=D(17)||abs(x-b)<=D(17)){galFilmHandle=abs(x-a)<=abs(x-b)?0:1;galFilmDrag=1;SetCapture(w);}
  else galPositionTo(galDuration*max(0,min(x-D(15),r.right-D(30)))/max(1,r.right-D(30)));
  return 0;
 }
 if(msg==WM_MOUSEMOVE&&galFilmDrag){
  RECT r;GetClientRect(w,&r);double at=edit_clamp(galDuration*(GET_X_LPARAM(lp)-D(15))/max(1,r.right-D(30)),galDuration),gap=min(.05,galDuration/100.);
  EditRange *range=&galRanges[galActiveRange];if(galFilmHandle==0)range->start=min(at,range->end-gap);else range->end=max(at,range->start+gap);galSyncRangeFields();return 0;
 }
 if(msg==WM_LBUTTONUP&&galFilmDrag){galFilmDrag=0;ReleaseCapture();galPositionTo(galFilmHandle?galRanges[galActiveRange].end:galRanges[galActiveRange].start);return 0;}
 if(msg==WM_CAPTURECHANGED){galFilmDrag=0;return 0;}
 if(msg==WM_KEYDOWN&&galDuration>0&&!galJob){
  if(wp==VK_TAB){galFilmHandle=!galFilmHandle;return 0;}
  if(wp==VK_LEFT||wp==VK_RIGHT){double step=GetKeyState(VK_SHIFT)<0?1:.1;EditRange *r=&galRanges[galActiveRange];double at=(galFilmHandle?r->end:r->start)+(wp==VK_RIGHT?step:-step),gap=min(.05,galDuration/100.);
   if(galFilmHandle)r->end=min(galDuration,max(r->start+gap,at));else r->start=max(0,min(r->end-gap,at));galSyncRangeFields();galPositionTo(galFilmHandle?r->end:r->start);return 0;}
 }
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){
  PAINTSTRUCT paint;HDC dc=BeginPaint(w,&paint);RECT r;GetClientRect(w,&r);FillRect(dc,&r,uiBgBrush);int left=D(15),width=max(1,r.right-D(30)),top=D(10),height=D(72);
  BITMAPINFO b={0};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=GAL_THUMB_W;b.bmiHeader.biHeight=-GAL_THUMB_H;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;
  for(int i=0;i<GAL_THUMBS;i++){
   RECT cell={left+width*i/GAL_THUMBS,top,left+width*(i+1)/GAL_THUMBS,top+height};FillRect(dc,&cell,uiSurfaceBrush);
   EnterCriticalSection(&galFilmJob.lock);if(galThumbReady[i])StretchDIBits(dc,cell.left,cell.top,cell.right-cell.left,height,0,0,GAL_THUMB_W,GAL_THUMB_H,galThumbs[i],&b,DIB_RGB_COLORS,SRCCOPY);LeaveCriticalSection(&galFilmJob.lock);
  }
  EditRange kept[EDIT_MAX_RANGES+1];int n=edit_plan(galRanges,galRangeCount,galDuration,galRemoveMode,kept);
  RECT strip={left,top,left+width,top+height};galTint(dc,&strip,RGB(0,0,0),150);
  /* Restore the brightness of kept spans from the source frames. */
  for(int k=0;k<n;k++){int a=galFilmX(kept[k].start,r.right),z=galFilmX(kept[k].end,r.right);int saved=SaveDC(dc);IntersectClipRect(dc,a,top,z,top+height);
   for(int i=0;i<GAL_THUMBS;i++){EnterCriticalSection(&galFilmJob.lock);if(galThumbReady[i])StretchDIBits(dc,left+width*i/GAL_THUMBS,top,width*(i+1)/GAL_THUMBS-width*i/GAL_THUMBS,height,0,0,GAL_THUMB_W,GAL_THUMB_H,galThumbs[i],&b,DIB_RGB_COLORS,SRCCOPY);LeaveCriticalSection(&galFilmJob.lock);}RestoreDC(dc,saved);
  }
  for(int i=0;i<galRangeCount;i++){RECT area={galFilmX(galRanges[i].start,r.right),top,galFilmX(galRanges[i].end,r.right),top+height};COLORREF color=galRemoveMode?RGB(255,65,85):UI_MINT;galTint(dc,&area,color,galRemoveMode?95:32);HPEN p=CreatePen(PS_SOLID,i==galActiveRange?D(3):1,color);HGDIOBJ op=SelectObject(dc,p),ob=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,area.left,area.top,area.right,area.bottom);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(p);}
  for(int h=0;h<2;h++){int x=galFilmX(h?galRanges[galActiveRange].end:galRanges[galActiveRange].start,r.right);RECT handle={x-D(7),top-D(2),x+D(7),top+height+D(2)};HBRUSH brush=CreateSolidBrush(galRemoveMode?RGB(255,85,101):UI_MINT);FillRect(dc,&handle,brush);DeleteObject(brush);HPEN p=CreatePen(PS_SOLID,D(2),UI_BG);HGDIOBJ old=SelectObject(dc,p);for(int j=-1;j<=1;j+=2){MoveToEx(dc,x+j*D(2),top+D(26),NULL);LineTo(dc,x+j*D(2),top+D(48));}SelectObject(dc,old);DeleteObject(p);}
  int x=galFilmX(galPosition,r.right);HPEN p=CreatePen(PS_SOLID,D(2),UI_TEXT);HGDIOBJ old=SelectObject(dc,p);MoveToEx(dc,x,top-D(5),NULL);LineTo(dc,x,top+height+D(4));SelectObject(dc,old);DeleteObject(p);
  SetTextColor(dc,UI_MUTED);SetBkMode(dc,TRANSPARENT);SelectObject(dc,uiSmallFont);RECT note={D(15),top+height+D(8),r.right-D(15),r.bottom};DrawTextW(dc,L"Потяните края · щелчок по кадру — перемотка · стрелки — точная подстройка",-1,&note,DT_CENTER|DT_SINGLELINE);EndPaint(w,&paint);return 0;
 }
 return DefWindowProcW(w,msg,wp,lp);
}
#define GAL_MAX_IMPORTS 512
static wchar_t galImports[GAL_MAX_IMPORTS][4096];static int galImportCount=0;
static void galImportsFile(int save){
 wchar_t path[4200];swprintf(path,4200,L"%ls\\gallery-imports.txt",appDir);FILE *f=_wfopen(path,save?L"wb":L"rb");if(!f)return;
 if(save){fwrite(&galImportCount,sizeof(int),1,f);for(int i=0;i<galImportCount;i++)fwrite(galImports[i],sizeof(galImports[i]),1,f);}
 else{int count=0;if(fread(&count,sizeof(count),1,f)==1&&count>=0&&count<=GAL_MAX_IMPORTS){galImportCount=0;for(int i=0;i<count;i++){if(fread(galImports[i],sizeof(galImports[i]),1,f)!=1)break;galImports[i][4095]=0;galImportCount++;}}}
 fclose(f);
}
static int galFileOrder(const void *a,const void *b){const GalFile *x=a,*y=b;return x->date<y->date?1:x->date>y->date?-1:_wcsicmp(x->name,y->name);}
static void galAddFile(const wchar_t *path){
 for(int i=0;i<galFileCount;i++)if(!_wcsicmp(galFiles[i].path,path))return;
 WIN32_FILE_ATTRIBUTE_DATA info;if(!GetFileAttributesExW(path,GetFileExInfoStandard,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))return;
 GalFile *next=galFiles?HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,galFiles,(galFileCount+1)*sizeof(*galFiles)):HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*galFiles));if(!next)return;galFiles=next;
 GalFile *file=&galFiles[galFileCount++];wcsncpy(file->path,path,4095);const wchar_t *name=wcsrchr(path,L'\\');wcsncpy(file->name,name?name+1:path,259);file->size=((ULONGLONG)info.nFileSizeHigh<<32)|info.nFileSizeLow;file->date=((ULONGLONG)info.ftLastWriteTime.dwHighDateTime<<32)|info.ftLastWriteTime.dwLowDateTime;
}
static int galTargets(int *indices,int only){
 if(only>=0&&only<galFileCount){indices[0]=only;return 1;}
 int n=0;for(int i=0;i<galFileCount;i++)if(galSelecting?ListView_GetCheckState(galList,i):(ListView_GetItemState(galList,i,LVIS_SELECTED)&LVIS_SELECTED))indices[n++]=i;return n;
}
static void galBrowserSelection(void){
 int n=0;for(int i=0;i<galFileCount;i++)if(galSelecting?ListView_GetCheckState(galList,i):(ListView_GetItemState(galList,i,LVIS_SELECTED)&LVIS_SELECTED))n++;
 wchar_t label[100];swprintf(label,100,L"Действия (%d)",n);SetWindowTextW(galActionsButton,label);EnableWindow(galDeleteButton,n>0&&!galJob);EnableWindow(galActionsButton,n>0&&!galJob);
}
static int galImported(const wchar_t *path){for(int i=0;i<galImportCount;i++)if(!_wcsicmp(path,galImports[i]))return i;return -1;}
static void galForgetImport(const wchar_t *path){int i=galImported(path);if(i>=0){for(int j=i;j<galImportCount-1;j++)wcscpy(galImports[j],galImports[j+1]);galImportCount--;}}
static void galDeleteFiles(int only,int forget){
 if(galJob||galFileCount<1)return;int *indices=HeapAlloc(GetProcessHeap(),0,galFileCount*sizeof(int));if(!indices)return;int n=galTargets(indices,only);if(!n){HeapFree(GetProcessHeap(),0,indices);return;}
 wchar_t question[800];swprintf(question,800,forget?L"Убрать из галереи выбранные видео (%d)? Внешние файлы останутся на месте.":L"Удалить выбранные файлы (%d)?\n\nWindows использует корзину, если она поддерживается. Для внешних видео это удаление оригинала. Windows может запросить дополнительное подтверждение.",n);
 if(MessageBoxW(gal,question,forget?L"Убрать из галереи":L"Удаление файлов",MB_YESNO|MB_ICONQUESTION)!=IDYES){HeapFree(GetProcessHeap(),0,indices);return;}
 galBrowserStop();galImageStop(&galFilmJob);galUnload();
 if(forget){for(int i=0;i<n;i++)galForgetImport(galFiles[indices[i]].path);galImportsFile(1);}
 else{
  size_t length=1;for(int i=0;i<n;i++)length+=wcslen(galFiles[indices[i]].path)+1;wchar_t *paths=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,length*sizeof(wchar_t));size_t at=0;
  if(paths){for(int i=0;i<n;i++){wcscpy(paths+at,galFiles[indices[i]].path);at+=wcslen(paths+at)+1;}SHFILEOPSTRUCTW operation={0};operation.hwnd=gal;operation.wFunc=FO_DELETE;operation.pFrom=paths;operation.fFlags=FOF_ALLOWUNDO|FOF_WANTNUKEWARNING;SHFileOperationW(&operation);HeapFree(GetProcessHeap(),0,paths);}
 }
 HeapFree(GetProcessHeap(),0,indices);galRefresh(NULL);
}
static HWND galRenameDialog,galRenameEdit;static int galRenameAccepted;
static LRESULT CALLBACK galRenameProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_COMMAND&&(LOWORD(wp)==1||LOWORD(wp)==2)){galRenameAccepted=LOWORD(wp)==1;ShowWindow(w,SW_HIDE);return 0;}
 if(msg==WM_CLOSE){galRenameAccepted=0;ShowWindow(w,SW_HIDE);return 0;}
 if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLOREDIT){SetTextColor((HDC)wp,UI_TEXT);SetBkColor((HDC)wp,UI_BG);return (LRESULT)uiBgBrush;}return DefWindowProcW(w,msg,wp,lp);
}
static void galRenameFile(int only){
 if(galJob||galFileCount<1)return;int *indices=HeapAlloc(GetProcessHeap(),0,galFileCount*sizeof(int));int n=indices?galTargets(indices,only):0;if(n!=1){if(indices)HeapFree(GetProcessHeap(),0,indices);return;}int index=indices[0];HeapFree(GetProcessHeap(),0,indices);
 WNDCLASSW cls={0};cls.lpfnWndProc=galRenameProc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"RecorderRename";cls.hbrBackground=uiBgBrush;cls.hCursor=LoadCursorW(NULL,IDC_ARROW);RegisterClassW(&cls);
 RECT r;GetWindowRect(gal,&r);galRenameDialog=CreateWindowExW(WS_EX_DLGMODALFRAME,cls.lpszClassName,L"Переименовать видео",WS_CAPTION|WS_SYSMENU|WS_POPUP,r.left+D(60),r.top+D(100),D(620),D(180),gal,NULL,cls.hInstance,NULL);uiDarkTitleBar(galRenameDialog);
 galRenameEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",galFiles[index].name,WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,D(18),D(20),D(576),D(40),galRenameDialog,NULL,cls.hInstance,NULL);SendMessageW(galRenameEdit,WM_SETFONT,(WPARAM)uiFont,TRUE);
 HWND ok=CreateWindowExW(0,L"BUTTON",L"Сохранить",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,D(18),D(85),D(280),D(40),galRenameDialog,(HMENU)1,cls.hInstance,NULL),cancel=CreateWindowExW(0,L"BUTTON",L"Отмена",WS_CHILD|WS_VISIBLE|WS_TABSTOP,D(310),D(85),D(280),D(40),galRenameDialog,(HMENU)2,cls.hInstance,NULL);SendMessageW(ok,WM_SETFONT,(WPARAM)uiFont,TRUE);SendMessageW(cancel,WM_SETFONT,(WPARAM)uiFont,TRUE);
 EnableWindow(gal,FALSE);galRenameAccepted=0;ShowWindow(galRenameDialog,SW_SHOW);SetFocus(galRenameEdit);MSG msg;
 while(IsWindowVisible(galRenameDialog)&&GetMessageW(&msg,NULL,0,0)>0){if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){galRenameAccepted=0;break;}if(!IsDialogMessageW(galRenameDialog,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 wchar_t name[260];GetWindowTextW(galRenameEdit,name,260);DestroyWindow(galRenameDialog);EnableWindow(gal,TRUE);SetForegroundWindow(gal);if(!galRenameAccepted)return;
 if(!*name||wcspbrk(name,L"\\/:*?\"<>|")||name[wcslen(name)-1]==L'.'||name[wcslen(name)-1]==L' '){MessageBoxW(gal,L"Введите имя без символов \\ / : * ? \" < > |",L"Имя файла",MB_OK);return;}
 const wchar_t *extension=wcsrchr(galFiles[index].name,L'.');if(extension){size_t length=wcslen(name),extlen=wcslen(extension);if(length<extlen||_wcsicmp(name+length-extlen,extension)){if(length+extlen>=260){MessageBoxW(gal,L"Имя слишком длинное.",L"Имя файла",MB_OK);return;}wcscat(name,extension);}}
 wchar_t path[4096];wcscpy(path,galFiles[index].path);wchar_t *base=wcsrchr(path,L'\\');if(!base)return;wcsncpy(base+1,name,4095-(base-path)-1);path[4095]=0;
 galBrowserStop();galImageStop(&galFilmJob);galUnload();
 if(MoveFileW(galFiles[index].path,path)){int imported=galImported(galFiles[index].path);if(imported>=0){wcscpy(galImports[imported],path);galImportsFile(1);}galRefresh(path);}
 else{MessageBoxW(gal,L"Не удалось переименовать файл. Проверьте имя и отсутствие файла с таким же именем.",L"Переименование",MB_OK|MB_ICONERROR);galRefresh(NULL);}
}
static void galOpenFolder(int only){
 int *indices=HeapAlloc(GetProcessHeap(),0,max(1,galFileCount)*sizeof(int));if(!indices)return;int n=galTargets(indices,only);
 if(n){PIDLIST_ABSOLUTE item=NULL;if(SUCCEEDED(SHParseDisplayName(galFiles[indices[0]].path,NULL,&item,0,NULL))){SHOpenFolderAndSelectItems(item,0,NULL,0);CoTaskMemFree(item);}}HeapFree(GetProcessHeap(),0,indices);
}
static void galActions(int only,POINT at){
 if(galJob)return;HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,1,L"Удалить файл(ы)…");AppendMenuW(m,MF_STRING,2,L"Переименовать…");AppendMenuW(m,MF_STRING,3,L"Показать в папке");AppendMenuW(m,MF_STRING,4,L"Убрать внешние видео из галереи");
 int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,at.x,at.y,0,gal,NULL);DestroyMenu(m);
 if(id==1)galDeleteFiles(only,0);if(id==2)galRenameFile(only);if(id==3)galOpenFolder(only);if(id==4)galDeleteFiles(only,1);
}
static void galImportVideo(void){
 if(galJob)return;wchar_t *buffer=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,65536*sizeof(wchar_t));if(!buffer)return;
 OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=gal;dialog.lpstrTitle=L"Добавить видео из другой папки";dialog.lpstrFilter=L"Видео\0*.mp4;*.mov;*.mkv;*.avi;*.webm;*.m4v;*.ts\0Все файлы\0*.*\0";dialog.lpstrFile=buffer;dialog.nMaxFile=65536;dialog.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_NOCHANGEDIR;
 if(GetOpenFileNameW(&dialog)){
  wchar_t selected[4096]=L"";wchar_t *name=buffer+wcslen(buffer)+1;
  if(!*name){wcsncpy(selected,buffer,4095);if(galImported(selected)<0&&galImportCount<GAL_MAX_IMPORTS)wcscpy(galImports[galImportCount++],selected);}
  else while(*name){wchar_t path[4096];swprintf(path,4096,L"%ls\\%ls",buffer,name);if(!*selected)wcscpy(selected,path);if(galImported(path)<0&&galImportCount<GAL_MAX_IMPORTS)wcscpy(galImports[galImportCount++],path);name+=wcslen(name)+1;}
  galImportsFile(1);galRefresh(selected);
 }HeapFree(GetProcessHeap(),0,buffer);
}
static void galSetView(int view){galView=max(0,min(view,2));ListView_SetView(galList,galView==0?LV_VIEW_LIST:galView==1?LV_VIEW_DETAILS:LV_VIEW_ICON);if(galView==2)ListView_SetIconSpacing(galList,D(146),D(122));SendMessageW(galViewBox,CB_SETCURSEL,galView,0);}
static void galRefresh(const wchar_t *selected){
 galBrowserStop();galBuilding=1;ListView_DeleteAllItems(galList);if(galFiles)HeapFree(GetProcessHeap(),0,galFiles);galFiles=NULL;galFileCount=0;
 wchar_t mask[4200];swprintf(mask,4200,L"%ls\\*.mp4",galFolder);WIN32_FIND_DATAW data;HANDLE find=FindFirstFileW(mask,&data);
 if(find!=INVALID_HANDLE_VALUE){do{if(!(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){wchar_t path[4096];swprintf(path,4096,L"%ls\\%ls",galFolder,data.cFileName);galAddFile(path);}}while(FindNextFileW(find,&data));FindClose(find);}
 for(int i=0;i<galImportCount;i++)galAddFile(galImports[i]);
 if(galFileCount)qsort(galFiles,galFileCount,sizeof(*galFiles),galFileOrder);
 if(galImages)ImageList_Destroy(galImages);galImages=ImageList_Create(GAL_THUMB_W,GAL_THUMB_H,ILC_COLOR32,16,16);
 BYTE blank[GAL_THUMB_W*GAL_THUMB_H*4];for(size_t i=0;i<sizeof(blank);i+=4){blank[i]=GetBValue(UI_SURFACE);blank[i+1]=GetGValue(UI_SURFACE);blank[i+2]=GetRValue(UI_SURFACE);blank[i+3]=255;}HBITMAP icon=galImageBitmap(blank);ImageList_Add(galImages,icon,NULL);DeleteObject(icon);ListView_SetImageList(galList,galImages,LVSIL_NORMAL);
 int choice=0;
 for(int i=0;i<galFileCount;i++){
  LVITEMW item={0};item.mask=LVIF_TEXT|LVIF_IMAGE;item.iItem=i;item.pszText=galFiles[i].name;item.iImage=0;SendMessageW(galList,LVM_INSERTITEMW,0,(LPARAM)&item);
  wchar_t size[80],date[80];swprintf(size,80,L"%.1f МБ",galFiles[i].size/1048576.);FILETIME utc={(DWORD)galFiles[i].date,(DWORD)(galFiles[i].date>>32)},local;SYSTEMTIME t;FileTimeToLocalFileTime(&utc,&local);FileTimeToSystemTime(&local,&t);swprintf(date,80,L"%02d.%02d.%04d",t.wDay,t.wMonth,t.wYear);
  LVITEMW sub={0};sub.iSubItem=1;sub.pszText=date;SendMessageW(galList,LVM_SETITEMTEXTW,i,(LPARAM)&sub);sub.iSubItem=2;sub.pszText=size;SendMessageW(galList,LVM_SETITEMTEXTW,i,(LPARAM)&sub);
  if(selected&&!_wcsicmp(selected,galFiles[i].path))choice=i;
 }
 galSetView(galView);
 if(galFileCount){ListView_SetItemState(galList,choice,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);ListView_EnsureVisible(galList,choice,FALSE);galLoad(galFiles[choice].path);galImageInit(&galBrowserJob);galBrowserJob.stop=CreateEventW(NULL,TRUE,FALSE,NULL);galBrowserJob.thread=CreateThread(NULL,0,galBrowserWorker,NULL,0,NULL);}
 else{galImageStop(&galFilmJob);galUnload();galInput[0]=0;galDuration=0;galRanges[0]=(EditRange){0,0};galUpdateClock(0);SetWindowTextW(galStatus,L"Нет видео. Нажмите «Добавить видео» или запишите экран.");EnableWindow(galSave,FALSE);}
 galBuilding=0;galBrowserSelection();
}
static void galLoad(const wchar_t *path){
 if(galJob)return;galImageStop(&galFilmJob);galUnload();wcsncpy(galInput,path,4095);galDuration=galProbe();galPosition=0;galRangeCount=1;galActiveRange=0;galRanges[0]=(EditRange){0,galDuration};galSyncRangeFields();galSetMode(0);galUpdateClock(0);
 typedef HRESULT (WINAPI *Factory)(LPCWSTR,BOOL,MFP_CREATION_OPTIONS,IMFPMediaPlayerCallback*,HWND,IMFPMediaPlayer**);static HMODULE library;static Factory factory;if(!library){library=LoadLibraryW(L"mfplay.dll");if(library)factory=(Factory)(void*)GetProcAddress(library,"MFPCreateMediaPlayer");}
 HRESULT hr=factory?factory(NULL,FALSE,0,&galCallback,galVideo,&galPlayer):E_NOTIMPL;if(SUCCEEDED(hr))hr=IMFPMediaPlayer_CreateMediaItemFromURL(galPlayer,galInput,FALSE,0,NULL);
 if(FAILED(hr)){galUnload();galPreviewStart(0,0);SetWindowTextW(galStatus,L"Предпросмотр без звука. Полный звук — «Открыть снаружи».");}
 if(galDuration>0)galFilmStart();else SetWindowTextW(galStatus,L"Не удалось прочитать видео. Проверьте файл или выберите другой.");
 EnableWindow(galSave,galDuration>0);InvalidateRect(galRuler,NULL,TRUE);InvalidateRect(galVideo,NULL,TRUE);
}
static void galEditEnabled(int enabled){
 EnableWindow(galList,enabled);HWND items[]={galFilm,galFromEdit,galToEdit,galAddRange,galDropRange,galRangesList,galKeep,galRemove,galViewBox,galSelectButton,galDeleteButton,galActionsButton,galPauseButton,galPauseSeconds,galPauseMode};
 for(unsigned i=0;i<sizeof(items)/sizeof(*items);i++)EnableWindow(items[i],enabled);EnableWindow(GetDlgItem(gal,143),enabled);
}
static void galCancelExport(void){
 if(!galJob)return;if(MessageBoxW(gal,galAnalyzing?L"Отменить поиск пауз? Выделение и исходное видео останутся без изменений.":L"Отменить сохранение? Исходное видео останется без изменений.",galAnalyzing?L"Поиск пауз":L"Сохранение",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
 TerminateProcess(galJob,2);WaitForSingleObject(galJob,3000);CloseHandle(galJob);galJob=NULL;galAnalyzing=0;DeleteFileW(galTemp);DeleteFileW(galFilterPath);DeleteFileW(galProgressPath);galEditEnabled(1);SetWindowTextW(galSave,L"Сохранить копию");SetWindowTextW(galStatus,L"Сохранение отменено. Исходник не изменён.");
}
static void galExportPlan(const EditRange *keep,int count,int single){
 if(galJob||!galInput[0])return;galSingleExport=single;if(count<1){MessageBoxW(gal,L"После обрезки не остаётся видео. Измените выделение.",L"Границы обрезки",MB_OK|MB_ICONINFORMATION);return;}
 ULONGLONG id=GetTickCount64();swprintf(galOutput,4200,L"%ls\\%ls_%llu.mp4",galFolder,single?L"Фрагмент":L"Обрезка",id);swprintf(galTemp,4200,L"%ls\\trim_%llu.mp4",appDir,id);swprintf(galLog,4200,L"%ls\\trim_%llu.log",appDir,id);swprintf(galFilterPath,4200,L"%ls\\trim_%llu.filter",appDir,id);swprintf(galProgressPath,4200,L"%ls\\trim_%llu.progress",appDir,id);
 FILE *filter=_wfopen(galFilterPath,L"wb");if(!filter){MessageBoxW(gal,L"Нет доступа к папке временных файлов.",L"Сохранение",MB_OK);return;}int valid=edit_write_graph(filter,keep,count,galHasAudio);if(fclose(filter)!=0)valid=0;if(!valid)return;
 galExportLength=0;for(int i=0;i<count;i++)galExportLength+=keep[i].end-keep[i].start;
 wchar_t cmd[24000];swprintf(cmd,24000,L"\"%ls\" -hide_banner -nostdin -n -progress \"%ls\" -i \"%ls\" -filter_complex_script \"%ls\" -map \"[vout]\" %ls -c:v libx264 -preset ultrafast -crf 23 -threads 2 -pix_fmt yuv420p %ls -movflags +faststart \"%ls\"",ffmpeg,galProgressPath,galInput,galFilterPath,galHasAudio?L"-map \"[aout]\"":L"",galHasAudio?L"-c:a aac -b:a 160k":L"-an",galTemp);
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE log=CreateFileW(galLog,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL),nul=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
 if(log==INVALID_HANDLE_VALUE||nul==INVALID_HANDLE_VALUE){if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(nul!=INVALID_HANDLE_VALUE)CloseHandle(nul);return;}
 STARTUPINFOW si={0};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=nul;si.hStdOutput=si.hStdError=log;PROCESS_INFORMATION process={0};int ok=CreateProcessW(ffmpeg,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,appDir,&si,&process);CloseHandle(log);CloseHandle(nul);
 if(!ok){MessageBoxW(gal,L"Не удалось запустить сохранение.",L"Обрезка",MB_OK|MB_ICONERROR);return;}galJob=process.hProcess;appOwnProcess(galJob);CloseHandle(process.hThread);galEditEnabled(0);EnableWindow(galSave,TRUE);SetWindowTextW(galSave,L"Отменить сохранение");SetWindowTextW(galStatus,L"Сохраняем копию… Исходный файл не изменяется.");
}
static int galReadRangeEdits(void){
 wchar_t a[80],b[80],*ea,*eb;GetWindowTextW(galFromEdit,a,80);GetWindowTextW(galToEdit,b,80);for(wchar_t *p=a;*p;p++)if(*p==L',')*p=L'.';for(wchar_t *p=b;*p;p++)if(*p==L',')*p=L'.';
 double start=wcstod(a,&ea),end=wcstod(b,&eb);if(ea==a||eb==b||*ea||*eb||!isfinite(start)||!isfinite(end)||start<0||end<=start||end>galDuration+.000001)return 0;
 galRanges[galActiveRange]=(EditRange){start,min(end,galDuration)};return 1;
}
static void galExport(void){
 if(galJob){galCancelExport();return;}
 if(!galReadRangeEdits()){MessageBoxW(gal,L"Укажите границы в секундах: 0 ≤ начало < конец ≤ длительность.",L"Границы обрезки",MB_OK|MB_ICONINFORMATION);return;}
 EditRange keep[EDIT_MAX_RANGES+1];int n=edit_plan(galRanges,galRangeCount,galDuration,galRemoveMode,keep);galExportPlan(keep,n,0);
}
static void galSavePiece(POINT screen){
 if(galJob||galDuration<=0)return;POINT client=screen;ScreenToClient(galFilm,&client);RECT bounds;GetClientRect(galFilm,&bounds);
 double at=galDuration*(client.x-D(15))/max(1,bounds.right-D(30));EditRange keep[EDIT_MAX_RANGES+1];int count=edit_plan(galRanges,galRangeCount,galDuration,galRemoveMode,keep),chosen=-1;
 for(int i=0;i<count;i++)if(at>=keep[i].start&&at<=keep[i].end){chosen=i;break;}
 HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING|(chosen<0?MF_GRAYED:0),1,chosen<0?L"Этот участок помечен на удаление":L"Сохранить этот фрагмент отдельно");
 int selected=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,screen.x,screen.y,0,gal,NULL);DestroyMenu(m);if(selected==1&&chosen>=0)galExportPlan(&keep[chosen],1,1);
}
static HWND galImportButton;
static void galLayout(void){
 RECT r;GetClientRect(gal,&r);int pad=D(16),left=D(340),x=left+pad,width=max(D(380),r.right-x-pad),videoHeight=max(D(140),r.bottom-D(606));
 MoveWindow(galImportButton,pad,pad,left-pad*2,D(36),TRUE);MoveWindow(galViewBox,pad,D(60),left-pad*2,D(180),TRUE);MoveWindow(galSelectButton,pad,D(103),D(145),D(34),TRUE);MoveWindow(galDeleteButton,pad+D(154),D(103),left-pad*2-D(154),D(34),TRUE);MoveWindow(galActionsButton,pad,D(143),left-pad*2,D(34),TRUE);MoveWindow(galList,pad,D(186),left-pad*2,max(D(100),r.bottom-D(202)),TRUE);
 MoveWindow(galVideo,x,pad,width,videoHeight,TRUE);int y=pad+videoHeight+D(8);
 MoveWindow(galSeek,x,y,width,D(26),TRUE);y+=D(26);MoveWindow(galRuler,x,y,width,D(22),TRUE);y+=D(22);MoveWindow(galClock,x,y,width,D(30),TRUE);y+=D(36);
 MoveWindow(galPlay,x,y,D(270),D(38),TRUE);MoveWindow(galBack,x+D(280),y,width-D(280),D(38),TRUE);y+=D(46);
 MoveWindow(galKeep,x,y,(width-D(8))/2,D(36),TRUE);MoveWindow(galRemove,x+(width+D(8))/2,y,(width-D(8))/2,D(36),TRUE);y+=D(42);
 MoveWindow(galFilm,x,y,width,D(108),TRUE);y+=D(114);
 MoveWindow(galFromLabel,x,y,D(85),D(30),TRUE);MoveWindow(galFromEdit,x+D(87),y,D(125),D(30),TRUE);MoveWindow(galToLabel,x+D(232),y,D(80),D(30),TRUE);MoveWindow(galToEdit,x+D(314),y,D(125),D(30),TRUE);y+=D(38);
 MoveWindow(galRangesList,x,y,width-D(230),D(54),TRUE);MoveWindow(galAddRange,x+width-D(220),y,D(220),D(25),TRUE);MoveWindow(galDropRange,x+width-D(220),y+D(29),D(220),D(25),TRUE);y+=D(62);
 MoveWindow(galPauseButton,x,y,D(215),D(34),TRUE);MoveWindow(GetDlgItem(gal,147),x+D(225),y,D(200),D(34),TRUE);MoveWindow(galPauseSeconds,x+D(430),y,D(90),D(34),TRUE);y+=D(40);MoveWindow(galPauseMode,x,y,width,D(160),TRUE);y+=D(36);
 MoveWindow(galSave,x,y,width,D(42),TRUE);y+=D(48);MoveWindow(galStatus,x,y,width,max(D(40),r.bottom-y-pad),TRUE);if(galPlayer)IMFPMediaPlayer_UpdateVideo(galPlayer);
}
static LRESULT CALLBACK galVideoProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_PAINT){PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);RECT r;GetClientRect(w,&r);
  if(galPlayer)IMFPMediaPlayer_UpdateVideo(galPlayer);
  else if(galFrameReady){FillRect(dc,&r,(HBRUSH)GetStockObject(BLACK_BRUSH));int width=min(r.right,r.bottom*640/360),height=width*360/640;BITMAPINFO b={0};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=640;b.bmiHeader.biHeight=-360;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;EnterCriticalSection(&galFrameLock);SetStretchBltMode(dc,HALFTONE);StretchDIBits(dc,(r.right-width)/2,(r.bottom-height)/2,width,height,0,0,640,360,galFrame,&b,DIB_RGB_COLORS,SRCCOPY);LeaveCriticalSection(&galFrameLock);}
  else{FillRect(dc,&r,uiSurfaceBrush);SetTextColor(dc,UI_MUTED);SetBkMode(dc,TRANSPARENT);SelectObject(dc,uiFont);DrawTextW(dc,galInput[0]?L"Загружаем кадр…":L"Выберите или добавьте видео",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
  EndPaint(w,&p);return 0;
 }return DefWindowProcW(w,msg,wp,lp);
}
static void galFindPauses(void){
 if(galJob||galDuration<=0)return;
 wchar_t value[80],*end;GetWindowTextW(galPauseSeconds,value,80);for(wchar_t *p=value;*p;p++)if(*p==L',')*p=L'.';double limit=wcstod(value,&end);
 if(end==value||*end||!isfinite(limit)||limit<.1||limit>600){MessageBoxW(gal,L"Укажите, сколько оставить от неподвижного участка: от 0,1 до 600 секунд.",L"Убрать паузы",MB_OK);return;}
 if(MessageBoxW(gal,L"Найти неподвижные участки во всём видео и заменить текущее выделение?\n\nЛишняя длительность будет отмечена на удаление. По умолчанию участки со звуком защищены. В режиме «Только по картинке» звук внутри вырезок удалится. Проверьте шкалу перед сохранением. Исходник останется.",L"Убрать паузы",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return;
 galProtectAudio=SendMessageW(galPauseMode,CB_GETCURSEL,0,0)!=1;galPauseLimit=limit;swprintf(galLog,4200,L"%ls\\pauses_%llu.log",appDir,GetTickCount64());
 wchar_t cmd[16000];swprintf(cmd,16000,L"\"%ls\" -hide_banner -nostdin -copyts -start_at_zero -i \"%ls\" -map 0:v:0 %ls -vf \"freezedetect=n=-60dB:d=%.6f\" %ls -f null -",ffmpeg,galInput,galProtectAudio&&galHasAudio?L"-map 0:a:0":L"-an",limit,galProtectAudio&&galHasAudio?L"-af silencedetect=noise=-50dB:d=0.1":L"");
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE log=CreateFileW(galLog,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL),nul=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
 if(log==INVALID_HANDLE_VALUE||nul==INVALID_HANDLE_VALUE){if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(nul!=INVALID_HANDLE_VALUE)CloseHandle(nul);SetWindowTextW(galStatus,L"Не удалось создать журнал анализа.");return;}
 STARTUPINFOW si={0};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=nul;si.hStdOutput=si.hStdError=log;PROCESS_INFORMATION pi={0};int ok=CreateProcessW(ffmpeg,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,appDir,&si,&pi);CloseHandle(log);CloseHandle(nul);
 if(!ok){SetWindowTextW(galStatus,L"Не удалось запустить анализ пауз.");return;}
 galJob=pi.hProcess;appOwnProcess(galJob);CloseHandle(pi.hThread);galAnalyzing=1;galEditEnabled(0);SetWindowTextW(galSave,L"Отменить анализ");SetWindowTextW(galStatus,L"Ищем неподвижные участки… Исходник не изменяется.");
}
static void galPausesDone(DWORD code){
 galAnalyzing=0;EditRange found[EDIT_MAX_RANGES];int count=0,overflow=0;double removed=0;
 FILE *f=_wfopen(galLog,L"rb");if(!f)code=1;
 if(f){count=pause_read(f,galDuration,galPauseLimit,found,&removed);fclose(f);if(count<0)overflow=1;}
 if(!code&&!overflow&&count&&galProtectAudio&&galHasAudio){EditRange safe[EDIT_MAX_RANGES];f=_wfopen(galLog,L"rb");if(!f)code=1;else{int n=pause_protect(f,found,count,galDuration,safe);fclose(f);if(n<0)overflow=1;else{count=n;memcpy(found,safe,n*sizeof(EditRange));removed=0;for(int i=0;i<n;i++)removed+=found[i].end-found[i].start;}}}
 if(code||overflow){SetWindowTextW(galStatus,overflow?L"Найдено больше 64 пауз. Выделение не изменено. Обработайте видео по частям.":L"Анализ не завершён. Выделение не изменено.");return;}
 DeleteFileW(galLog);if(!count){SetWindowTextW(galStatus,L"Нет подходящих пауз: картинка меняется либо звук защищён. Выделение не изменено.");return;}
 memcpy(galRanges,found,count*sizeof(EditRange));galRangeCount=count;galActiveRange=0;galSetMode(1);galSyncRangeFields();galPositionTo(found[0].start);
 wchar_t text[220];swprintf(text,220,L"Найдено пауз: %d. Удалится %.2f с. Защита звука учитывает выбранный режим. Проверьте выделение и сохраните копию.",count,removed);SetWindowTextW(galStatus,text);
}

static void galExportTick(void){
 if(!galJob)return;
 if(WaitForSingleObject(galJob,0)==WAIT_OBJECT_0){
  DWORD code;GetExitCodeProcess(galJob,&code);CloseHandle(galJob);galJob=NULL;galEditEnabled(1);SetWindowTextW(galSave,L"Сохранить копию");
  if(galAnalyzing){galPausesDone(code);return;}
  unsigned long long frames=0;FILE *progressFile=_wfopen(galProgressPath,L"rb");if(progressFile){char line[512];while(fgets(line,sizeof(line),progressFile))if(!strncmp(line,"frame=",6)){unsigned long long n=strtoull(line+6,NULL,10);if(n>frames)frames=n;}fclose(progressFile);}
  if(code==0&&frames>0&&CopyFileW(galTemp,galOutput,TRUE)){
   DeleteFileW(galTemp);DeleteFileW(galLog);DeleteFileW(galFilterPath);DeleteFileW(galProgressPath);
   if(galSingleExport){EditRange ranges[EDIT_MAX_RANGES];memcpy(ranges,galRanges,sizeof(ranges));int count=galRangeCount,active=galActiveRange,remove=galRemoveMode;double position=galPosition;wchar_t input[4096];wcscpy(input,galInput);galRefresh(input);memcpy(galRanges,ranges,sizeof(ranges));galRangeCount=count;galActiveRange=active;galSetMode(remove);galSyncRangeFields();galPositionTo(position);SetWindowTextW(galStatus,L"Фрагмент сохранён отдельно. Можно сохранить следующий кусочек.");}
   else{galRefresh(galOutput);SetWindowTextW(galStatus,L"Копия сохранена. Исходное видео не изменено.");}
  }
  else SetWindowTextW(galStatus,L"Не удалось сохранить копию. Журнал находится в папке диагностики.");return;
 }
 if(galAnalyzing)return;
 FILE *f=_wfopen(galProgressPath,L"rb");if(f){char data[4096];_fseeki64(f,0,SEEK_END);long long length=_ftelli64(f);_fseeki64(f,length>4095?length-4095:0,SEEK_SET);size_t n=fread(data,1,4095,f);data[n]=0;fclose(f);double seconds=0;for(char *p=data;(p=strstr(p,"out_time_us="))!=NULL;p+=12)seconds=strtoull(p+12,NULL,10)/1000000.;wchar_t text[160];swprintf(text,160,L"Сохраняем копию: %d%%. Исходник не изменяется.",galExportLength>0?min(99,(int)(seconds*100/galExportLength)):0);SetWindowTextW(galStatus,text);}
}
static LRESULT CALLBACK galProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_DRAWITEM:if(uiDrawItem((DRAWITEMSTRUCT*)lp))return TRUE;break;
 case WM_SIZE:if(galList)galLayout();return 0;
 case WM_GETMINMAXINFO:((MINMAXINFO*)lp)->ptMinTrackSize=(POINT){D(1060),D(740)};return 0;
 case WM_CTLCOLORLISTBOX:case WM_CTLCOLOREDIT:case WM_CTLCOLORSTATIC:SetTextColor((HDC)wp,UI_TEXT);SetBkColor((HDC)wp,UI_BG);return (LRESULT)uiBgBrush;
 case WM_APP+51:{if(lp&&(IMFPMediaPlayer*)lp!=galPlayer)return 0;PROPVARIANT d;PropVariantInit(&d);if(galPlayer&&SUCCEEDED(IMFPMediaPlayer_GetDuration(galPlayer,&galPosition100ns,&d))){double duration=d.vt==VT_UI8?d.uhVal.QuadPart/10000000.:d.vt==VT_I8?d.hVal.QuadPart/10000000.:galDuration;if(fabs(duration-galDuration)>.1){galDuration=duration;galRanges[0].end=duration;galSyncRangeFields();}galUpdateClock(galCurrentSeconds());}PropVariantClear(&d);return 0;}
 case WM_APP+52:if(lp&&(IMFPMediaPlayer*)lp!=galPlayer)return 0;galUnload();galPreviewStart(galPosition,0);SetWindowTextW(galStatus,L"Предпросмотр без звука. Полный звук — «Открыть снаружи».");return 0;
 case WM_APP+53:if(!galDragging)galUpdateClock(galCurrentSeconds());return 0;
 case WM_APP+54:InvalidateRect(galFilm,NULL,FALSE);return 0;
 case WM_APP+55:{GalBrowserImage *image=(GalBrowserImage*)lp;if(image->index>=0&&image->index<galFileCount){HBITMAP bitmap=galImageBitmap(image->pixels);if(bitmap){int i=ImageList_Add(galImages,bitmap,NULL);LVITEMW item={0};item.mask=LVIF_IMAGE;item.iItem=image->index;item.iImage=i;SendMessageW(galList,LVM_SETITEMW,0,(LPARAM)&item);DeleteObject(bitmap);}}HeapFree(GetProcessHeap(),0,image);return 0;}
 case WM_CONTEXTMENU:{POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if((HWND)wp==galFilm){if(p.x==-1){RECT r;GetWindowRect(galFilm,&r);p=(POINT){r.left+D(20),r.top+D(20)};}galSavePiece(p);return 0;}if((HWND)wp==galList){POINT client=p;ScreenToClient(galList,&client);LVHITTESTINFO hit={0};hit.pt=client;int i=ListView_HitTest(galList,&hit);galActions(i,p);return 0;}break;}
 case WM_NOTIFY:{NMHDR *h=(NMHDR*)lp;if(h->hwndFrom==galList&&h->code==LVN_ITEMCHANGED&&!galBuilding){NMLISTVIEW *n=(NMLISTVIEW*)lp;galBrowserSelection();if(!galJob&&(n->uNewState&LVIS_SELECTED)&&!(n->uOldState&LVIS_SELECTED)&&n->iItem>=0&&n->iItem<galFileCount)galLoad(galFiles[n->iItem].path);}return 0;}
 case WM_HSCROLL:{if((HWND)lp==galSeek&&galDuration>0){double at=galDuration*SendMessageW(galSeek,TBM_GETPOS,0,0)/10000.;galDragging=LOWORD(wp)==TB_THUMBTRACK;galUpdateClock(at);if(!galDragging)galPositionTo(at);}return 0;}
 case WM_COMMAND:{
  int id=LOWORD(wp),code=HIWORD(wp);
  if((id==114||id==115)&&code==EN_CHANGE&&!galEditing&&galDuration>0){if(galReadRangeEdits()){galFillRanges();InvalidateRect(galFilm,NULL,FALSE);}return 0;}
  if(id==102&&galInput[0]){if(galPlayer){MFP_MEDIAPLAYER_STATE state;IMFPMediaPlayer_GetState(galPlayer,&state);if(state==MFP_MEDIAPLAYER_STATE_PLAYING)IMFPMediaPlayer_Pause(galPlayer);else IMFPMediaPlayer_Play(galPlayer);}else if(!galPreviewThread||WaitForSingleObject(galPreviewThread,0)==WAIT_OBJECT_0)galPreviewStart(0,1);else InterlockedExchange(&galPreviewPaused,!galPreviewPaused);galUpdateClock(galCurrentSeconds());}
  if(id==103&&galInput[0])ShellExecuteW(w,L"open",galInput,NULL,NULL,SW_SHOWNORMAL);
  if(id==104)galExport();
  if(id==145)galFindPauses();
  if(id==130&&!galJob)galSetMode(0);if(id==131&&!galJob)galSetMode(1);
  if(id==132&&code==LBN_SELCHANGE&&!galJob){int i=(int)SendMessageW(galRangesList,LB_GETCURSEL,0,0);if(i>=0&&i<galRangeCount){galActiveRange=i;galSyncRangeFields();galPositionTo(galRanges[i].start);}}
  if(id==133&&!galJob&&galDuration>0){if(!galReadRangeEdits()){MessageBoxW(w,L"Сначала исправьте границы текущего фрагмента.",L"Фрагмент",MB_OK);return 0;}if(galRangeCount<EDIT_MAX_RANGES){double at=min(galPosition,max(0,galDuration-.1));galActiveRange=galRangeCount;galRanges[galRangeCount++]=(EditRange){at,min(galDuration,at+max(.1,min(5.,galDuration/4)))};galSyncRangeFields();}else MessageBoxW(w,L"Можно выделить до 64 фрагментов.",L"Фрагменты",MB_OK);}
  if(id==134&&!galJob){if(galRangeCount>1){for(int i=galActiveRange;i<galRangeCount-1;i++)galRanges[i]=galRanges[i+1];galRangeCount--;galActiveRange=min(galActiveRange,galRangeCount-1);}else{galRanges[0]=(EditRange){0,galDuration};galSetMode(0);}galSyncRangeFields();}
  if(id==140&&code==CBN_SELCHANGE)galSetView((int)SendMessageW(galViewBox,CB_GETCURSEL,0,0));
  if(id==141&&!galJob){galSelecting=!galSelecting;ListView_SetExtendedListViewStyle(galList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|(galSelecting?LVS_EX_CHECKBOXES:0));SetWindowTextW(galSelectButton,galSelecting?L"Готово":L"Выбрать");galBrowserSelection();}
  if(id==142)galDeleteFiles(-1,0);if(id==143)galImportVideo();
  if(id==144){RECT r;GetWindowRect(galActionsButton,&r);galActions(-1,(POINT){r.left,r.bottom});}
  return 0;
 }
 case WM_TIMER:galImageWatch(&galFilmJob);galImageWatch(&galBrowserJob);galExportTick();if(!galDragging)galUpdateClock(galCurrentSeconds());return 0;
 case WM_CLOSE:if(galJob){galCancelExport();if(galJob)return 0;}DestroyWindow(w);return 0;
 case WM_DESTROY:if(galJob){TerminateProcess(galJob,2);WaitForSingleObject(galJob,3000);CloseHandle(galJob);galJob=NULL;}galBrowserStop();galImageStop(&galFilmJob);galUnload();if(galImages){ImageList_Destroy(galImages);galImages=NULL;}if(galFiles){HeapFree(GetProcessHeap(),0,galFiles);galFiles=NULL;}galFileCount=0;KillTimer(w,1);gal=NULL;galList=NULL;return 0;
 }return DefWindowProcW(w,msg,wp,lp);
}
static void galShow(const wchar_t *path){
 if(gal){ShowWindow(gal,SW_RESTORE);SetForegroundWindow(gal);if(path&&*path)galRefresh(path);return;}
 galImageInit(&galFilmJob);galImageInit(&galBrowserJob);galImportsFile(0);
 wchar_t videos[MAX_PATH]=L"";if(FAILED(SHGetFolderPathW(NULL,CSIDL_MYVIDEO,NULL,SHGFP_TYPE_CURRENT,videos))||!videos[0]){GetEnvironmentVariableW(L"USERPROFILE",videos,MAX_PATH-32);wcscat(videos,L"\\Videos");}swprintf(galFolder,4096,L"%ls\\ScreenRecorder",videos);CreateDirectoryW(galFolder,NULL);
 WNDCLASSW c={0};c.lpfnWndProc=galProc;c.hInstance=GetModuleHandleW(NULL);c.lpszClassName=L"RecorderGallery";c.hbrBackground=uiBgBrush;c.hCursor=LoadCursorW(NULL,IDC_ARROW);RegisterClassW(&c);c.lpfnWndProc=galVideoProc;c.lpszClassName=L"RecorderPlaybackSurface";RegisterClassW(&c);c.lpfnWndProc=galFilmProc;c.lpszClassName=L"RecorderFilmstrip";c.hCursor=LoadCursorW(NULL,IDC_SIZEWE);RegisterClassW(&c);c.lpfnWndProc=galRulerProc;c.lpszClassName=L"RecorderTimeRuler";RegisterClassW(&c);
 RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);gal=CreateWindowExW(0,L"RecorderGallery",L"Галерея · просмотр и обрезка",WS_OVERLAPPEDWINDOW,max(work.left,D(15)),max(work.top,D(15)),min(D(1280),work.right-work.left-D(30)),min(D(980),work.bottom-work.top-D(30)),window,NULL,c.hInstance,NULL);uiDarkTitleBar(gal);SetClassLongPtrW(gal,GCLP_HBRBACKGROUND,(LONG_PTR)uiBgBrush);
 galList=galControl(WC_LISTVIEWW,L"",101,LVS_REPORT|LVS_SHOWSELALWAYS|WS_TABSTOP);ListView_SetBkColor(galList,UI_BG);ListView_SetTextBkColor(galList,UI_BG);ListView_SetTextColor(galList,UI_TEXT);ListView_SetExtendedListViewStyle(galList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);LVCOLUMNW column={0};column.mask=LVCF_TEXT|LVCF_WIDTH;column.pszText=L"Название";column.cx=D(260);SendMessageW(galList,LVM_INSERTCOLUMNW,0,(LPARAM)&column);column.pszText=L"Дата";column.cx=D(120);SendMessageW(galList,LVM_INSERTCOLUMNW,1,(LPARAM)&column);column.pszText=L"Размер";column.cx=D(100);SendMessageW(galList,LVM_INSERTCOLUMNW,2,(LPARAM)&column);
 galImportButton=galControl(L"BUTTON",L"+ Добавить видео",143,WS_TABSTOP);galViewBox=galControl(L"COMBOBOX",L"",140,CBS_DROPDOWNLIST|WS_TABSTOP);for(int i=0;i<3;i++){const wchar_t *names[]={L"Список",L"Таблица",L"Миниатюры"};SendMessageW(galViewBox,CB_ADDSTRING,0,(LPARAM)names[i]);}
 galSelectButton=galControl(L"BUTTON",L"Выбрать",141,WS_TABSTOP);galDeleteButton=galControl(L"BUTTON",L"Удалить",142,WS_TABSTOP);galActionsButton=galControl(L"BUTTON",L"Действия",144,WS_TABSTOP);
 galVideo=galControl(L"RecorderPlaybackSurface",L"",110,0);galSeek=galControl(TRACKBAR_CLASSW,L"Позиция просмотра",111,WS_TABSTOP);SendMessageW(galSeek,TBM_SETRANGE,TRUE,MAKELPARAM(0,10000));
 galRuler=galControl(L"RecorderTimeRuler",L"",120,0);galClock=galControl(L"STATIC",L"00:00.000 / 00:00.000",121,SS_LEFT|SS_CENTERIMAGE);
 galPlay=galControl(L"BUTTON",L"Воспроизвести / Пауза",102,WS_TABSTOP);galBack=galControl(L"BUTTON",L"Открыть снаружи",103,WS_TABSTOP);galKeep=galControl(L"BUTTON",L"Оставить выделенное",130,WS_TABSTOP);galRemove=galControl(L"BUTTON",L"Удалить выделенное",131,WS_TABSTOP);
 galFilm=galControl(L"RecorderFilmstrip",L"Границы фрагмента",122,WS_TABSTOP);galFromLabel=galControl(L"STATIC",L"Начало, с",116,0);galToLabel=galControl(L"STATIC",L"Конец, с",117,0);galFromEdit=galControl(L"EDIT",L"0.000",114,WS_TABSTOP|ES_AUTOHSCROLL);galToEdit=galControl(L"EDIT",L"0.000",115,WS_TABSTOP|ES_AUTOHSCROLL);
 galRangesList=galControl(L"LISTBOX",L"",132,LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP);galAddRange=galControl(L"BUTTON",L"+ Фрагмент",133,WS_TABSTOP);galDropRange=galControl(L"BUTTON",L"Убрать фрагмент",134,WS_TABSTOP);
 galPauseButton=galControl(L"BUTTON",L"Убрать паузы…",145,WS_TABSTOP);galControl(L"STATIC",L"Оставлять не более, с",147,SS_CENTERIMAGE);galPauseSeconds=galControl(L"EDIT",L"1",146,WS_TABSTOP|ES_AUTOHSCROLL);
 galPauseMode=galControl(L"COMBOBOX",L"",148,CBS_DROPDOWNLIST|WS_TABSTOP);SendMessageW(galPauseMode,CB_ADDSTRING,0,(LPARAM)L"Сохранять речь и звук (по умолчанию)");SendMessageW(galPauseMode,CB_ADDSTRING,0,(LPARAM)L"Только по картинке — звук вырезок удалится");SendMessageW(galPauseMode,CB_SETCURSEL,0,0);
 galSave=galControl(L"BUTTON",L"Сохранить копию",104,WS_TABSTOP);galStatus=galControl(L"STATIC",L"",0,0);
 galSelecting=0;galView=2;galLayout();SetTimer(gal,1,200,NULL);ShowWindow(gal,SW_SHOW);galRefresh(path);
}
#endif
