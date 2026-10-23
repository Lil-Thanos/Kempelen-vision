#pragma once
#include <string>

// grid[ligne][colonne], ligne 0 = haut de l'image.
// Pièces : "PNBRQK" blancs, "pnbrqk" noirs, '.' = case vide.
inline std::string gridToFen(const char grid[8][8], bool whiteAtBottom, bool whiteToMove)
{
    std::string fen;
    for (int r = 0; r < 8; ++r) {
        int empty = 0;
        for (int c = 0; c < 8; ++c) {
            // plateau retourné : on lit la grille à l'envers
            char p = whiteAtBottom ? grid[r][c] : grid[7 - r][7 - c];
            if (p == '.' || p == 0) { ++empty; continue; }
            if (empty) { fen += char('0' + empty); empty = 0; }
            fen += p;
        }
        if (empty) fen += char('0' + empty);
        if (r < 7) fen += '/';
    }
    fen += whiteToMove ? " w" : " b";
    fen += " - - 0 1";     // roques et en passant inconnus pour l'instant
    return fen;
}