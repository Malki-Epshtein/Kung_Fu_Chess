#include "RoomDialog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cctype>
#include <iostream>

namespace {
    constexpr int kEditId    = 101;
    constexpr int kCreateId  = 102;
    constexpr int kJoinId    = 103;
    constexpr int kCancelId  = 104;

    const wchar_t* kClassName = L"KfcRoomDialogClass";

    // The dialog only ever runs one at a time (blocking, on the caller's
    // thread), so a couple of statics to bridge the WndProc callback are
    // simpler than threading state through via GWLP_USERDATA for this.
    HWND               g_editHwnd = nullptr;
    RoomDialogResult*  g_result   = nullptr;

    std::string wideToUtf8(const std::wstring& wide) {
        if (wide.empty())
            return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    std::string readEditText(HWND editHwnd) {
        int len = GetWindowTextLengthW(editHwnd);
        if (len <= 0)
            return {};
        std::wstring buffer(len, L'\0');
        GetWindowTextW(editHwnd, buffer.data(), len + 1);
        return wideToUtf8(buffer);
    }

    // OpenCV's canvas text rendering (cv::putText, Hershey fonts) can only
    // draw ASCII - the room name gets shown there later (Stage G2c), so it
    // has to be restricted to what that can actually render. The Win32
    // EDIT control itself handles Hebrew/Unicode input fine; this is
    // purely about what the OTHER half of the UI (OpenCV) can display.
    bool isValidRoomName(const std::string& name) {
        if (name.empty())
            return false;
        for (char c : name) {
            if (!std::isalnum(static_cast<unsigned char>(c)))
                return false;
        }
        return true;
    }

    void finish(HWND dialogHwnd, RoomDialogResult::Action action) {
        if (action != RoomDialogResult::Action::Cancel) {
            std::string typed = readEditText(g_editHwnd);
            if (typed.empty()) {
                // Matches Cancel's behavior (no message needed to send) -
                // just close, nothing to validate.
                if (g_result) {
                    g_result->action = RoomDialogResult::Action::Cancel;
                    g_result->roomName = std::string();
                }
                DestroyWindow(dialogHwnd);
                return;
            }
            if (!isValidRoomName(typed)) {
                MessageBoxW(dialogHwnd, L"Room name must use only English letters/digits.", L"Invalid room name", MB_OK | MB_ICONWARNING);
                return; // dialog stays open so the user can fix it
            }
            if (g_result) {
                g_result->action   = action;
                g_result->roomName = typed;
            }
        } else if (g_result) {
            g_result->action = action;
            g_result->roomName = std::string();
        }
        DestroyWindow(dialogHwnd);
    }

    LRESULT CALLBACK dialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_CREATE: {
                CreateWindowExW(0, L"STATIC", L"Room name (English letters/digits):", WS_CHILD | WS_VISIBLE,
                                 20, 20, 260, 20, hwnd, nullptr, nullptr, nullptr);
                g_editHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                 20, 45, 260, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Create", WS_CHILD | WS_VISIBLE,
                                 20, 85, 80, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreateId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Join", WS_CHILD | WS_VISIBLE,
                                 110, 85, 80, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kJoinId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
                                 200, 85, 80, 30, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelId)), nullptr, nullptr);
                return 0;
            }
            case WM_COMMAND: {
                if (HIWORD(wParam) == BN_CLICKED) {
                    int id = LOWORD(wParam);
                    if (id == kCreateId)      finish(hwnd, RoomDialogResult::Action::Create);
                    else if (id == kJoinId)   finish(hwnd, RoomDialogResult::Action::Join);
                    else if (id == kCancelId) finish(hwnd, RoomDialogResult::Action::Cancel);
                }
                return 0;
            }
            case WM_CLOSE:
                finish(hwnd, RoomDialogResult::Action::Cancel);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void registerClassOnce() {
        static bool registered = false;
        if (registered)
            return;
        WNDCLASSW wc{};
        wc.lpfnWndProc   = dialogWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.lpszClassName = kClassName;
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_BTNFACE + 1));
        RegisterClassW(&wc);
        registered = true;
    }

    const char* actionName(RoomDialogResult::Action action) {
        switch (action) {
            case RoomDialogResult::Action::Create: return "Create";
            case RoomDialogResult::Action::Join:   return "Join";
            default:                               return "Cancel";
        }
    }
}

RoomDialogResult showRoomDialog() {
    registerClassOnce();

    RoomDialogResult result;
    g_result   = &result;
    g_editHwnd = nullptr;

    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, kClassName, L"Room",
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 160,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindow(hwnd))
            break;
    }

    g_result = nullptr;
    std::cout << "[client] Room dialog result: " << actionName(result.action)
               << " name='" << result.roomName << "'" << std::endl;
    return result;
}
