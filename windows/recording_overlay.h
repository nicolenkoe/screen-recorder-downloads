#ifndef RECORDER_RECORDING_OVERLAY_H
#define RECORDER_RECORDING_OVERLAY_H
static HWND overlayEdges[256],overlayPanel,overlayNotice;static int overlayEdgeCount,overlayPhase,overlayExcluded,overlaySource;
static RECT overlayTarget;static ULONGLONG overlayNoticeUntil;static wchar_t overlayTitle[160],overlayDetail[200];
static HFONT overlayFont,overlayLargeFont,overlaySmallFont;
static void overlayHideEdges(void){for(int i=0;i<overlayEdgeCount;i++)ShowWindow(overlayEdges[i],SW_HIDE);}
static void uiOverlayDestroy(void){for(int i=0;i<overlayEdgeCount;i++)if(overlayEdges[i])DestroyWindow(overlayEdges[i]);overlayEdgeCount=0;if(overlayPanel)DestroyWindow(overlayPanel);if(overlayNotice)DestroyWindow(overlayNotice);overlayPanel=overlayNotice=NULL;overlayPhase=0;if(overlayFont)DeleteObject(overlayFont);if(overlayLargeFont)DeleteObject(overlayLargeFont);if(overlaySmallFont)DeleteObject(overlaySmallFont);overlayFont=overlayLargeFont=overlaySmallFont=NULL;}
static void overlayNoticeShow(const wchar_t *title,const wchar_t *detail,int milliseconds){
 wcsncpy(overlayTitle,title,159);wcsncpy(overlayDetail,detail,199);if(overlayNotice)SetWindowTextW(overlayNotice,title);overlayNoticeUntil=GetTickCount64()+milliseconds;
 if(overlayNotice){ShowWindow(overlayNotice,SW_SHOWNOACTIVATE);SetWindowPos(overlayNotice,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);InvalidateRect(overlayNotice,NULL,FALSE);}
}
static LRESULT CALLBACK overlayProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 int kind=(int)GetWindowLongPtrW(w,GWLP_USERDATA);
 if(msg==WM_NCCREATE){CREATESTRUCTW *c=(CREATESTRUCTW*)lp;SetWindowLongPtrW(w,GWLP_USERDATA,(LONG_PTR)c->lpCreateParams);return DefWindowProcW(w,msg,wp,lp);}
 if(msg==WM_NCHITTEST&&kind==0)return HTTRANSPARENT;
 if(msg==WM_LBUTTONUP){if(kind==2&&overlayPhase==1)PostMessageW(window,WM_APP+80,0,0);
  if(kind==1){int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);if(y>D(48)&&(overlayPhase==2||overlayPhase==3)){if(x<D(285))PostMessageW(window,WM_COMMAND,4,0);else if(x<D(520))PostMessageW(window,WM_COMMAND,5,0);}else if(x>D(505))PostMessageW(window,WM_COMMAND,2,0);}return 0;}
 if(msg==WM_LBUTTONDOWN&&kind==1&&GET_Y_LPARAM(lp)<D(45)&&GET_X_LPARAM(lp)<D(500)){ReleaseCapture();SendMessageW(w,WM_NCLBUTTONDOWN,HTCAPTION,0);return 0;}
 if(msg==WM_ERASEBKGND)return 1;
 if(msg==WM_PAINT){
  PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);RECT r;GetClientRect(w,&r);SetBkMode(dc,TRANSPARENT);
  if(kind==0){HBRUSH b=CreateSolidBrush((GetTickCount64()/650)%2?RGB(255,44,67):RGB(216,30,52));FillRect(dc,&r,b);DeleteObject(b);}
  else{
   HBRUSH clear=CreateSolidBrush(RGB(255,0,255));FillRect(dc,&r,clear);DeleteObject(clear);HBRUSH bg=CreateSolidBrush(UI_BG);HPEN border=CreatePen(PS_SOLID,D(2),overlayPhase==3?RGB(255,54,76):UI_MINT);HGDIOBJ ob=SelectObject(dc,bg),op=SelectObject(dc,border);RoundRect(dc,1,1,r.right-1,r.bottom-1,D(18),D(18));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(bg);DeleteObject(border);
   SetTextColor(dc,UI_TEXT);
   if(kind==2){
    SelectObject(dc,wcslen(overlayTitle)==1?overlayLargeFont:overlayFont);RECT text={D(15),D(10),r.right-D(15),D(105)};DrawTextW(dc,overlayTitle,-1,&text,DT_CENTER|DT_VCENTER|DT_WORDBREAK);
    SelectObject(dc,overlaySmallFont);text=(RECT){D(12),D(112),r.right-D(12),r.bottom-D(8)};DrawTextW(dc,overlayDetail,-1,&text,DT_CENTER|DT_VCENTER|DT_WORDBREAK);
   }else{
    HBRUSH b=CreateSolidBrush(overlayPhase==3?RGB(255,44,67):RGB(255,194,75));ob=SelectObject(dc,b);Ellipse(dc,D(16),D(18),D(34),D(36));SelectObject(dc,ob);DeleteObject(b);
    wchar_t time[40],line[180];GetWindowTextW(timerLabel,time,40);swprintf(line,180,L"%ls  %ls",overlayPhase==3?L"ИДЁТ ЗАПИСЬ":overlayPhase==2?L"ЗАПУСК ЗАПИСИ":L"СОХРАНЯЕМ ВИДЕО",overlayPhase==3?time:L"");SelectObject(dc,overlayFont);RECT text={D(48),D(5),D(505),D(45)};DrawTextW(dc,line,-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    text=(RECT){D(520),D(6),r.right-D(10),D(45)};SetTextColor(dc,RGB(255,112,125));DrawTextW(dc,overlayPhase==4?L"Отмена":L"■ Стоп",-1,&text,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(dc,overlaySmallFont);SetTextColor(dc,SendMessageW(micBox,BM_GETCHECK,0,0)==BST_CHECKED?UI_MINT:UI_MUTED);text=(RECT){D(18),D(48),D(280),D(78)};DrawTextW(dc,SendMessageW(micBox,BM_GETCHECK,0,0)==BST_CHECKED?L"Микрофон: ВКЛ":L"Микрофон: ВЫКЛ",-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SetTextColor(dc,SendMessageW(systemBox,BM_GETCHECK,0,0)==BST_CHECKED?UI_MINT:UI_MUTED);text=(RECT){D(290),D(48),D(515),D(78)};DrawTextW(dc,SendMessageW(systemBox,BM_GETCHECK,0,0)==BST_CHECKED?L"Звук ПК: ВКЛ":L"Звук ПК: ВЫКЛ",-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    if(!overlayExcluded){SetTextColor(dc,RGB(255,194,75));text=(RECT){D(520),D(48),r.right-D(8),D(78)};DrawTextW(dc,L"В кадре",-1,&text,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
   }
  }
  EndPaint(w,&p);return 0;
 }
 return DefWindowProcW(w,msg,wp,lp);
}
static HWND overlayCreate(int kind,int x,int y,int width,int height){
 DWORD ex=WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE;if(kind==0)ex|=WS_EX_TRANSPARENT;
 HWND w=CreateWindowExW(ex,L"RecorderStatusOverlay",kind==0?L"Рамка записи":kind==1?L"Панель записи":L"Состояние записи",WS_POPUP,x,y,width,height,NULL,NULL,GetModuleHandleW(NULL),(void*)(INT_PTR)kind);
 if(w){if(kind==0)SetLayeredWindowAttributes(w,0,240,LWA_ALPHA);else SetLayeredWindowAttributes(w,RGB(255,0,255),255,LWA_COLORKEY);DWORD affinity=0;if(!SetWindowDisplayAffinity(w,0x11)||!GetWindowDisplayAffinity(w,&affinity)||affinity!=0x11)overlayExcluded=0;}else overlayExcluded=0;return w;
}
static void overlayMakeEdges(RECT r){
 int thick=D(6);if(overlayEdgeCount>252)return;RECT a[4]={{r.left,r.top,r.right,r.top+thick},{r.left,r.bottom-thick,r.right,r.bottom},{r.left,r.top+thick,r.left+thick,r.bottom-thick},{r.right-thick,r.top+thick,r.right,r.bottom-thick}};
 for(int i=0;i<4;i++){RECT e=a[i];overlayEdges[overlayEdgeCount++]=overlayCreate(0,e.left,e.top,max(1,e.right-e.left),max(1,e.bottom-e.top));}
}
static BOOL CALLBACK overlayMonitor(HMONITOR monitor,HDC dc,LPRECT rect,LPARAM param){overlayMakeEdges(*rect);return TRUE;}
static int overlayPrepare(int source){
 uiOverlayDestroy();overlaySource=source;overlayTarget=captureRect;overlayExcluded=1;
 overlayFont=CreateFontW(-D(23),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 overlayLargeFont=CreateFontW(-D(78),0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 overlaySmallFont=CreateFontW(-D(17),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 WNDCLASSW c={0};c.lpfnWndProc=overlayProc;c.hInstance=GetModuleHandleW(NULL);c.lpszClassName=L"RecorderStatusOverlay";c.hCursor=LoadCursorW(NULL,IDC_ARROW);RegisterClassW(&c);
 if(source==2)EnumDisplayMonitors(NULL,NULL,overlayMonitor,0);
 else{if(source==3&&IsWindow(uiPickedWindow)){RECT r;GetClientRect(uiPickedWindow,&r);POINT p={0,0};ClientToScreen(uiPickedWindow,&p);overlayTarget=(RECT){p.x,p.y,p.x+r.right,p.y+r.bottom};}overlayMakeEdges(overlayTarget);}
 MONITORINFO monitor={sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);RECT r=monitor.rcWork;
 overlayPanel=overlayCreate(1,r.right-D(665),r.top+D(15),D(650),D(86));overlayNotice=overlayCreate(2,r.left+(r.right-r.left-D(540))/2,r.top+(r.bottom-r.top-D(170))/2,D(540),D(170));
 if(!overlayPanel||!overlayNotice||overlayEdgeCount<4){uiOverlayDestroy();return 0;}
 if(!overlayExcluded&&MessageBoxW(window,L"Windows не смогла исключить индикаторы из захвата.\n\nПоказать заметную рамку и панель? Они могут попасть в записанное видео.\n\n«ОК» — записывать с видимыми индикаторами. «Отмена» — не начинать запись.",L"Индикаторы записи",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK){uiOverlayDestroy();return 0;}
 overlayPhase=1;return 1;
}
static void overlayCount(int count){wchar_t text[12];swprintf(text,12,L"%d",count);overlayPhase=1;overlayNoticeShow(text,countdownEsc?L"До начала записи · Esc или щелчок — отменить":L"До начала записи · щелчок — отменить",1200);}
static void overlayStarting(void){overlayPhase=2;ShowWindow(overlayPanel,SW_SHOWNOACTIVATE);overlayNoticeShow(L"Запускаем запись…",L"Ждём первые кадры",10000);}
static void overlayStopping(void){overlayPhase=4;overlayHideEdges();if(overlayPanel)ShowWindow(overlayPanel,SW_SHOWNOACTIVATE);overlayNoticeShow(L"Останавливаем запись…",L"Видео ещё сохраняется",1200);}
static void overlayStopped(void){overlayPhase=4;overlayHideEdges();overlayNoticeShow(L"Запись остановлена",L"Сохраняем видео…",1600);}
static void overlayFinished(int success,int hadFrames){
 overlayHideEdges();if(overlayPanel)ShowWindow(overlayPanel,SW_HIDE);overlayPhase=5;
 overlayNoticeShow(success?L"Видео сохранено":hadFrames?L"Не удалось сохранить видео":L"Запись не началась",success?L"Запись остановлена · видео в галерее":L"Откройте окно программы и проверьте сообщение",2600);
}
static void uiIndicator(int recording){
 if(recording&&overlayPhase!=3){overlayPhase=3;for(int i=0;i<overlayEdgeCount;i++)if(overlayEdges[i])ShowWindow(overlayEdges[i],SW_SHOWNOACTIVATE);if(overlayPanel)ShowWindow(overlayPanel,SW_SHOWNOACTIVATE);overlayNoticeShow(L"Запись началась",L"Рамка и таймер останутся поверх окон",1600);}
 if(overlayPhase==3&&overlaySource==3&&IsWindow(uiPickedWindow)){
  RECT r;GetClientRect(uiPickedWindow,&r);POINT p={0,0};ClientToScreen(uiPickedWindow,&p);int t=D(6);RECT edges[4]={{p.x,p.y,p.x+r.right,p.y+t},{p.x,p.y+r.bottom-t,p.x+r.right,p.y+r.bottom},{p.x,p.y+t,p.x+t,p.y+r.bottom-t},{p.x+r.right-t,p.y+t,p.x+r.right,p.y+r.bottom-t}};
  for(int i=0;i<4&&i<overlayEdgeCount;i++)SetWindowPos(overlayEdges[i],HWND_TOPMOST,edges[i].left,edges[i].top,max(1,edges[i].right-edges[i].left),max(1,edges[i].bottom-edges[i].top),SWP_NOACTIVATE);
 }
 if(overlayPhase==3)for(int i=0;i<overlayEdgeCount;i++){SetWindowPos(overlayEdges[i],HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);InvalidateRect(overlayEdges[i],NULL,FALSE);}
 if(overlayPanel&&IsWindowVisible(overlayPanel)){SetWindowPos(overlayPanel,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);InvalidateRect(overlayPanel,NULL,FALSE);}
 if(overlayNotice&&IsWindowVisible(overlayNotice)&&GetTickCount64()>=overlayNoticeUntil)ShowWindow(overlayNotice,SW_HIDE);
}
#endif
