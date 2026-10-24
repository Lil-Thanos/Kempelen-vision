#include "overlay.h"
#include <cmath>
#include <algorithm>

static const COLORREF KEY = RGB(255, 0, 255);   // couleur rendue transparente

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

    SetLayeredWindowAttributes(hwnd_, KEY, 0, LWA_COLORKEY);
    SetWindowDisplayAffinity(hwnd_, WDA_EXCLUDEFROMCAPTURE);
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);

    // surface de dessin hors écran (double buffering)
    HDC scr = GetDC(hwnd_);
    memDC_ = CreateCompatibleDC(scr);
    bmp_ = CreateCompatibleBitmap(scr, w_, h_);
    oldBmp_ = (HBITMAP)SelectObject(memDC_, bmp_);
    ReleaseDC(hwnd_, scr);
}

Overlay::~Overlay() {
    if (memDC_) {
        SelectObject(memDC_, oldBmp_);
        DeleteObject(bmp_);
        DeleteDC(memDC_);
    }
    if (hwnd_) DestroyWindow(hwnd_);
}

// centre d'une case ("e", '2') en pixels écran
static POINT squareCenter(const cv::Rect& b, char file, char rank, bool whiteAtBottom)
{
    int f = file - 'a', r = rank - '1';
    int col = whiteAtBottom ? f : 7 - f;
    int row = whiteAtBottom ? 7 - r : r;
    return { b.x + (LONG)((col + 0.5) * b.width / 8.0),
             b.y + (LONG)((row + 0.5) * b.height / 8.0) };
}

static void drawArrow(HDC dc, const cv::Rect& b, const std::string& mv, bool whiteAtBottom)
{
    POINT from = squareCenter(b, mv[0], mv[1], whiteAtBottom);
    POINT to = squareCenter(b, mv[2], mv[3], whiteAtBottom);

    double dx = to.x - from.x, dy = to.y - from.y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1) return;
    dx /= len; dy /= len;

    double sq = b.width / 8.0;
    double headLen = sq * 0.45, headHalfW = sq * 0.28;
    POINT base = { to.x - (LONG)(dx * headLen), to.y - (LONG)(dy * headLen) };

    COLORREF col = RGB(255, 140, 0);   // orange (jamais le magenta : c'est la clé de transparence)
    HPEN pen = CreatePen(PS_SOLID, std::max(4, (int)(sq * 0.12)), col);
    HBRUSH br = CreateSolidBrush(col);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBr = SelectObject(dc, br);

    MoveToEx(dc, from.x, from.y, nullptr);
    LineTo(dc, base.x, base.y);

    POINT tri[3] = {
        to,
        { base.x + (LONG)(-dy * headHalfW), base.y + (LONG)(dx * headHalfW) },
        { base.x - (LONG)(-dy * headHalfW), base.y - (LONG)(dx * headHalfW) }
    };
    Polygon(dc, tri, 3);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBr);
    DeleteObject(pen);
    DeleteObject(br);
}

void Overlay::drawGrid(const std::optional<cv::Rect>& board,
    const std::string& move, bool whiteAtBottom)
{
    // on ne redessine que si l'état a changé (ou toutes les 2 s par sécurité)
    ULONGLONG now = GetTickCount64();
    bool same = drawnOnce_ && lastBoard_ == board && lastMove_ == move
        && lastBottom_ == whiteAtBottom;
    if (same && now - lastDraw_ < 2000) return;
    lastBoard_ = board; lastMove_ = move; lastBottom_ = whiteAtBottom;
    drawnOnce_ = true; lastDraw_ = now;

    // 1) tout est dessiné hors écran
    HBRUSH bg = CreateSolidBrush(KEY);
    RECT all{ 0, 0, w_, h_ };
    FillRect(memDC_, &all, bg);
    DeleteObject(bg);

    if (board) {
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
        HGDIOBJ old = SelectObject(memDC_, pen);
        HGDIOBJ oldBr = SelectObject(memDC_, GetStockObject(NULL_BRUSH));

        Rectangle(memDC_, board->x, board->y,
            board->x + board->width, board->y + board->height);
        for (int i = 1; i < 8; ++i) {
            int x = board->x + i * board->width / 8;
            int y = board->y + i * board->height / 8;
            MoveToEx(memDC_, x, board->y, nullptr);
            LineTo(memDC_, x, board->y + board->height);
            MoveToEx(memDC_, board->x, y, nullptr);
            LineTo(memDC_, board->x + board->width, y);
        }
        SelectObject(memDC_, oldBr);
        SelectObject(memDC_, old);
        DeleteObject(pen);

        if (move.size() >= 4)
            drawArrow(memDC_, *board, move, whiteAtBottom);
    }

    // 2) une seule copie vers l'écran : plus d'état "effacé" visible
    HDC dc = GetDC(hwnd_);
    BitBlt(dc, 0, 0, w_, h_, memDC_, 0, 0, SRCCOPY);
    ReleaseDC(hwnd_, dc);
}

void Overlay::pump() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}