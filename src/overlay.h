#pragma once

#include <windows.h>
#include <opencv2/core.hpp>
#include <optional>

class Overlay {
public:
    Overlay();
    ~Overlay();
    void drawGrid(const std::optional<cv::Rect>& board);
    void pump(); 
private:
    HWND hwnd_ = nullptr;
    int w_ = 0, h_ = 0;
};