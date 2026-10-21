#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;


class FrameCapture
{

public:
	FrameCapture();
	~FrameCapture();


	bool Initialize(HWND hwnd);
	void CaptureFrame();


};
