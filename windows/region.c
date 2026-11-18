#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "region.h"
static HWND overlay; static HBITMAP screenshot; static HDC picture;
static int vx,vy,vw,vh,dragging,finishing; static POINT anchor,current;
static RegionResult complete;
static RECT selection(void) {
    RECT r={min(anchor.x,current.x),min(anchor.y,current.y),max(anchor.x,current.x),max(anchor.y,current.y)};
    r.left=max(0,r.left);r.top=max(0,r.top);r.right=min(vw,r.right);r.bottom=min(vh,r.bottom);return r;
}
static void end(int accepted) {
    if(finishing) return; finishing=1;
    RECT r=selection(); ReleaseCapture(); DestroyWindow(overlay); overlay=NULL;
    if(picture) { DeleteDC(picture);picture=NULL; } if(screenshot) { DeleteObject(screenshot); screenshot=NULL; }
    complete(vx+r.left,vy+r.top,r.right-r.left,r.bottom-r.top,accepted);
}
static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT p; HDC dc=BeginPaint(hwnd,&p);
        PatBlt(dc,0,0,vw,vh,BLACKNESS); BLENDFUNCTION blend={AC_SRC_OVER,0,110,0};
        AlphaBlend(dc,0,0,vw,vh,picture,0,0,vw,vh,blend);
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));
        RECT title={max(15,-vx+24),max(15,-vy+24),vw-15,max(15,-vy+24)+45};
        DrawTextW(dc,L"Выделите область мышью. Отпустите кнопку — начнётся запись. Esc — отмена.",-1,&title,DT_LEFT|DT_WORDBREAK);
        if(dragging) {
            RECT r=selection(); BitBlt(dc,r.left,r.top,r.right-r.left,r.bottom-r.top,picture,r.left,r.top,SRCCOPY);
            HPEN pen=CreatePen(PS_SOLID,3,RGB(0,200,255)); HGDIOBJ oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));
            Rectangle(dc,r.left,r.top,r.right,r.bottom);SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);
            wchar_t text[100];swprintf(text,100,L"%d × %d",r.right-r.left,r.bottom-r.top);
            RECT size={r.left+8,max(0,r.top-28),min(vw,r.left+220),r.top};DrawTextW(dc,text,-1,&size,DT_LEFT|DT_SINGLELINE);
        }
        EndPaint(hwnd,&p);return 0;
    }
    case WM_LBUTTONDOWN:
        dragging=1;anchor.x=GET_X_LPARAM(lp);anchor.y=GET_Y_LPARAM(lp);current=anchor;SetCapture(hwnd);InvalidateRect(hwnd,NULL,FALSE);return 0;
    case WM_MOUSEMOVE:
        if(dragging) { current.x=GET_X_LPARAM(lp);current.y=GET_Y_LPARAM(lp);InvalidateRect(hwnd,NULL,FALSE); }return 0;
    case WM_LBUTTONUP:
        if(dragging) { current.x=GET_X_LPARAM(lp);current.y=GET_Y_LPARAM(lp);RECT r=selection();
            if(r.right-r.left>=32 && r.bottom-r.top>=32) end(1);
            else { dragging=0;ReleaseCapture();InvalidateRect(hwnd,NULL,FALSE); } }return 0;
    case WM_KEYDOWN: if(wp==VK_ESCAPE) end(0);return 0;
    case WM_CLOSE: end(0);return 0;
    case WM_CAPTURECHANGED: if(dragging && !finishing) end(0);return 0;
    }return DefWindowProcW(hwnd,msg,wp,lp);
}
int region_show(HINSTANCE instance,RegionResult result) {
    complete=result;dragging=finishing=0;anchor.x=anchor.y=current.x=current.y=0;
    vx=GetSystemMetrics(SM_XVIRTUALSCREEN);vy=GetSystemMetrics(SM_YVIRTUALSCREEN);
    vw=GetSystemMetrics(SM_CXVIRTUALSCREEN);vh=GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HDC dc=GetDC(NULL);picture=CreateCompatibleDC(dc);screenshot=CreateCompatibleBitmap(dc,vw,vh);
    if(!picture || !screenshot) { if(picture)DeleteDC(picture);if(screenshot)DeleteObject(screenshot);ReleaseDC(NULL,dc);return 0; }
    SelectObject(picture,screenshot);BOOL ok=BitBlt(picture,0,0,vw,vh,dc,vx,vy,SRCCOPY|CAPTUREBLT);ReleaseDC(NULL,dc);
    if(!ok) { DeleteDC(picture);DeleteObject(screenshot);return 0; }
    WNDCLASSW wc={0};wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=L"ScreenRecorderRegion";wc.hCursor=LoadCursorW(NULL,IDC_CROSS);RegisterClassW(&wc);
    overlay=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"Выбор области",WS_POPUP,vx,vy,vw,vh,NULL,NULL,instance,NULL);
    if(!overlay) { DeleteDC(picture);DeleteObject(screenshot);return 0; }
    ShowWindow(overlay,SW_SHOW);SetForegroundWindow(overlay);SetFocus(overlay);UpdateWindow(overlay);return 1;
}
