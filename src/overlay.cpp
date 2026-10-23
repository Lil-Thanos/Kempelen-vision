#include "overlay.h"

static const COLORREF KEY = RGB(255, 0, 255);   

static LRESULT CALLBACK OverlayProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return DefWindowProcW(h, m, w, l);
}

Overlay::Overlay() {
    w_ = GetSystemMetrics(SM_CXSCREEN);
    h_ = GetSystemMetrics(SM_CYSCREEN);
    HINSTANCE inst = GetModuleHandleW(nullptr);

    WNDCLASSW wc{};
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"ChessOverlay";
    wc.hbrBackground = CreateSolidBrush(KEY);
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"ChessOverlay", L"", WS_POPUP,
        0, 0, w_, h_, nullptr, nullptr, inst, nullptr);

    SetLayeredWindowAttributes(hwnd_, KEY, 0, LWA_COLORKEY);   // magenta = invisible
    SetWindowDisplayAffinity(hwnd_, WDA_EXCLUDEFROMCAPTURE);   // invisible pour DXGI
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
}

Overlay::~Overlay() { if (hwnd_) DestroyWindow(hwnd_); }

void Overlay::drawGrid(const std::optional<cv::Rect>& board) {
    HDC dc = GetDC(hwnd_);

    // on efface tout (remplit de la couleur transparente)
    HBRUSH bg = CreateSolidBrush(KEY);
    RECT all{ 0, 0, w_, h_ };
    FillRect(dc, &all, bg);
    DeleteObject(bg);

    if (board) {
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
        HGDIOBJ old = SelectObject(dc, pen);
        SelectObject(dc, GetStockObject(NULL_BRUSH));

        Rectangle(dc, board->x, board->y,
            board->x + board->width, board->y + board->height);
        for (int i = 1; i < 8; ++i) {
            int x = board->x + i * board->width / 8;
            int y = board->y + i * board->height / 8;
            MoveToEx(dc, x, board->y, nullptr);
            LineTo(dc, x, board->y + board->height);
            MoveToEx(dc, board->x, y, nullptr);
            LineTo(dc, board->x + board->width, y);
        }
        SelectObject(dc, old);
        DeleteObject(pen);
    }
    ReleaseDC(hwnd_, dc);
}

void Overlay::pump() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}