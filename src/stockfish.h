#pragma once

#include <windows.h>
#include <string>

class Stockfish {

public:

    Stockfish() = default;
    ~Stockfish();

    bool        start(const std::string& exePath);
    bool        alive() const;

    std::string bestMove(const std::string& fen, int moveTimeMs = 300); 

private:

    void        send(const std::string& cmd);

    bool        readLine(std::string& line);
    bool        waitFor(const std::string& token);


    HANDLE inWrite_     = nullptr;
    HANDLE outRead_     = nullptr;

    PROCESS_INFORMATION pi_{};

    std::string     buffer_;
};