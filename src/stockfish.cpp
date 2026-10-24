#include "stockfish.h"

bool Stockfish::start(const std::string& exePath)
{
    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    HANDLE inRead = nullptr, outWrite = nullptr;

    CreatePipe(&inRead, &inWrite_, &sa, 0);
    SetHandleInformation(inWrite_, HANDLE_FLAG_INHERIT, 0);  
    CreatePipe(&outRead_, &outWrite, &sa, 0);
    SetHandleInformation(outRead_, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inRead;
    si.hStdOutput = outWrite;
    si.hStdError = outWrite;

    std::string cmd = "\"" + exePath + "\"";
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi_))
        return false;

    CloseHandle(inRead);   
    CloseHandle(outWrite);

    send("uci");
    if (!waitFor("uciok")) return false;
    send("isready");
    return waitFor("readyok");
}

Stockfish::~Stockfish()
{
    if (pi_.hProcess) {
        send("quit");
        WaitForSingleObject(pi_.hProcess, 1000);
        CloseHandle(pi_.hProcess);
        CloseHandle(pi_.hThread);
    }
    if (inWrite_) CloseHandle(inWrite_);
    if (outRead_) CloseHandle(outRead_);
}

void Stockfish::send(const std::string& cmd)
{
    std::string s = cmd + "\n";
    DWORD written = 0;
    WriteFile(inWrite_, s.c_str(), (DWORD)s.size(), &written, nullptr);
}

bool Stockfish::readLine(std::string& line)
{
    while (true) {
        size_t pos = buffer_.find('\n');
        if (pos != std::string::npos) {
            line = buffer_.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            buffer_.erase(0, pos + 1);
            return true;
        }
        char chunk[4096];
        DWORD n = 0;
        if (!ReadFile(outRead_, chunk, sizeof(chunk), &n, nullptr) || n == 0)
            return false;                       // fin du process
        buffer_.append(chunk, n);
    }
}

bool Stockfish::waitFor(const std::string& token)
{
    std::string line;
    while (readLine(line))
        if (line.rfind(token, 0) == 0) return true;   
    return false;
}

std::string Stockfish::bestMove(const std::string& fen, int moveTimeMs)
{
    send("position fen " + fen);
    send("go movetime " + std::to_string(moveTimeMs));

    std::string line;
    while (readLine(line)) {
        if (line.rfind("bestmove", 0) == 0) {
            size_t a = line.find(' ');
            if (a == std::string::npos) return "";
            size_t b = line.find(' ', a + 1);
            std::string mv = line.substr(a + 1, b == std::string::npos ? b : b - a - 1);
            return mv == "(none)" ? "" : mv;
        }
    }
    return "";
}

bool Stockfish::alive() const
{
    return pi_.hProcess && WaitForSingleObject(pi_.hProcess, 0) == WAIT_TIMEOUT;
}