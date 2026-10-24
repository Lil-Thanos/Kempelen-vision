#include "frameCapture.h"
#include "overlay.h"
#include "detector.h"
#include "asyncEngine.h"
#include "fen.h"
#include <chrono>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <string>

// doublons de asyncEngine.cpp mais ça fonctionne on touche pas ;) ONLY_MY_TURN plausible

// true : la flèche n'apparaît que quand c'est a notre tour (le camp en bas de l'écran)
static const bool ONLY_MY_TURN = true;

// verifie que la position est possible
static bool plausible(const char g[8][8], std::string* why = nullptr)
{
    int K = 0, k = 0, P = 0, p = 0, w = 0, b = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            char x = g[r][c];
            if (x == '.') continue;
            if (std::isupper((unsigned char)x)) ++w; else ++b;
            if (x == 'K') ++K;
            if (x == 'k') ++k;
            if (x == 'P') ++P;
            if (x == 'p') ++p;
        }
    if (K != 1 || k != 1) {
        if (why) *why = "rois : " + std::to_string(K) + " blanc / " + std::to_string(k) + " noir";
        return false;
    }
    if (P > 8 || p > 8 || w > 16 || b > 16) {
        if (why) *why = "trop de pieces";
        return false;
    }
    return true;
}

// qui joue 'w', 'b', ou 0 si on ne sait pas.
static char moverColor(const char a[8][8], const char b[8][8])
{
    int changed = 0, w = 0, bl = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            if (a[r][c] == b[r][c]) continue;
            ++changed;
            char p = b[r][c];
            if (p == '.') continue;
            if (std::isupper((unsigned char)p)) ++w; else ++bl;
        }
    if (changed == 0 || changed > 4) return 0;
    if (w && !bl) return 'w';
    if (bl && !w) return 'b';
    return 0;
}

// nom de la case
static std::string squareName(int row, int col, bool whiteAtBottom)
{
    int file = whiteAtBottom ? col : 7 - col;
    int rank = whiteAtBottom ? 7 - row : row;
    return std::string{ char('a' + file), char('1' + rank) };
}

// logs
static std::string describeMove(const char a[8][8], const char b[8][8], bool wb)
{
    int fr = -1, fc = -1, tr = -1, tc = -1, nFrom = 0, nTo = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            if (a[r][c] == b[r][c]) continue;
            if (b[r][c] == '.') { ++nFrom; fr = r; fc = c; }
            else { ++nTo;   tr = r; tc = c; }
        }
    if (nFrom == 1 && nTo == 1) {
        std::string s = squareName(fr, fc, wb) + squareName(tr, tc, wb);
        if (std::tolower((unsigned char)a[fr][fc]) == 'p' &&
            std::tolower((unsigned char)b[tr][tc]) != 'p')
            s += (char)std::tolower((unsigned char)b[tr][tc]);        
        return s;
    }
    if (nFrom == 2 && nTo == 2) return "roque";
    if (nFrom == 2 && nTo == 1) return "prise en passant";
    return "coup non identifie (" + std::to_string(nFrom) + " cases videes, "
        + std::to_string(nTo) + " cases changees)";
}

// deux rectangles quasi identiques ?
static bool closeRect(const cv::Rect& a, const cv::Rect& b, int tol = 6)
{
    return std::abs(a.x - b.x) <= tol && std::abs(a.y - b.y) <= tol &&
        std::abs(a.width - b.width) <= tol && std::abs(a.height - b.height) <= tol;
}


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

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION,
        &device, nullptr, &context);
    if (FAILED(hr)) { std::cerr << "D3D11CreateDevice failed\n"; return; }

    ComPtr<IDXGIDevice> dxgiDevice;
    device.As(&dxgiDevice);
    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);
    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) { std::cerr << "No output found\n"; return; }

    ComPtr<IDXGIOutput1> output1;
    output.As(&output1);
    ComPtr<IDXGIOutputDuplication> duplication;
    hr = output1->DuplicateOutput(device.Get(), &duplication);
    if (FAILED(hr)) { std::cerr << "DuplicateOutput failed: " << std::hex << hr << '\n'; return; }

    SetProcessDPIAware();
    Overlay overlay;

    // initialisation de l'ia et de stockfish
    PieceDetector detector;
    if (!detector.load("engine/best.onnx")) { std::cerr << "best.onnx introuvable\n"; return; }
    AsyncEngine engine;
    if (!engine.start("engine/stockfish.exe")) { std::cerr << "Stockfish introuvable\n"; return; }

    ComPtr<ID3D11Texture2D> staging;
    std::optional<cv::Rect> board;
    unsigned long long count = 0;
    int misses = 0;

    char candidate[8][8] = {}, accepted[8][8] = {};
    int  stableCount = 0;
    bool hasAccepted = false, whiteToMove = true, whiteAtBottom = true;
    bool f8Prev = false, f9Prev = false;
    std::string currentFen, shownMove, lastLogFen, lastReject;
    cv::Mat lastBgr;
    auto lastInfer = std::chrono::steady_clock::now();

    std::cout << "Echap : quitter | F8 : inverser le trait | F9 : inverser l'orientation\n";

    // affiche une raison de refus une seule fois (pas 4 fois par seconde)
    auto reject = [&](const std::string& why) {
        if (why != lastReject) {
            std::cout << "[LECTURE] ignoree : " << why << "\n";
            lastReject = why;
        }
        };

    // demande l'analyse de la position acceptée
    auto requestAnalysis = [&]() {
        currentFen = gridToFen(accepted, whiteAtBottom, whiteToMove);
        shownMove.clear();
        lastLogFen.clear();
        bool myTurn = (whiteToMove == whiteAtBottom);   // camp du bas du plateau  = nous
        std::cout << "[TRAIT] " << (whiteToMove ? "Blancs" : "Noirs") << " -> "
            << (myTurn ? "a moi de jouer" : "tour de l'adversaire") << "\n"
            << "[FEN]   " << currentFen << "\n";
        if (myTurn || !ONLY_MY_TURN) engine.request(currentFen);
        };

    auto analyze = [&](const cv::Mat& frame)
        {
            if (!board || frame.empty()) {                 
                if (hasAccepted) std::cout << "[PARTIE] plateau perdu, reinitialisation\n";
                hasAccepted = false; stableCount = 0;
                shownMove.clear(); currentFen.clear(); lastLogFen.clear(); lastReject.clear();
                return;
            }
            auto now = std::chrono::steady_clock::now();
            if (now - lastInfer < std::chrono::milliseconds(250)) return;   // 4 analyses/s max
            lastInfer = now;

            cv::Rect r = *board & cv::Rect(0, 0, frame.cols, frame.rows);
            if (r.width < 64 || r.height < 64) return;
            cv::Mat crop = frame(r).clone();

            char grid[8][8];
            PieceDetector::toGrid(detector.detect(crop), crop.size(), grid);

            // la même lecture 2 fois de suite avant d'y croire
            if (std::memcmp(grid, candidate, 64) == 0) ++stableCount;
            else { std::memcpy(candidate, grid, 64); stableCount = 1; }

            if (stableCount < 2) { reject("lecture instable (les pieces bougent ?)"); return; }
            std::string why;
            if (!plausible(candidate, &why)) { reject("position invraisemblable (" + why + ")"); return; }
            lastReject.clear();

            if (hasAccepted && std::memcmp(candidate, accepted, 64) == 0) return;   // rien de neuf

            if (!hasAccepted) {
                // l'orientation est fixée UNE fois (au début), puis F9 pour la corriger
                whiteAtBottom = PieceDetector::whiteAtBottom(candidate);
                std::cout << "[PARTIE] position detectee, blancs en "
                    << (whiteAtBottom ? "bas" : "haut") << " (je joue les "
                    << (whiteAtBottom ? "Blancs" : "Noirs") << ")\n";
            }
            else {
                char m = moverColor(accepted, candidate);
                if (m == 'w') whiteToMove = false;
                else if (m == 'b') whiteToMove = true;
                bool mine = ((m == 'w') == whiteAtBottom);
                std::cout << "[COUP] " << (m == 'w' ? "Blancs" : m == 'b' ? "Noirs" : "?")
                    << (m ? (mine ? " (moi)" : " (adversaire)") : "")
                    << " : " << describeMove(accepted, candidate, whiteAtBottom) << "\n";
            }
            std::memcpy(accepted, candidate, 64);
            hasAccepted = true;
            requestAnalysis();
        };

    auto render = [&]()
        {
            std::string forFen;
            std::string mv = engine.latest(forFen);
            if (!currentFen.empty() && forFen == currentFen && forFen != lastLogFen) {
                lastLogFen = forFen;
                shownMove = mv;
                if (mv.empty()) std::cout << "[SF] aucun coup renvoye\n";
                else            std::cout << "[SF] meilleur coup : " << mv << "\n";
            }
            overlay.drawGrid(board, shownMove, whiteAtBottom);   // ne redessine que si ça a changé
        };

    while (true)
    {
        overlay.pump();
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
        if (f8 && !f8Prev && hasAccepted) {
            whiteToMove = !whiteToMove;
            std::cout << "[TOUCHE] trait inverse\n";
            requestAnalysis();
        }
        f8Prev = f8;

        bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        if (f9 && !f9Prev && hasAccepted) {
            whiteAtBottom = !whiteAtBottom;
            std::cout << "[TOUCHE] orientation inversee\n";
            requestAnalysis();
        }
        f9Prev = f9;

        DXGI_OUTDUPL_FRAME_INFO info{};
        ComPtr<IDXGIResource> resource;
        hr = duplication->AcquireNextFrame(100, &info, &resource);

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            analyze(lastBgr);
            render();
            continue;
        }
        if (FAILED(hr)) {
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

            if (!staging) {
                D3D11_TEXTURE2D_DESC sd = bureau;
                sd.Usage = D3D11_USAGE_STAGING;
                sd.BindFlags = 0;
                sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                sd.MiscFlags = 0;
                device->CreateTexture2D(&sd, nullptr, &staging);
            }
            if (staging) {
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

        // --- localisation du plateau (avec tolérance de quelques pixels) ---
        if (!board || ++count % 10 == 0)
        {
            auto b = detectBoard(bgr);
            if (b) {
                if (!board) std::cout << "[PLATEAU] trouve : " << b->width << "x" << b->height
                    << " en (" << b->x << "," << b->y << ")\n";
                if (!board || !closeRect(*board, *b)) board = b;
                misses = 0;
            }
            else if (++misses >= 3) board.reset();
        }

        lastBgr = bgr;
        analyze(bgr);
        render();
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