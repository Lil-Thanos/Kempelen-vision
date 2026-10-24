#include "ia-chess.h"

#include "src/stockfish.h"
#include "src/detector.h"

#include "src/fen.h"

#include <opencv2/core/utils/logger.hpp>

using namespace std;

void test_stockfish()
{ 
    Stockfish sf;
    sf.start("engine/stockfish.exe");
    std::cout << sf.bestMove("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w - - 0 1") << "\n";

    // Test 2 : ta fonction gridToFen (position de départ, blancs en bas)
    //char g[8][8] = {
    //    { 'r','n','b','q','k','b','n','r' },
    //    { 'p','p','p','p','p','p','p','p' },
    //    { '.','.','.','.','.','.','.','.' },
    //    { '.','.','.','.','.','.','.','.' },
    //    { '.','.','.','.','.','.','.','.' },
    //    { '.','.','.','.','.','.','.','.' },
    //    { 'P','P','P','P','P','P','P','P' },
    //    { 'R','N','B','Q','K','B','N','R' },
    //};
    //std::string fen = gridToFen(g, true, true);
    //std::cout << fen << "\n" << sf.bestMove(fen) << "\n";

}

int test_detector() {
    PieceDetector det;
    if (!det.load("model/best.onnx")) return 1;

    cv::Mat img = cv::imread("file_test/image.png");
    auto t0 = std::chrono::steady_clock::now();
    auto dets = det.detect(img);
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

    char grid[8][8];
    PieceDetector::toGrid(dets, img.size(), grid);
    bool wb = PieceDetector::whiteAtBottom(grid);
    std::string fen = gridToFen(grid, wb, true);

    std::cout << dets.size() << " pièces en " << ms << " ms\n" << fen << "\n";
}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	FrameCapture frameCapture;

	cout << "Welcome to Kempelen" << endl;

	frameCapture.captureFrame();
    //int test = 1;
    
    //test_stockfish();

	//printf("test_stockfish returned %d\n", test);

	return 0;
}
