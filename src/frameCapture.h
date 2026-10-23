#pragma once

// detection de l'ecran et du plateau 

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <opencv2/opencv.hpp>
#include <optional>


#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;


class FrameCapture
{

public:
	FrameCapture();
	~FrameCapture();


	bool initialize(HWND hwnd);

	void captureFrame();

	void findBoard();


	std::optional<cv::Rect>  detectBoard(const cv::Mat& bgr);

	inline cv::Rect  squareRect(const cv::Rect& board, int row, int col);
	inline std::optional<cv::Point>  pointToSquare(const cv::Rect& board, cv::Point2f p);
};
