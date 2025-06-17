#include <windows.h>
#include <shellapi.h>
#include <time.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

NOTIFYICONDATA nid;
HWND hwnd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Ajoute l'icône dans la zone de notification
void AddTrayIcon(HINSTANCE hInstance) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1)); // ID de l'icône = 1
    lstrcpy(nid.szTip, TEXT("NoSleep actif"));
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Supprime l'icône à la sortie
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// Simulation appui sur AltGr (VK_RMENU)
void SimulateAltGrPress() {
    INPUT inputs[2] = {0};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RMENU;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RMENU;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}
void SimulateMouseMove() {
    INPUT inputs[2] = {0};

    // Move cursor forward
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = 50;
    inputs[0].mi.dy = 50;
    inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;

    // Move cursor backward
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dx = -50;
    inputs[1].mi.dy = -50;
    inputs[1].mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(2, inputs, sizeof(INPUT));
}
DWORD GetIdleTime() {
    LASTINPUTINFO lii = {0};
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        return GetTickCount() - lii.dwTime; // Temps écoulé depuis la dernière interaction
    }
    return 0; // En cas d'échec
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "NoSleepClass";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    hwnd = CreateWindowEx(0, CLASS_NAME, "NoSleep", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    AddTrayIcon(hInstance);

    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    // Hotkey globale : Ctrl + Shift + Q
    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 0x51); // 0x51 = Q

    DWORD lastSimTime = GetTickCount();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Vérifie l'inactivité utilisateur
        DWORD idleTime = GetIdleTime();
        if (idleTime > 120000) { // 2 minutes d'inactivité
            if (GetTickCount() - lastSimTime > 60000 * 2) { // Toutes les 4 minutes
                SimulateAltGrPress();
                SimulateMouseMove();
                lastSimTime = GetTickCount();
            }
        }
    }

    SetThreadExecutionState(ES_CONTINUOUS);
    RemoveTrayIcon();
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON && lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        HMENU menu = CreatePopupMenu();
        AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, "Quitter NoSleep");
        SetForegroundWindow(hwnd);
        TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(menu);
    }
    else if (msg == WM_COMMAND && LOWORD(wParam) == ID_TRAY_EXIT) {
        PostQuitMessage(0);
    }
    else if (msg == WM_HOTKEY && wParam == 1) {
        PostQuitMessage(0); // Ctrl + Shift + Q
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
