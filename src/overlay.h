#pragma once
#include <windows.h>
#include <opencv2/core.hpp>
#include <optional>
#include <string>

class Overlay {
public:
    Overlay();
    ~Overlay();
    void drawGrid(const std::optional<cv::Rect>& board,
        const std::string& move = "", bool whiteAtBottom = true);
    void pump();
private:
    HWND hwnd_ = nullptr;
    int w_ = 0, h_ = 0;
    HDC memDC_ = nullptr;
    HBITMAP bmp_ = nullptr, oldBmp_ = nullptr;

    std::optional<cv::Rect> lastBoard_;
    std::string lastMove_;
    bool lastBottom_ = true, drawnOnce_ = false;
    ULONGLONG lastDraw_ = 0;
};