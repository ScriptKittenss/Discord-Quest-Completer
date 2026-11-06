#include <windows.h>
#include <shellapi.h>

#define ID_BTN_GITHUB     1002
#define ID_BTN_DISCORD    1005
#define WM_TRAY_MESSAGE   (WM_USER + 1)
#define ID_TRAY_EXIT      1003
#define ID_TRAY_TOGGLE    1004

typedef BOOL(WINAPI* P_Shell_NotifyIconW)(DWORD, PNOTIFYICONDATAW);
typedef HINSTANCE(WINAPI* P_ShellExecuteW)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
typedef HFONT(WINAPI* P_CreateFontW)(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR);
typedef BOOL(WINAPI* P_DeleteObject)(HGDIOBJ);
typedef HWND(WINAPI* P_CreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
typedef HBRUSH(WINAPI* P_CreateSolidBrush)(COLORREF);
typedef COLORREF(WINAPI* P_SetTextColor)(HDC, COLORREF);
typedef int(WINAPI* P_SetBkMode)(HDC, int);
typedef HRESULT(WINAPI* P_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
typedef int(WINAPI* P_FillRect)(HDC, const RECT*, HBRUSH);
typedef int(WINAPI* P_FrameRect)(HDC, const RECT*, HBRUSH);
typedef int(WINAPI* P_DrawTextW)(HDC, LPCWSTR, int, LPRECT, UINT);
typedef int(WINAPI* P_GetWindowTextW)(HWND, LPWSTR, int);
typedef HGDIOBJ(WINAPI* P_SelectObject)(HDC, HGDIOBJ);

P_Shell_NotifyIconW f_Shell_NotifyIconW = NULL;
P_ShellExecuteW f_ShellExecuteW = NULL;
P_CreateFontW f_CreateFontW = NULL;
P_DeleteObject f_DeleteObject = NULL;
P_CreateSolidBrush f_CreateSolidBrush = NULL;
P_SetTextColor f_SetTextColor = NULL;
P_SetBkMode f_SetBkMode = NULL;
P_DwmSetWindowAttribute f_DwmSetWindowAttribute = NULL;
P_FillRect f_FillRect = NULL;
P_FrameRect f_FrameRect = NULL;
P_DrawTextW f_DrawTextW = NULL;
P_GetWindowTextW f_GetWindowTextW = NULL;
P_SelectObject f_SelectObject = NULL;

wchar_t g_szGameName[256] = L"Discord Game";
const wchar_t* GITHUB_URL = L"https://github.com/ScriptKittenss/Discord-Quest-Completer";
const wchar_t* DISCORD_URL = L"https://discord.gg/2v9F7bp94F";
NOTIFYICONDATAW nid = { 0 };
HFONT hFontTitle = NULL, hFontGame = NULL, hFontStatus = NULL, hFontLink = NULL, hFontBtn = NULL;
HBRUSH hDarkBrush = NULL;

HWND hTitleText = NULL;
HWND hGameText = NULL;
HWND hStatusText = NULL;
HWND hLinkText = NULL;
HWND hBtnGit = NULL;
HWND hBtnDisc = NULL;

wchar_t* FindStringW(const wchar_t* str, const wchar_t* substr) {
    while (*str) {
        const wchar_t* h = str, * n = substr;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (wchar_t*)str;
        str++;
    }
    return NULL;
}

void ToggleWindow(HWND hWnd) {
    if (IsWindowVisible(hWnd)) {
        ShowWindow(hWnd, SW_HIDE);
    }
    else {
        ShowWindow(hWnd, SW_SHOW);
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        hFontTitle = f_CreateFontW(17, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        hFontGame = f_CreateFontW(21, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        hFontStatus = f_CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        hFontLink = f_CreateFontW(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Consolas");
        hFontBtn = f_CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");

        hTitleText = CreateWindowExW(0, L"STATIC", L"Script Kittens Quest Completer", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 18, 440, 22, hWnd, NULL, NULL, NULL);
        hGameText = CreateWindowExW(0, L"STATIC", g_szGameName, WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 46, 440, 28, hWnd, NULL, NULL, NULL);
        hStatusText = CreateWindowExW(0, L"STATIC", L"Active Quest Emulation  |  Simulating In-Game", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 80, 420, 20, hWnd, NULL, NULL, NULL);
        hLinkText = CreateWindowExW(0, L"STATIC", L"github.com/ScriptKittenss/Discord-Quest-Completer", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 104, 420, 18, hWnd, NULL, NULL, NULL);

        // Owner-drawn buttons for custom sleek styling
        hBtnGit = CreateWindowExW(0, L"BUTTON", L"View on GitHub", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 45, 136, 165, 36, hWnd, (HMENU)ID_BTN_GITHUB, NULL, NULL);
        hBtnDisc = CreateWindowExW(0, L"BUTTON", L"Join Discord", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 230, 136, 165, 36, hWnd, (HMENU)ID_BTN_DISCORD, NULL, NULL);

        SendMessageW(hTitleText, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
        SendMessageW(hGameText, WM_SETFONT, (WPARAM)hFontGame, TRUE);
        SendMessageW(hStatusText, WM_SETFONT, (WPARAM)hFontStatus, TRUE);
        SendMessageW(hLinkText, WM_SETFONT, (WPARAM)hFontLink, TRUE);

        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hWnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAY_MESSAGE;
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        lstrcpynW(nid.szTip, g_szGameName, 128);
        if (f_Shell_NotifyIconW) f_Shell_NotifyIconW(NIM_ADD, &nid);
    } break;

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        if (f_FillRect && hDarkBrush) {
            f_FillRect(hdc, &rc, hDarkBrush);
            return 1;
        }
        break;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS && pDIS->CtlType == ODT_BUTTON && f_FillRect && f_FrameRect && f_CreateSolidBrush && f_DeleteObject) {
            HDC hdc = pDIS->hDC;
            RECT rc = pDIS->rcItem;
            BOOL isPressed = (pDIS->itemState & ODS_SELECTED);
            
            HBRUSH hBg = NULL;
            HBRUSH hBorder = NULL;
            COLORREF textColor = RGB(255, 255, 255);

            if (pDIS->CtlID == ID_BTN_DISCORD) {
                hBg = f_CreateSolidBrush(isPressed ? RGB(71, 82, 196) : RGB(88, 101, 242));
                hBorder = f_CreateSolidBrush(isPressed ? RGB(88, 101, 242) : RGB(108, 120, 255));
            } else {
                hBg = f_CreateSolidBrush(isPressed ? RGB(25, 28, 38) : RGB(34, 38, 50));
                hBorder = f_CreateSolidBrush(isPressed ? RGB(45, 50, 68) : RGB(56, 62, 82));
                textColor = RGB(235, 238, 245);
            }

            f_FillRect(hdc, &rc, hBg);
            f_FrameRect(hdc, &rc, hBorder);
            f_DeleteObject(hBg);
            f_DeleteObject(hBorder);

            if (f_SetBkMode) f_SetBkMode(hdc, TRANSPARENT);
            if (f_SetTextColor) f_SetTextColor(hdc, textColor);
            if (f_SelectObject && hFontBtn) f_SelectObject(hdc, hFontBtn);

            wchar_t btnText[64] = { 0 };
            if (f_GetWindowTextW) {
                f_GetWindowTextW(pDIS->hwndItem, btnText, 64);
            }
            if (f_DrawTextW) {
                f_DrawTextW(hdc, btnText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            return TRUE;
        }
        break;
    }

    case WM_TRAY_MESSAGE:
        if (lParam == WM_RBUTTONUP) {
            POINT cur; GetCursorPos(&cur);
            HMENU hMenu = CreatePopupMenu();
            InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_TOGGLE, IsWindowVisible(hWnd) ? L"Hide" : L"Show");
            InsertMenuW(hMenu, 2, MF_SEPARATOR, 0, NULL);
            InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cur.x, cur.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK) ToggleWindow(hWnd);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_GITHUB) {
            if (f_ShellExecuteW) f_ShellExecuteW(NULL, L"open", GITHUB_URL, NULL, NULL, SW_SHOWNORMAL);
        }
        if (LOWORD(wParam) == ID_BTN_DISCORD) {
            if (f_ShellExecuteW) f_ShellExecuteW(NULL, L"open", DISCORD_URL, NULL, NULL, SW_SHOWNORMAL);
        }
        if (LOWORD(wParam) == ID_TRAY_EXIT) DestroyWindow(hWnd);
        if (LOWORD(wParam) == ID_TRAY_TOGGLE) ToggleWindow(hWnd);
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        if (f_SetBkMode) f_SetBkMode(hdc, TRANSPARENT);
        
        if (hwndStatic == hGameText) {
            if (f_SetTextColor) f_SetTextColor(hdc, RGB(240, 82, 103)); // Vibrant crimson
        } else if (hwndStatic == hStatusText) {
            if (f_SetTextColor) f_SetTextColor(hdc, RGB(52, 211, 153)); // Emerald green
        } else if (hwndStatic == hTitleText) {
            if (f_SetTextColor) f_SetTextColor(hdc, RGB(160, 166, 178)); // Subtle off-white
        } else {
            if (f_SetTextColor) f_SetTextColor(hdc, RGB(115, 122, 138)); // Slate gray
        }
        return (LRESULT)hDarkBrush;
    }

    case WM_DESTROY:
        if (f_Shell_NotifyIconW) f_Shell_NotifyIconW(NIM_DELETE, &nid);
        if (hFontTitle && f_DeleteObject) f_DeleteObject(hFontTitle);
        if (hFontGame && f_DeleteObject) f_DeleteObject(hFontGame);
        if (hFontStatus && f_DeleteObject) f_DeleteObject(hFontStatus);
        if (hFontLink && f_DeleteObject) f_DeleteObject(hFontLink);
        if (hFontBtn && f_DeleteObject) f_DeleteObject(hFontBtn);
        if (hDarkBrush && f_DeleteObject) f_DeleteObject(hDarkBrush);
        PostQuitMessage(0);
        break;

    default: return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

extern "C" {
    #pragma function(memset)
    void* memset(void* dest, int c, size_t count) {
        char* bytes = (char*)dest;
        while (count--) {
            *bytes++ = (char)c;
        }
        return dest;
    }
}

extern "C" void mainEntryPoint() {
    HINSTANCE hInst = GetModuleHandleW(NULL);

    HMODULE hShell32 = LoadLibraryW(L"shell32.dll");
    HMODULE hGdi32 = LoadLibraryW(L"gdi32.dll");
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");

    f_Shell_NotifyIconW = (P_Shell_NotifyIconW)GetProcAddress(hShell32, "Shell_NotifyIconW");
    f_ShellExecuteW = (P_ShellExecuteW)GetProcAddress(hShell32, "ShellExecuteW");
    f_CreateFontW = (P_CreateFontW)GetProcAddress(hGdi32, "CreateFontW");
    f_DeleteObject = (P_DeleteObject)GetProcAddress(hGdi32, "DeleteObject");
    f_CreateSolidBrush = (P_CreateSolidBrush)GetProcAddress(hGdi32, "CreateSolidBrush");
    f_SetTextColor = (P_SetTextColor)GetProcAddress(hGdi32, "SetTextColor");
    f_SetBkMode = (P_SetBkMode)GetProcAddress(hGdi32, "SetBkMode");
    f_SelectObject = (P_SelectObject)GetProcAddress(hGdi32, "SelectObject");
    f_FillRect = (P_FillRect)GetProcAddress(hUser32, "FillRect");
    f_FrameRect = (P_FrameRect)GetProcAddress(hUser32, "FrameRect");
    f_DrawTextW = (P_DrawTextW)GetProcAddress(hUser32, "DrawTextW");
    f_GetWindowTextW = (P_GetWindowTextW)GetProcAddress(hUser32, "GetWindowTextW");
    P_CreateWindowExW f_CreateWindowExW = (P_CreateWindowExW)GetProcAddress(hUser32, "CreateWindowExW");

    if (hDwm) {
        f_DwmSetWindowAttribute = (P_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
    }

    if (f_CreateSolidBrush) {
        hDarkBrush = f_CreateSolidBrush(RGB(17, 19, 25)); // Deep dark background
    }

    LPWSTR lpCmdLine = GetCommandLineW();

    // Security check
    const wchar_t* authFlag = L"--auth";
    wchar_t* authPos = FindStringW(lpCmdLine, authFlag);
    if (!authPos || !FindStringW(lpCmdLine, L"1shot-1moan-official")) {
        MessageBoxW(NULL,
            L"Protected Build by 1shot-1moan / Script Kittens.\n\nDirect manual execution or unverified forks are locked.\nDownload the official releases from:\nhttps://github.com/ScriptKittenss/Discord-Quest-Completer",
            L"Script Kittens Protection",
            MB_ICONWARNING | MB_OK);
        if (f_ShellExecuteW) {
            f_ShellExecuteW(NULL, L"open", GITHUB_URL, NULL, NULL, SW_SHOWNORMAL);
        }
        ExitProcess(1);
    }

    const wchar_t* flag = L"--title";
    wchar_t* pos = FindStringW(lpCmdLine, flag);

    if (pos) {
        pos += 7;
        while (*pos == L' ') pos++;

        if (*pos == L'"') {
            pos++; 
            int i = 0;
            while (*pos != L'"' && *pos != L'\0' && i < 255) {
                g_szGameName[i++] = *pos++;
            }
            g_szGameName[i] = L'\0';
        } else {
            int i = 0;
            while (*pos != L' ' && *pos != L'\0' && i < 255) {
                g_szGameName[i++] = *pos++;
            }
            g_szGameName[i] = L'\0';
        }
    }

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"SKQuestWindow";
    wc.hbrBackground = hDarkBrush ? hDarkBrush : (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hWnd = f_CreateWindowExW(0, wc.lpszClassName, g_szGameName, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 455, 235, NULL, NULL, hInst, NULL);

    // Apply dark title bar on Windows 10/11
    if (f_DwmSetWindowAttribute) {
        BOOL dark = TRUE;
        f_DwmSetWindowAttribute(hWnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE
        f_DwmSetWindowAttribute(hWnd, 19, &dark, sizeof(dark)); // older Win10 build
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ExitProcess(0);
}