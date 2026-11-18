#ifndef RECORDER_UI_H
#define RECORDER_UI_H
#include <commctrl.h>
#include <windowsx.h>
#define UI_BG RGB(17,27,35)
#define UI_SURFACE RGB(30,46,57)
#define UI_MINT RGB(112,237,187)
#define UI_TEXT RGB(240,247,250)
#define UI_MUTED RGB(161,183,195)
#define UI_LINE RGB(60,80,93)
static HFONT uiFont,uiTitleFont,uiSmallFont,uiTimerFont;
static HBRUSH uiBgBrush,uiSurfaceBrush;
static int uiScale=100,uiDpi=96,uiScrollX=0,uiScrollY=0,uiReady=0,uiLayoutBusy=0,uiIdle=1,uiGain=100,uiSystemGain=100;
static HWND uiFontButton,uiGainSlider,uiGainLabel,uiSystemGainSlider,uiSystemGainLabel,uiMode[5],uiSubtitle;
static HWND uiDot;static int uiDotAttempted=0;
static WNDPROC uiOldButton;
typedef struct{HWND w;int x,y,width,height,small;}UiControl;
static UiControl uiItems[64];static int uiCount;
static int U(int value){return MulDiv(value,uiScale*uiDpi,9600);}
static int D(int value){return MulDiv(value,uiDpi,96);}
static void uiReadSettings(void){DWORD type=0,size=4,value=100;HKEY key;
 if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\ScreenRecorder",0,KEY_READ,&key)==ERROR_SUCCESS){if(RegQueryValueExW(key,L"FontPercent",NULL,&type,(BYTE*)&value,&size)==ERROR_SUCCESS&&type==REG_DWORD&&(value==100||value==125||value==150||value==175||value==200))uiScale=value;size=4;value=100;if(RegQueryValueExW(key,L"MicGain",NULL,&type,(BYTE*)&value,&size)==ERROR_SUCCESS&&value<=100)uiGain=value;size=4;value=100;if(RegQueryValueExW(key,L"SystemGain",NULL,&type,(BYTE*)&value,&size)==ERROR_SUCCESS&&type==REG_DWORD&&value<=100)uiSystemGain=value;RegCloseKey(key);}}
static void uiSaveSettings(void){HKEY key;if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\ScreenRecorder",0,NULL,0,KEY_WRITE,NULL,&key,NULL)==ERROR_SUCCESS){DWORD s=uiScale,g=uiGain,sys=uiSystemGain;RegSetValueExW(key,L"FontPercent",0,REG_DWORD,(BYTE*)&s,4);RegSetValueExW(key,L"MicGain",0,REG_DWORD,(BYTE*)&g,4);RegSetValueExW(key,L"SystemGain",0,REG_DWORD,(BYTE*)&sys,4);RegCloseKey(key);}}
static LRESULT CALLBACK uiButtonProc(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR ref){
 if(GetPropW(w,L"RecorderCheck")){if(msg==BM_GETCHECK)return GetPropW(w,L"RecorderChecked")?BST_CHECKED:BST_UNCHECKED;if(msg==BM_SETCHECK){if(wp==BST_CHECKED)SetPropW(w,L"RecorderChecked",(HANDLE)1);else RemovePropW(w,L"RecorderChecked");InvalidateRect(w,NULL,FALSE);return 0;}}
 return DefSubclassProc(w,msg,wp,lp);
}
static void uiSliderChange(HWND w,int value,int code){int low=(int)(INT_PTR)GetPropW(w,L"SliderMin"),high=(int)(INT_PTR)GetPropW(w,L"SliderMax");value=max(low,min(high,value));SetWindowLongPtrW(w,GWLP_USERDATA,value);InvalidateRect(w,NULL,FALSE);SendMessageW(GetParent(w),WM_HSCROLL,MAKEWPARAM(code,value),(LPARAM)w);}
static LRESULT CALLBACK uiSliderProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){if(msg==WM_CREATE){SetPropW(w,L"SliderMax",(HANDLE)100);return 0;}if(msg==TBM_SETRANGE){SetPropW(w,L"SliderMin",(HANDLE)(INT_PTR)(short)LOWORD(lp));SetPropW(w,L"SliderMax",(HANDLE)(INT_PTR)(short)HIWORD(lp));return 0;}if(msg==TBM_SETPOS){SetWindowLongPtrW(w,GWLP_USERDATA,lp);InvalidateRect(w,NULL,FALSE);return 0;}if(msg==TBM_GETPOS)return GetWindowLongPtrW(w,GWLP_USERDATA);
 if(msg==WM_LBUTTONDOWN||(msg==WM_MOUSEMOVE&&GetCapture()==w)){if(msg==WM_LBUTTONDOWN){SetFocus(w);SetCapture(w);}RECT r;GetClientRect(w,&r);int low=(int)(INT_PTR)GetPropW(w,L"SliderMin"),high=(int)(INT_PTR)GetPropW(w,L"SliderMax");int x=GET_X_LPARAM(lp);uiSliderChange(w,low+MulDiv(x-D(12),high-low,max(1,r.right-D(24))),TB_THUMBTRACK);return 0;}
 if(msg==WM_LBUTTONUP){ReleaseCapture();uiSliderChange(w,(int)GetWindowLongPtrW(w,GWLP_USERDATA),TB_ENDTRACK);return 0;}if(msg==WM_KEYDOWN){int value=(int)GetWindowLongPtrW(w,GWLP_USERDATA);if(wp==VK_LEFT||wp==VK_DOWN)value--;else if(wp==VK_RIGHT||wp==VK_UP)value++;else if(wp==VK_HOME)value=(int)(INT_PTR)GetPropW(w,L"SliderMin");else if(wp==VK_END)value=(int)(INT_PTR)GetPropW(w,L"SliderMax");else return DefWindowProcW(w,msg,wp,lp);uiSliderChange(w,value,TB_THUMBPOSITION);return 0;}
 if(msg==WM_GETDLGCODE)return DLGC_WANTARROWS;if(msg==WM_SETFOCUS||msg==WM_KILLFOCUS){InvalidateRect(w,NULL,FALSE);return 0;}if(msg==WM_ERASEBKGND)return 1;if(msg==WM_PAINT){PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);RECT r;GetClientRect(w,&r);FillRect(dc,&r,uiBgBrush);int low=(int)(INT_PTR)GetPropW(w,L"SliderMin"),high=(int)(INT_PTR)GetPropW(w,L"SliderMax"),pos=(int)GetWindowLongPtrW(w,GWLP_USERDATA);int x=D(12)+MulDiv(pos-low,max(1,r.right-D(24)),max(1,high-low)),y=r.bottom/2;HBRUSH b=CreateSolidBrush(UI_LINE);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,GetStockObject(NULL_PEN));RoundRect(dc,D(12),y-D(3),r.right-D(12),y+D(3),D(6),D(6));SelectObject(dc,ob);DeleteObject(b);b=CreateSolidBrush(IsWindowEnabled(w)?UI_MINT:UI_MUTED);ob=SelectObject(dc,b);RoundRect(dc,D(12),y-D(3),x,y+D(3),D(6),D(6));Ellipse(dc,x-D(9),y-D(9),x+D(9),y+D(9));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);if(GetFocus()==w){InflateRect(&r,-1,-1);DrawFocusRect(dc,&r);}EndPaint(w,&p);return 0;}return DefWindowProcW(w,msg,wp,lp);}
static HWND uiAdd(const wchar_t *text,int x,int y,int width,int height,int id,const wchar_t *cls,DWORD style){
 if(!wcscmp(cls,TRACKBAR_CLASSW)){WNDCLASSW wc={0};wc.lpfnWndProc=uiSliderProc;wc.hInstance=GetModuleHandleW(NULL);wc.lpszClassName=L"RecorderSlider";wc.hCursor=LoadCursorW(NULL,IDC_HAND);RegisterClassW(&wc);cls=L"RecorderSlider";style=WS_TABSTOP;}
 int button=!wcscmp(cls,L"BUTTON"),check=button&&((style&BS_TYPEMASK)==BS_AUTOCHECKBOX);
 if(button)style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
 if(!wcscmp(cls,L"COMBOBOX"))style|=CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
 HWND w=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,0,0,1,1,window,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
 if(check)SetPropW(w,L"RecorderCheck",(HANDLE)1);
 if(button)SetWindowSubclass(w,uiButtonProc,1,0);
 if(uiCount<64)uiItems[uiCount++]=(UiControl){w,x,y,width,height,id==7};return w;
}
static void uiFonts(void){HFONT old[]={uiFont,uiTitleFont,uiSmallFont,uiTimerFont};
 uiFont=CreateFontW(-U(20),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
 uiTitleFont=CreateFontW(-U(32),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 uiSmallFont=CreateFontW(-D(14),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 uiTimerFont=CreateFontW(-U(28),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 for(int i=0;i<uiCount;i++){SendMessageW(uiItems[i].w,WM_SETFONT,(WPARAM)(uiItems[i].small?uiSmallFont:uiFont),TRUE);wchar_t cls[32];GetClassNameW(uiItems[i].w,cls,32);if(!wcscmp(cls,L"COMBOBOX")){SendMessageW(uiItems[i].w,CB_SETITEMHEIGHT,(WPARAM)-1,U(34));SendMessageW(uiItems[i].w,CB_SETITEMHEIGHT,0,U(34));}}
 SendMessageW(timerLabel,WM_SETFONT,(WPARAM)uiTimerFont,TRUE);
 for(int i=0;i<4;i++)if(old[i])DeleteObject(old[i]);
}
static void uiLayout(int resize){if(!uiReady||uiLayoutBusy)return;uiLayoutBusy=1;
 if(resize){MONITORINFO monitor={sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);RECT bounds={0,0,U(1020),U(796)};AdjustWindowRectEx(&bounds,(DWORD)GetWindowLongPtrW(window,GWL_STYLE),FALSE,0);int w=min(bounds.right-bounds.left,monitor.rcWork.right-monitor.rcWork.left-D(24)),h=min(bounds.bottom-bounds.top,monitor.rcWork.bottom-monitor.rcWork.top-D(24));RECT old;GetWindowRect(window,&old);int x=max(monitor.rcWork.left,min(old.left,monitor.rcWork.right-w)),y=max(monitor.rcWork.top,min(old.top,monitor.rcWork.bottom-h));SetWindowPos(window,NULL,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);RECT available;GetClientRect(window,&available);int addW=max(0,U(1020)-available.right),addH=max(0,U(796)-available.bottom);if(addW||addH){w=min(w+addW,monitor.rcWork.right-monitor.rcWork.left-D(24));h=min(h+addH,monitor.rcWork.bottom-monitor.rcWork.top-D(24));SetWindowPos(window,NULL,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);}}
 RECT client;GetClientRect(window,&client);SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS};si.nMin=0;si.nMax=U(1020)-1;si.nPage=client.right;si.nPos=uiScrollX;SetScrollInfo(window,SB_HORZ,&si,TRUE);si.nMax=U(796)-1;si.nPage=client.bottom;si.nPos=uiScrollY;SetScrollInfo(window,SB_VERT,&si,TRUE);uiScrollX=GetScrollPos(window,SB_HORZ);uiScrollY=GetScrollPos(window,SB_VERT);GetClientRect(window,&client);
 for(int i=0;i<uiCount;i++){UiControl c=uiItems[i];int x=U(c.x)-uiScrollX,y=U(c.y)-uiScrollY,w=U(c.width),h=U(c.height);if(c.w==uiFontButton){x=client.right-D(112);y=D(14);w=D(96);h=D(32);}MoveWindow(c.w,x,y,w,h,TRUE);}
 InvalidateRect(window,NULL,TRUE);uiLayoutBusy=0;
}
static void uiText(HDC dc,const wchar_t *text,int x,int y,int w,int h,HFONT font,COLORREF color,UINT flags){RECT r={U(x)-uiScrollX,U(y)-uiScrollY,U(x+w)-uiScrollX,U(y+h)-uiScrollY};SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);HGDIOBJ old=SelectObject(dc,font);DrawTextW(dc,text,-1,&r,flags);SelectObject(dc,old);}
/* Native, animated explanations of each capture mode. Coordinates follow interface scaling. */
static int uiDiagramMode=-1;
static void uiShape(HDC dc,int x,int y,int w,int h,COLORREF fill,COLORREF edge,int thickness){
 HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,max(1,U(thickness)),edge);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);
 RoundRect(dc,U(x)-uiScrollX,U(y)-uiScrollY,U(x+w)-uiScrollX,U(y+h)-uiScrollY,U(8),U(8));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
}
static void uiDiagramText(HDC dc,const wchar_t *text,int x,int y,int w,int h,COLORREF color){
 uiText(dc,text,x,y,w,h,uiFont,color,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
static void uiMonitor(HDC dc,int x,int y,int w,int h,int selected,const wchar_t *label){
 uiShape(dc,x,y,w,h,UI_BG,selected?UI_MINT:UI_MUTED,selected?3:2);
 uiShape(dc,x+7,y+7,w-14,h-24,selected?RGB(30,69,57):UI_SURFACE,UI_LINE,1);
 uiShape(dc,x+w/2-5,y+h,10,12,UI_MUTED,UI_MUTED,1);
 uiShape(dc,x+w/2-37,y+h+12,74,5,UI_MUTED,UI_MUTED,1);
 if(label)uiDiagramText(dc,label,x+10,y+30,w-20,h-55,UI_TEXT);
}
static void uiExampleWindow(HDC dc,int x,int y,int w,int h,int chosen,const wchar_t *label){
 uiShape(dc,x,y,w,h,chosen?RGB(36,81,66):RGB(39,51,63),chosen?UI_MINT:UI_LINE,chosen?3:1);
 uiShape(dc,x+5,y+5,w-10,16,chosen?RGB(59,126,100):RGB(58,72,85),chosen?UI_MINT:UI_LINE,1);
 if(label)uiDiagramText(dc,label,x+7,y+25,w-14,h-30,chosen?UI_TEXT:UI_MUTED);
}
static void uiDemoCursor(HDC dc,int x,int y){
 POINT points[]={{U(x)-uiScrollX,U(y)-uiScrollY},{U(x)-uiScrollX,U(y+23)-uiScrollY},{U(x+6)-uiScrollX,U(y+17)-uiScrollY},{U(x+13)-uiScrollX,U(y+29)-uiScrollY},{U(x+18)-uiScrollX,U(y+26)-uiScrollY},{U(x+11)-uiScrollX,U(y+15)-uiScrollY},{U(x+22)-uiScrollX,U(y+15)-uiScrollY}};
 HBRUSH b=CreateSolidBrush(UI_TEXT);HPEN p=CreatePen(PS_SOLID,1,UI_BG);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);Polygon(dc,points,7);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
}
static void uiCaptureDiagram(HDC dc,int mode){
 const wchar_t *titles[]={L"Запишется выделенная часть",L"Запишется весь этот монитор",L"Общий рабочий стол двух мониторов",L"Запишется одно выбранное окно",L"Схема режима · пока недоступно"};
 uiDiagramText(dc,titles[mode],50,266,520,30,mode==4?RGB(243,185,107):UI_MUTED);
 if(mode==2){uiMonitor(dc,63,321,235,137,1,L"Монитор 1");uiMonitor(dc,319,321,235,137,1,L"Монитор 2");return;}
 uiMonitor(dc,104,307,410,151,mode==1,NULL);
 if(mode==0){
  uiExampleWindow(dc,123,326,139,73,0,NULL);uiExampleWindow(dc,270,338,219,87,0,NULL);
  double phase=(GetTickCount64()%5000)/5000.0,progress=phase<.16?0:phase<.72?(phase-.16)/.56:1;
  int w=60+(int)(190*progress),h=35+(int)(57*progress),x=167,y=336;
  uiShape(dc,x,y,w,h,RGB(39,95,73),UI_MINT,3);
  uiShape(dc,x+w-4,y+h-4,8,8,UI_TEXT,UI_MINT,1);uiDemoCursor(dc,x+w,y+h);
 }else if(mode==1){
  uiExampleWindow(dc,122,325,142,75,0,NULL);uiExampleWindow(dc,278,338,216,89,0,NULL);
  uiShape(dc,111,314,396,127,RGB(31,74,59),UI_MINT,3);uiDiagramText(dc,L"Весь экран",120,346,376,55,UI_TEXT);
 }else{
  uiExampleWindow(dc,120,321,168,84,mode==4,mode==4?L"Окно 1":L"Другое окно");
  uiExampleWindow(dc,290,348,205,86,1,mode==4?L"Окно 2":L"Выбранное окно");uiDemoCursor(dc,424,350);
 }
}
static void uiAnimateDiagram(void){if(uiIdle){RECT r={U(50)-uiScrollX,U(265)-uiScrollY,U(575)-uiScrollX,U(487)-uiScrollY};InvalidateRect(window,&r,FALSE);}}

static void uiPaint(void){PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);RECT r;GetClientRect(window,&r);FillRect(dc,&r,uiBgBrush);
 uiText(dc,L"Запись экрана 3.0.2",90,20,650,36,uiFont,UI_TEXT,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 int lx=U(28)-uiScrollX,ly=U(16)-uiScrollY;HPEN pen=CreatePen(PS_SOLID,U(4),UI_MINT);HGDIOBJ op=SelectObject(dc,pen),ob=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,lx+U(21),ly+U(4),lx+U(57),ly+U(36),U(6),U(6));MoveToEx(dc,lx+U(41),ly+U(36),NULL);LineTo(dc,lx+U(41),ly+U(44));MoveToEx(dc,lx+U(29),ly+U(44),NULL);LineTo(dc,lx+U(53),ly+U(44));SelectObject(dc,op);DeleteObject(pen);pen=CreatePen(PS_SOLID,U(4),UI_TEXT);SelectObject(dc,pen);RoundRect(dc,lx+U(4),ly+U(19),lx+U(29),ly+U(55),U(7),U(7));HBRUSH red=CreateSolidBrush(RGB(255,38,62));SelectObject(dc,red);Ellipse(dc,lx+U(11),ly+U(30),lx+U(23),ly+U(42));SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(pen);DeleteObject(red);
 uiText(dc,L"Запишите то, что важно",30,85,950,46,uiTitleFont,UI_TEXT,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 uiText(dc,L"Выберите режим захвата и источники звука.",30,137,950,34,uiFont,UI_MUTED,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 pen=CreatePen(PS_SOLID,1,UI_LINE);op=SelectObject(dc,pen);MoveToEx(dc,U(30)-uiScrollX,U(245)-uiScrollY,NULL);LineTo(dc,U(990)-uiScrollX,U(245)-uiScrollY);MoveToEx(dc,U(600)-uiScrollX,U(265)-uiScrollY,NULL);LineTo(dc,U(600)-uiScrollX,U(555)-uiScrollY);SelectObject(dc,op);DeleteObject(pen);
 int mode=uiDiagramMode==4?4:(int)SendMessageW(sourceBox,CB_GETCURSEL,0,0);if(mode<0||mode>4)mode=0;
 uiCaptureDiagram(dc,mode);
 const wchar_t *tips[]={L"Зажмите кнопку мыши → протяните рамку → отпустите.",L"В запись попадёт всё, что видно на этом мониторе.",L"Оба монитора будут записаны в одно видео.",L"После «Захватить экран» выберите окно из списка.",L"Два отдельных окна. Запись этого режима пока недоступна."};
 uiText(dc,tips[mode],50,490,520,66,uiFont,UI_MUTED,DT_CENTER|DT_WORDBREAK);
 uiText(dc,L"Качество",625,491,125,40,uiFont,UI_MUTED,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 uiText(dc,L"Уровень микрофона",625,353,350,28,uiSmallFont,UI_MUTED,DT_LEFT|DT_SINGLELINE);
 int meter=mic.loopback?0:(int)mic.level;RECT bar={U(625)-uiScrollX,U(380)-uiScrollY,U(978)-uiScrollX,U(387)-uiScrollY};FillRect(dc,&bar,uiSurfaceBrush);bar.right=bar.left+U(353)*max(0,min(meter,100))/100;HBRUSH mintBrush=CreateSolidBrush(UI_MINT);FillRect(dc,&bar,mintBrush);DeleteObject(mintBrush);
 uiText(dc,L"Ctrl + Shift + F12 — остановить запись",30,623,950,23,uiSmallFont,UI_MUTED,DT_LEFT|DT_SINGLELINE);
 EndPaint(window,&paint);
}
static int uiDrawItem(DRAWITEMSTRUCT *draw){HDC dc=draw->hDC;RECT r=draw->rcItem;wchar_t text[256]=L"";SetBkMode(dc,TRANSPARENT);SelectObject(dc,uiFont);COLORREF fill=UI_SURFACE,ink=UI_TEXT;int id=draw->CtlID;
 if(draw->CtlType==ODT_COMBOBOX){if(draw->itemID==(UINT)-1)GetWindowTextW(draw->hwndItem,text,256);else SendMessageW(draw->hwndItem,CB_GETLBTEXT,draw->itemID,(LPARAM)text);HBRUSH b=CreateSolidBrush(draw->itemState&ODS_SELECTED?UI_LINE:UI_SURFACE);FillRect(dc,&r,b);DeleteObject(b);SetTextColor(dc,UI_TEXT);r.left+=U(12);DrawTextW(dc,text,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE);return 1;}
 if(draw->CtlType!=ODT_BUTTON)return 0;
 FillRect(dc,&r,uiBgBrush);GetWindowTextW(draw->hwndItem,text,256);int source=(int)SendMessageW(sourceBox,CB_GETCURSEL,0,0);int modeMap[]={0,3,1,2};int active=uiDiagramMode==4?id==24:(id>=20&&id<=23&&source==modeMap[id-20]);
 if(id==1||id==104||active){fill=UI_MINT;ink=UI_BG;}if(draw->itemState&ODS_DISABLED){fill=RGB(28,40,49);ink=RGB(104,125,138);}if(draw->itemState&ODS_SELECTED){fill=id==1||active?RGB(74,191,149):UI_LINE;}
 HBRUSH b=CreateSolidBrush(id==7?UI_BG:fill);HPEN p=CreatePen(PS_SOLID,1,id==7?UI_BG:(active||id==1?fill:UI_LINE));HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,r.left+1,r.top+1,r.right-1,r.bottom-1,U(14),U(14));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);
 SetTextColor(dc,ink);if(id==7)SelectObject(dc,uiSmallFont);
 if(GetPropW(draw->hwndItem,L"RecorderCheck")){int on=SendMessageW(draw->hwndItem,BM_GETCHECK,0,0)==BST_CHECKED;RECT pill={r.right-U(61),r.top+(r.bottom-r.top-U(25))/2,r.right-U(9),r.top+(r.bottom-r.top+U(25))/2};b=CreateSolidBrush(on?UI_MINT:UI_LINE);ob=SelectObject(dc,b);op=SelectObject(dc,GetStockObject(NULL_PEN));RoundRect(dc,pill.left,pill.top,pill.right,pill.bottom,U(24),U(24));SelectObject(dc,ob);DeleteObject(b);b=CreateSolidBrush(on?UI_BG:UI_TEXT);ob=SelectObject(dc,b);int cx=on?pill.right-U(13):pill.left+U(13);Ellipse(dc,cx-U(9),pill.top+U(4),cx+U(9),pill.bottom-U(4));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);r.right-=U(65);r.left+=U(11);DrawTextW(dc,text,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}else DrawTextW(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 if(draw->itemState&ODS_FOCUS){InflateRect(&r,-U(5),-U(5));DrawFocusRect(dc,&r);}return 1;
}
static void uiScaleMenu(void){HMENU m=CreatePopupMenu();int sizes[]={100,125,150,175,200};for(int i=0;i<5;i++){wchar_t label[24];swprintf(label,24,L"%d%%",sizes[i]);AppendMenuW(m,MF_STRING|(uiScale==sizes[i]?MF_CHECKED:0),1000+i,label);}RECT r;GetWindowRect(uiFontButton,&r);int chosen=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTALIGN,r.right,r.bottom,0,window,NULL);DestroyMenu(m);if(chosen>=1000&&chosen<1005){uiScale=sizes[chosen-1000];uiScrollX=uiScrollY=0;uiFonts();uiLayout(1);uiSaveSettings();}}
static void uiScroll(int horizontal,int command,int pos){int bar=horizontal?SB_HORZ:SB_VERT;SCROLLINFO si={sizeof(si),SIF_ALL};GetScrollInfo(window,bar,&si);int next=si.nPos;switch(command){case SB_LINEUP:next-=U(35);break;case SB_LINEDOWN:next+=U(35);break;case SB_PAGEUP:next-=si.nPage;break;case SB_PAGEDOWN:next+=si.nPage;break;case SB_THUMBPOSITION:case SB_THUMBTRACK:next=si.nTrackPos;break;}si.fMask=SIF_POS;si.nPos=next;SetScrollInfo(window,bar,&si,TRUE);if(horizontal)uiScrollX=GetScrollPos(window,bar);else uiScrollY=GetScrollPos(window,bar);uiLayout(0);}
static LRESULT CALLBACK uiDotProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){if(msg==WM_LBUTTONUP){PostMessageW(window,WM_COMMAND,2,0);return 0;}if(msg==WM_ERASEBKGND)return 1;if(msg==WM_PAINT){PAINTSTRUCT p;HDC dc=BeginPaint(w,&p);RECT r;GetClientRect(w,&r);HBRUSH clear=CreateSolidBrush(RGB(255,0,255));FillRect(dc,&r,clear);DeleteObject(clear);HBRUSH b=CreateSolidBrush(UI_SURFACE);HPEN pen=CreatePen(PS_SOLID,1,UI_LINE);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,pen);RoundRect(dc,1,1,r.right-1,r.bottom-1,D(22),D(22));SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(pen);b=CreateSolidBrush((GetTickCount64()/500)%2?RGB(255,44,67):RGB(140,27,42));ob=SelectObject(dc,b);Ellipse(dc,D(12),D(16),D(28),D(32));SelectObject(dc,ob);DeleteObject(b);wchar_t time[32];GetWindowTextW(timerLabel,time,32);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,UI_TEXT);SelectObject(dc,uiSmallFont);RECT text={D(38),0,r.right-D(25),r.bottom};DrawTextW(dc,time,-1,&text,DT_LEFT|DT_VCENTER|DT_SINGLELINE);RECT stop={r.right-D(23),D(18),r.right-D(11),D(30)};b=CreateSolidBrush(UI_TEXT);FillRect(dc,&stop,b);DeleteObject(b);EndPaint(w,&p);return 0;}return DefWindowProcW(w,msg,wp,lp);}
static void uiIndicator(int recording){if(!recording){if(uiDot){DestroyWindow(uiDot);uiDot=NULL;}uiDotAttempted=0;return;}if(uiDot){InvalidateRect(uiDot,NULL,FALSE);return;}if(uiDotAttempted)return;uiDotAttempted=1;
 typedef LONG (WINAPI *VersionFn)(OSVERSIONINFOW*);OSVERSIONINFOW ver={sizeof(ver)};VersionFn get=(VersionFn)(void*)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"RtlGetVersion");if(!get||get(&ver)!=0||ver.dwBuildNumber<19041)return;
 WNDCLASSW c={0};c.lpfnWndProc=uiDotProc;c.hInstance=GetModuleHandleW(NULL);c.lpszClassName=L"RecorderExcludedIndicator";c.hCursor=LoadCursorW(NULL,IDC_HAND);RegisterClassW(&c);MONITORINFO monitor={sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor);
 uiDot=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE,c.lpszClassName,L"Остановить запись",WS_POPUP,monitor.rcWork.right-D(195),monitor.rcWork.top+D(14),D(180),D(48),NULL,NULL,c.hInstance,NULL);
 if(!uiDot)return;DWORD affinity=0;if(!SetWindowDisplayAffinity(uiDot,0x11)||!GetWindowDisplayAffinity(uiDot,&affinity)||affinity!=0x11){DestroyWindow(uiDot);uiDot=NULL;return;}SetLayeredWindowAttributes(uiDot,RGB(255,0,255),255,LWA_COLORKEY);ShowWindow(uiDot,SW_SHOWNOACTIVATE);
}
static void uiDestroy(void){uiIndicator(0);HFONT fonts[]={uiFont,uiTitleFont,uiSmallFont,uiTimerFont};for(int i=0;i<4;i++)if(fonts[i])DeleteObject(fonts[i]);if(uiBgBrush)DeleteObject(uiBgBrush);if(uiSurfaceBrush)DeleteObject(uiSurfaceBrush);}
#endif
