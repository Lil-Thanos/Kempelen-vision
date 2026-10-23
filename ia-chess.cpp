#include "ia-chess.h"

#include "src/stockfish.h"
#include "src/fen.h"

#include <opencv2/core/utils/logger.hpp>

using namespace std;

int test_stockfish()
{ 
    Stockfish sf;
    if (!sf.start("engine/stockfish.exe")) { std::cerr << "Stockfish introuvable\n"; return 1; }

    // Test 1 : position de départ
    std::cout << sf.bestMove("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") << "\n";

    // Test 2 : ta fonction gridToFen (position de départ, blancs en bas)
    char g[8][8] = {
        { 'r','n','b','q','k','b','n','r' },
        { 'p','p','p','p','p','p','p','p' },
        { '.','.','.','.','.','.','.','.' },
        { '.','.','.','.','.','.','.','.' },
        { '.','.','.','.','.','.','.','.' },
        { '.','.','.','.','.','.','.','.' },
        { 'P','P','P','P','P','P','P','P' },
        { 'R','N','B','Q','K','B','N','R' },
    };
    std::string fen = gridToFen(g, true, true);
    std::cout << fen << "\n" << sf.bestMove(fen) << "\n";

}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	FrameCapture frameCapture;

	cout << "Welcome to Kempelen" << endl;

	//frameCapture.captureFrame();
    int test = test_stockfish();

	printf("test_stockfish returned %d\n", test);

	return 0;
}
