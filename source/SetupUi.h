#pragma once
#include "SetupText.h"
#include "SetupOptions.h"
#include <algorithm>

namespace setupui {
inline HWND window = nullptr, folderEdit = nullptr, browseButton = nullptr,
            installButton = nullptr, languageBox = nullptr, menuCheck = nullptr;
inline setuptext::Language language = setuptext::Language::English;
inline HFONT regular = nullptr, smallFont = nullptr, heading = nullptr, brand = nullptr, bold = nullptr;
inline HBRUSH editBrush = nullptr, checkBrush = nullptr;
inline float scale = 1.0f;
inline bool chosen = false, busy = false, finished = false;
inline unsigned completed = 0, total = 1;
inline std::wstring file, selected;
inline bool addedLoader = false, useNewMenu = false, menuTouched = false;
inline void loadMenuPreference() {
    if (!folderEdit || !menuCheck || menuTouched) return;
    const int count = GetWindowTextLengthW(folderEdit);
    std::wstring path(static_cast<std::size_t>(count)+1,L'\0');
    GetWindowTextW(folderEdit,path.data(),count+1);path.resize(static_cast<std::size_t>(count));
    const auto ini = fs::path(path) / L"SA-People" / L"settings.ini";
    useNewMenu = fs::exists(ini) && setupoptions::newMenu(ini.c_str());
    SendMessageW(menuCheck,BM_SETCHECK,useNewMenu ? BST_CHECKED : BST_UNCHECKED,0);
}
inline const wchar_t *tr(setuptext::Key key) { return setuptext::get(language, key); }
inline int px(int value) { return static_cast<int>(value * scale); }
inline RECT rect(int x, int y, int w, int h) { return {px(x), px(y), px(x+w), px(y+h)}; }
inline void fill(HDC dc, int x, int y, int w, int h, COLORREF color) {
    auto brush = CreateSolidBrush(color);
    auto r = rect(x,y,w,h); FillRect(dc,&r,brush); DeleteObject(brush);
}
inline void text(HDC dc, const wchar_t *value, int x, int y, int w, int h, HFONT font,
                 COLORREF color, UINT flags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX) {
    auto previous = SelectObject(dc,font); SetTextColor(dc,color); SetBkMode(dc,TRANSPARENT);
    auto r=rect(x,y,w,h); DrawTextW(dc,value,-1,&r,flags); SelectObject(dc,previous);
}
inline void refresh() {
    SetWindowTextW(browseButton,tr(setuptext::Browse));
    SetWindowTextW(menuCheck,tr(setuptext::WithNewMenu));
    SetWindowTextW(installButton,tr(finished ? setuptext::Close : setuptext::Install));
    InvalidateRect(window,nullptr,FALSE);
}
inline void paint(HDC dc) {
    constexpr COLORREF white=RGB(243,239,227), muted=RGB(158,160,163), red=RGB(208,64,57);
    fill(dc,0,0,820,560,RGB(18,20,23));
    fill(dc,0,0,820,5,red);
    // A restrained skyline keeps the artwork local and the setup self-contained.
    for(int i=0;i<16;++i) {
        int h=28+(i*37)%68;
        fill(dc,400+i*27,118-h,20,h,RGB(27,30,34));
    }
    text(dc,L"SA PEOPLE",32,26,440,48,brand,white);
    text(dc,tr(setuptext::Subtitle),34,81,680,20,smallFont,muted);
    text(dc,L"SETUP",694,36,92,24,bold,red,DT_RIGHT|DT_SINGLELINE);
    fill(dc,32,116,756,1,RGB(52,55,59));
    text(dc,tr(finished ? setuptext::Complete : busy ? setuptext::Installing : setuptext::Headline),
         32,137,756,44,heading,white);
    text(dc,tr(finished && addedLoader ? setuptext::Loader : setuptext::Intro),34,187,752,35,regular,muted);
    const setuptext::Key titles[]={setuptext::People,setuptext::Phone,setuptext::Memory};
    const setuptext::Key bodies[]={setuptext::PeopleBody,setuptext::PhoneBody,setuptext::MemoryBody};
    const wchar_t *numbers[]={L"01",L"02",L"03"};
    for(int i=0;i<3;++i) {
        int x=32+i*256;
        fill(dc,x,233,244,103,RGB(27,30,34)); fill(dc,x,233,3,103,RGB(100,44,42));
        text(dc,numbers[i],x+16,245,35,20,smallFont,red);
        text(dc,tr(titles[i]),x+16,268,218,24,bold,white);
        text(dc,tr(bodies[i]),x+16,294,212,38,smallFont,muted);
    }
    text(dc,tr(setuptext::Folder),34,356,750,20,smallFont,muted);
    if(busy || finished) {
        fill(dc,32,386,756,34,RGB(27,30,34));
        text(dc,(finished?selected:file).c_str(),44,391,732,25,smallFont,white,
             DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        fill(dc,32,431,756,4,RGB(52,55,59));
        fill(dc,32,431,static_cast<int>(756ULL*completed/std::max(1u,total)),4,red);
    } else text(dc,tr(setuptext::Hint),34,430,752,18,smallFont,muted,
              DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    text(dc,tr(setuptext::Preserve),34,474,752,23,smallFont,RGB(167,180,154));
    fill(dc,32,500,756,1,RGB(52,55,59));
    text(dc,tr(setuptext::LanguageLabel),530,520,99,24,smallFont,muted,DT_RIGHT|DT_SINGLELINE);
}
inline LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch(message) {
    case DM_GETDEFID: return MAKELRESULT(102, DC_HASDEFID);
    case WM_PAINT: {
        PAINTSTRUCT ps; auto dc=BeginPaint(hwnd,&ps);
        RECT client; GetClientRect(hwnd,&client);
        auto memory=CreateCompatibleDC(dc); auto bitmap=CreateCompatibleBitmap(dc,client.right,client.bottom);
        auto old=SelectObject(memory,bitmap); paint(memory);
        BitBlt(dc,0,0,client.right,client.bottom,memory,0,0,SRCCOPY);
        SelectObject(memory,old); DeleteObject(bitmap); DeleteDC(memory); EndPaint(hwnd,&ps); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lp)==menuCheck) {
            SetTextColor(reinterpret_cast<HDC>(wp),RGB(243,239,227));
            SetBkColor(reinterpret_cast<HDC>(wp),RGB(18,20,23));
            return reinterpret_cast<LRESULT>(checkBrush);
        }
        break;
    case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(wp),RGB(243,239,227));
        SetBkColor(reinterpret_cast<HDC>(wp),RGB(35,38,43));
        return reinterpret_cast<LRESULT>(editBrush);
    case WM_DRAWITEM: {
        auto item=reinterpret_cast<DRAWITEMSTRUCT *>(lp);
        bool primary=item->CtlID==102;
        auto brush=CreateSolidBrush(primary?RGB(177,48,44):RGB(46,49,54));
        FillRect(item->hDC,&item->rcItem,brush); DeleteObject(brush);
        wchar_t label[120]={}; GetWindowTextW(item->hwndItem,label,120);
        auto previous=SelectObject(item->hDC,bold); SetBkMode(item->hDC,TRANSPARENT);
        SetTextColor(item->hDC,RGB(243,239,227));
        RECT r=item->rcItem; if(item->itemState&ODS_SELECTED) OffsetRect(&r,1,1);
        DrawTextW(item->hDC,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        if(item->itemState&ODS_FOCUS) {InflateRect(&r,-4,-4); DrawFocusRect(item->hDC,&r);}
        SelectObject(item->hDC,previous); return TRUE;
    }
    case WM_COMMAND:
        if(LOWORD(wp)==100 && HIWORD(wp)==EN_CHANGE && !busy && !finished) {
            loadMenuPreference(); return 0;
        }
        if(LOWORD(wp)==104 && HIWORD(wp)==BN_CLICKED) {
            menuTouched=true;
            useNewMenu=SendMessageW(menuCheck,BM_GETCHECK,0,0)==BST_CHECKED; return 0;
        }
        if(LOWORD(wp)==IDCANCEL) {if(!busy) DestroyWindow(hwnd); return 0;}
        if(LOWORD(wp)==103 && HIWORD(wp)==CBN_SELCHANGE) {
            auto index=SendMessageW(languageBox,CB_GETCURSEL,0,0);
            if(index>=0 && index<4) language=static_cast<setuptext::Language>(index);
            refresh(); return 0;
        }
        if(LOWORD(wp)==101 && !busy && !finished) {
            auto path=pickFolder(); if(!path.empty()) SetWindowTextW(folderEdit,path.c_str()); return 0;
        }
        if((LOWORD(wp)==102 || LOWORD(wp)==IDOK) && !busy) {
            if(finished) {DestroyWindow(hwnd); return 0;}
            int count=GetWindowTextLengthW(folderEdit);
            std::wstring path(static_cast<std::size_t>(count)+1,L'\0');
            GetWindowTextW(folderEdit,path.data(),count+1); path.resize(static_cast<std::size_t>(count));
            if(path.empty()) {path=pickFolder(); if(path.empty()) return 0; SetWindowTextW(folderEdit,path.c_str());}
            if(!fs::exists(fs::path(path)/L"gta_sa.exe")) {
                MessageBoxW(hwnd,tr(setuptext::Missing),Title,MB_ICONWARNING); return 0;
            }
            useNewMenu=SendMessageW(menuCheck,BM_GETCHECK,0,0)==BST_CHECKED;
            selected=path; chosen=true; return 0;
        }
        break;
    case WM_CLOSE: if(!busy) DestroyWindow(hwnd); return 0;
    case WM_DESTROY: window=nullptr; return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
inline void cleanup() {
    for(auto font:{regular,smallFont,heading,brand,bold}) if(font) DeleteObject(font);
    regular=smallFont=heading=brand=bold=nullptr;
    if(editBrush) DeleteObject(editBrush); editBrush=nullptr;
    if(checkBrush) DeleteObject(checkBrush); checkBrush=nullptr;
}
inline bool create() {
    SetProcessDPIAware();
    auto primary=PRIMARYLANGID(GetUserDefaultUILanguage());
    language=primary==LANG_TURKISH?setuptext::Language::Turkish:
        primary==LANG_RUSSIAN?setuptext::Language::Russian:
        primary==LANG_SPANISH?setuptext::Language::Spanish:setuptext::Language::English;
    auto dc=GetDC(nullptr); scale=GetDeviceCaps(dc,LOGPIXELSX)/96.0f; ReleaseDC(nullptr,dc);
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    scale=std::min(scale,std::min((work.right-work.left-40)/820.0f,(work.bottom-work.top-90)/560.0f));
    auto font=[](int height,int weight) {return CreateFontW(-px(height),0,0,0,weight,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");};
    regular=font(16,FW_NORMAL); smallFont=font(13,FW_NORMAL); heading=font(32,FW_SEMIBOLD);
    brand=font(39,FW_BLACK); bold=font(16,FW_SEMIBOLD); editBrush=CreateSolidBrush(RGB(35,38,43));
    checkBrush=CreateSolidBrush(RGB(18,20,23));
    WNDCLASSW wc{}; wc.lpfnWndProc=procedure; wc.hInstance=GetModuleHandleW(nullptr);
    wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512)); wc.hIcon=LoadIconW(nullptr,MAKEINTRESOURCEW(32512));
    wc.lpszClassName=L"SAPeopleSetup";
    if(!RegisterClassW(&wc)) return false;
    RECT size=rect(0,0,820,560); DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
    AdjustWindowRect(&size,style,FALSE);
    window=CreateWindowW(wc.lpszClassName,Title,style,
        work.left+((work.right-work.left)-(size.right-size.left))/2,
        work.top+((work.bottom-work.top)-(size.bottom-size.top))/2,
        size.right-size.left,size.bottom-size.top,nullptr,nullptr,wc.hInstance,nullptr);
    if(!window) return false;
    auto control=[&](const wchar_t *type,DWORD flags,int x,int y,int w,int h,int id) {
        auto child=CreateWindowExW(0,type,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|flags,
            px(x),px(y),px(w),px(h),window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),wc.hInstance,nullptr);
        SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(regular),TRUE); return child;
    };
    folderEdit=control(L"EDIT",ES_AUTOHSCROLL|WS_BORDER,32,386,626,34,100);
    browseButton=control(L"BUTTON",BS_OWNERDRAW,670,386,118,34,101);
    installButton=control(L"BUTTON",BS_OWNERDRAW,32,510,270,38,102);
    languageBox=control(L"COMBOBOX",CBS_DROPDOWNLIST|WS_VSCROLL,644,514,144,160,103);
    menuCheck=control(L"BUTTON",BS_AUTOCHECKBOX,34,451,752,23,104);
    SendMessageW(menuCheck,WM_SETFONT,reinterpret_cast<WPARAM>(smallFont),TRUE);
    SendMessageW(menuCheck,BM_SETCHECK,BST_UNCHECKED,0);
    for(auto name:setuptext::names) SendMessageW(languageBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(name));
    SendMessageW(languageBox,CB_SETCURSEL,static_cast<WPARAM>(language),0);
    // Suggest the setup's own directory only when it already contains the game.
    wchar_t module[32768]={};
    if(GetModuleFileNameW(nullptr,module,32768)) {
        auto beside=fs::path(module).parent_path();
        if(fs::exists(beside/L"gta_sa.exe")) SetWindowTextW(folderEdit,beside.c_str());
    }
    refresh(); ShowWindow(window,SW_SHOWNORMAL); UpdateWindow(window); SetFocus(folderEdit); return true;
}
inline std::wstring choose() {
    if(!create()) {cleanup(); throw std::runtime_error("Setup window unavailable");}
    MSG message{};
    while(window && !chosen && GetMessageW(&message,nullptr,0,0)>0)
        if(!IsDialogMessageW(window,&message)) {TranslateMessage(&message); DispatchMessageW(&message);}
    if(!chosen) {cleanup(); return {};}
    busy=true; ShowWindow(folderEdit,SW_HIDE); ShowWindow(browseButton,SW_HIDE);
    EnableWindow(installButton,FALSE); EnableWindow(languageBox,FALSE); EnableWindow(menuCheck,FALSE); refresh(); UpdateWindow(window);
    return selected;
}
inline void progress(unsigned value,unsigned count,const std::wstring &path) {
    if(!window) return;
    completed=value; total=count; file=path; InvalidateRect(window,nullptr,FALSE); UpdateWindow(window);
}
inline void finish() {
    if(!window) return;
    busy=false; finished=true; completed=total; EnableWindow(installButton,TRUE);
    EnableWindow(languageBox,TRUE); refresh(); SetFocus(installButton);
    MSG message{};
    while(window && GetMessageW(&message,nullptr,0,0)>0)
        if(!IsDialogMessageW(window,&message)) {TranslateMessage(&message); DispatchMessageW(&message);}
    cleanup();
}
inline void close() {if(window) {busy=false; DestroyWindow(window);} cleanup();}
} // namespace setupui
