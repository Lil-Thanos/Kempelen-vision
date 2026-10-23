#include "frameCapture.h"

#include "overlay.h"

FrameCapture::FrameCapture()
{
	if (!initialize(nullptr))
	{
		std::cerr << "erreur d'initialisation de FrameCapture\n";
	}

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
        return;
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
        return;
    }

    ComPtr<IDXGIOutput1> output1;
    output.As(&output1);

    ComPtr<IDXGIOutputDuplication> duplication;
    hr = output1->DuplicateOutput(device.Get(), &duplication);

    if (FAILED(hr))
    {
        std::cerr << "DuplicateOutput failed: " << std::hex << hr << '\n';
        return;
    }

    SetProcessDPIAware();
    Overlay overlay;

    ComPtr<ID3D11Texture2D> staging;
    std::optional<cv::Rect> board;      

    unsigned long long count = 0;
    int misses = 0;                    

    while (true)
    {
        overlay.pump();
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        DXGI_OUTDUPL_FRAME_INFO info{};
        ComPtr<IDXGIResource> resource;

        hr = duplication->AcquireNextFrame(100, &info, &resource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT)
        {
            overlay.drawGrid(board);
            continue;
        }

        if (FAILED(hr))
        {
            std::cerr << "AcquireNextFrame failed: " << std::hex << hr << '\n';
            break;
        }

        bool newImage = info.LastPresentTime.QuadPart != 0;
        bool copie = false;

        ComPtr<ID3D11Texture2D> frame;
        if (newImage && SUCCEEDED(resource.As(&frame)))
        {
            D3D11_TEXTURE2D_DESC bureau{};
            frame->GetDesc(&bureau);

            if (!staging)
            {
                D3D11_TEXTURE2D_DESC sd = bureau;
                sd.Usage = D3D11_USAGE_STAGING;
                sd.BindFlags = 0;
                sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                sd.MiscFlags = 0;
                device->CreateTexture2D(&sd, nullptr, &staging);
            }

            if (staging)
            {
                context->CopyResource(staging.Get(), frame.Get());
                copie = true;
            }
        }

        duplication->ReleaseFrame();
        if (!copie) continue;

        D3D11_MAPPED_SUBRESOURCE m{};
        if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &m)))
            continue;

        D3D11_TEXTURE2D_DESC d{};
        staging->GetDesc(&d);

        cv::Mat bgra(d.Height, d.Width, CV_8UC4, m.pData, m.RowPitch);
        cv::Mat bgr;
        cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
        context->Unmap(staging.Get(), 0);

        if (!board || ++count % 10 == 0)
        {
            auto b = detectBoard(bgr);
            if (b)
            {
                // empeche le clignotement 
                if (!board || std::abs(b->x - board->x) > 5 || std::abs(b->y - board->y) > 5 || std::abs(b->width - board->width) > 5)
                board = b;

                misses = 0;
            }
            else if (++misses >= 3)
            {
                board.reset();          
            }
        }

        overlay.drawGrid(board);
    }
}


void FrameCapture::findBoard()
{
    cv::Mat img = cv::imread("screenshot.png");
    auto board = detectBoard(img);

    if (!board) {
        // plan B sélection manuelle
        cv::Rect r = cv::selectROI("Selectionne le plateau", img);
        board = r;
    }

    cv::rectangle(img, *board, { 0, 255, 0 }, 2);
    for (int i = 1; i < 8; ++i) {
        int x = board->x + i * board->width / 8;
        int y = board->y + i * board->height / 8;
        cv::line(img, { x, board->y }, { x, board->y + board->height }, { 0, 255, 0 }, 1);
        cv::line(img, { board->x, y }, { board->x + board->width, y }, { 0, 255, 0 }, 1);
    }
    cv::imshow("Plateau", img);
    cv::waitKey(0);
}


// ===================== OUT CLASS ================== //

std::optional<cv::Rect>  FrameCapture::detectBoard(const cv::Mat& bgr) {
    cv::Mat gray, edges;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, { 5, 5 }, 0);
    cv::Canny(gray, edges, 50, 150);
    cv::dilate(edges, edges, cv::Mat(), { -1, -1 }, 1);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    std::optional<cv::Rect> best;
    double bestArea = 0;
    for (const auto& c : contours) {
        std::vector<cv::Point> approx;
        cv::approxPolyDP(c, approx, 0.02 * cv::arcLength(c, true), true);
        if (approx.size() != 4 || !cv::isContourConvex(approx)) continue;

        cv::Rect r = cv::boundingRect(approx);
        double ratio = (double)r.width / r.height;
        double area = r.area();
        if (ratio > 0.95 && ratio < 1.05 &&
            area > 0.2 * bgr.rows * bgr.cols * 0.25 && area > bestArea) {
            bestArea = area;
            best = r;
        }
    }
    return best;
}

inline cv::Rect  FrameCapture::squareRect(const cv::Rect& board, int row, int col) {

    int w = board.width / 8, h = board.height / 8;
    return { board.x + col * w, board.y + row * h, w, h };
}

inline std::optional<cv::Point>  FrameCapture::pointToSquare(const cv::Rect& board, cv::Point2f p) {
    if (!board.contains(cv::Point(p))) return std::nullopt;


    int col = (int)((p.x - board.x) / (board.width / 8.0));
    int row = (int)((p.y - board.y) / (board.height / 8.0));

    return cv::Point(col, row);
}