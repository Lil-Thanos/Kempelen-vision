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
    return "coup non identifie (" + std::to_string(nFrom) + " cases vides, "
        + std::to_string(nTo) + " cases changees)";
}

// empêche double identification d'un plateau 2 cases proches
static bool closeRect(const cv::Rect& a, const cv::Rect& b, int tol = 6)
{
    return std::abs(a.x - b.x) <= tol && std::abs(a.y - b.y) <= tol &&
        std::abs(a.width - b.width) <= tol && std::abs(a.height - b.height) <= tol;
}