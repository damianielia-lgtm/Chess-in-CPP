#pragma once

#include <windows.h>
#include <string>

struct StockfishProcess {
    PROCESS_INFORMATION processInfo{};
    HANDLE stdinWrite = nullptr;  // you write commands here
    HANDLE stdoutRead = nullptr;  // you read Stockfish output here
    bool quitRequested = false;

    StockfishProcess() = default;
    ~StockfishProcess() noexcept { close(); };

    StockfishProcess(const StockfishProcess&) = delete;
    StockfishProcess& operator=(const StockfishProcess&) = delete;

    StockfishProcess(StockfishProcess&& other) noexcept;
    StockfishProcess& operator=(StockfishProcess&& other) noexcept;

    void requestQuit() noexcept;
    void close() noexcept;
};

StockfishProcess startStockfish(const std::wstring& stockfishPath);

void sendCommand(StockfishProcess& sf, const std::string& command);

std::string readLine(StockfishProcess& sf);

std::string readUntil(StockfishProcess& sf, const std::string& marker);
