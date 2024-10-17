#include <windows.h>
#include <chrono>
#include <unordered_map>
#include <strsafe.h>
#include "resource.h"

using namespace std;
using namespace chrono;

const milliseconds DEFAULT_SPACE_BAR_THRESHOLD(300);
const milliseconds DEFAULT_OTHER_KEYS_THRESHOLD(60);

milliseconds space_bar_threshold = DEFAULT_SPACE_BAR_THRESHOLD;
milliseconds other_keys_threshold = DEFAULT_OTHER_KEYS_THRESHOLD;

unordered_map<int, steady_clock::time_point> keyPressTimes;
NOTIFYICONDATA nid;
HINSTANCE hInstance;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        int vkCode = pKeyBoard->vkCode;

        auto currentTime = steady_clock::now();

        if (keyPressTimes.find(vkCode) != keyPressTimes.end()) {
            auto timeDifference = duration_cast<milliseconds>(currentTime - keyPressTimes[vkCode]);

            if ((vkCode == VK_SPACE && timeDifference < space_bar_threshold) ||
                (vkCode != VK_SPACE && timeDifference < other_keys_threshold)) {
                return 1;  // Block the key press
            }
        }

        keyPressTimes[vkCode] = currentTime;
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void createTrayIcon(HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP;
    nid.hIcon = (HICON)LoadImage(NULL, L"C:\\Users\\totir\\source\\repos\\KeyboardSniffer\\keyboard.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    
    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), TEXT("Keystroke Filter"));

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void showMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, TEXT("Close"));

    SetForegroundWindow(hWnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, nullptr);

    if (cmd == 1) {
        PostQuitMessage(0);  // Exit the application
    }
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_APP:
        if (lParam == WM_RBUTTONUP) {
            showMenu(hWnd);
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInstance = hInst;
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);

    // Create a hidden window
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("HiddenWindow");
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(wc.lpszClassName, TEXT("Hidden Window"), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    createTrayIcon(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hhkLowLevelKybd);
    return 0;
}
