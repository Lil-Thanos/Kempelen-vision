#pragma once
#include "stockfish.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

class AsyncEngine {
public:
    bool start(const std::string& exe) {
        if (!sf_.start(exe)) return false;
        th_ = std::thread([this] { loop(); });
        return true;
    }
    ~AsyncEngine() {
        { std::lock_guard<std::mutex> lk(m_); stop_ = true; }
        cv_.notify_all();
        if (th_.joinable()) th_.join();
    }

    // demande l'analyse d'une position
    void request(const std::string& fen) {
        { std::lock_guard<std::mutex> lk(m_); pending_ = fen; }
        cv_.notify_all();
    }

    // dernier coup trouvé + la position pour laquelle il a été calculé
    std::string latest(std::string& forFen) {
        std::lock_guard<std::mutex> lk(m_);
        forFen = doneFen_;
        return move_;
    }

private:
    void loop() {
        std::unique_lock<std::mutex> lk(m_);
        while (!stop_) {
            cv_.wait(lk, [this] { return stop_ || !pending_.empty(); });
            if (stop_) break;
            std::string fen = std::move(pending_);
            pending_.clear();
            lk.unlock();
            std::string mv = sf_.bestMove(fen, 300);   // seul thread qui parle à stockfish
            lk.lock();
            doneFen_ = fen;
            move_ = mv;
        }
    }

    Stockfish sf_;              
    std::thread th_;
    std::mutex m_;
    std::condition_variable cv_;
    std::string pending_, doneFen_, move_;
    bool stop_ = false;
};