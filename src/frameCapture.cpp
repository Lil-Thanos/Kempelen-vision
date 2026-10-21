#include "frameCapture.h"


FrameCapture::FrameCapture()
{
	if (!initialize(nullptr))
	{
		std::cerr << "erreur d'initialisation de FrameCapture\n";
	}

    captureFrame();

}

FrameCapture::~FrameCapture()
{

}

bool FrameCapture::initialize(HWND hwnd)
{
	return true;
}

void FrameCapture::captureFrame()
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0, nullptr, 0,
        D3D11_SDK_VERSION,
        &device, nullptr, &context
    );

    if (FAILED(hr)) 
    {
        std::cerr << "D3D11CreateDevice failed\n";
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    device.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);

    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);

    if (FAILED(hr)) 
    {
        std::cerr << "No output found\n";
    }

    ComPtr<IDXGIOutput1> output1;
    output.As(&output1);

    ComPtr<IDXGIOutputDuplication> duplication;
    hr = output1->DuplicateOutput(device.Get(), &duplication);

    if (FAILED(hr)) 
    {
        std::cerr << "DuplicateOutput failed: " << std::hex << hr << '\n';
    }

    while (true) {
        DXGI_OUTDUPL_FRAME_INFO info{};
        ComPtr<IDXGIResource> resource;

        hr = duplication->AcquireNextFrame(100, &info, &resource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
            continue;

        if (FAILED(hr)) {
            std::cerr << "AcquireNextFrame failed: " << std::hex << hr << '\n';
            break;
        }

        ComPtr<ID3D11Texture2D> frame;
        hr = resource.As(&frame);

        if (SUCCEEDED(hr)) {
            D3D11_TEXTURE2D_DESC bueau{};
            frame->GetDesc(&bueau);

            static unsigned long long count = 0;

            if (++count % 120 == 0) 
            {
                std::cout << bueau.Width << " x " << bueau.Height << '\n';
            }
        }

        duplication->ReleaseFrame();

        if (FAILED(hr))
            break;
    }
}