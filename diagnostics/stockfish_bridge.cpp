#include <windows.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include "stockfish_bridge.h"

namespace {
     struct HandleGuard {
          HANDLE handle = nullptr;

          HandleGuard() = default;
          explicit HandleGuard(HANDLE handle) : handle(handle) {}
          ~HandleGuard() noexcept {
               reset();
          }

          HandleGuard(const HandleGuard&) = delete;
          HandleGuard& operator=(const HandleGuard&) = delete;

          HANDLE* put() noexcept {
               reset();
               return &handle;
          }

          HANDLE get() const noexcept {
               return handle;
          }

          HANDLE release() noexcept {
               HANDLE released = handle;
               handle = nullptr;
               return released;
          }

          void reset(HANDLE newHandle = nullptr) noexcept {
               if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
                    CloseHandle(handle);
               }
               handle = newHandle;
          }
     };
}

StockfishProcess::~StockfishProcess() noexcept {
     close();
}

StockfishProcess::StockfishProcess(StockfishProcess&& other) noexcept
     : processInfo(other.processInfo),
       stdinWrite(other.stdinWrite),
       stdoutRead(other.stdoutRead),
       quitRequested(other.quitRequested) {
     other.processInfo = {};
     other.stdinWrite = nullptr;
     other.stdoutRead = nullptr;
     other.quitRequested = false;
}

StockfishProcess& StockfishProcess::operator=(StockfishProcess&& other) noexcept {
     if (this != &other) {
          close();
          processInfo = other.processInfo;
          stdinWrite = other.stdinWrite;
          stdoutRead = other.stdoutRead;
          quitRequested = other.quitRequested;

          other.processInfo = {};
          other.stdinWrite = nullptr;
          other.stdoutRead = nullptr;
          other.quitRequested = false;
     }

     return *this;
}

void StockfishProcess::requestQuit() noexcept {
     if (quitRequested) {
          return;
     }

     quitRequested = true;

     if (stdinWrite == nullptr || stdinWrite == INVALID_HANDLE_VALUE) {
          return;
     }

     const char quitCommand[] = "quit\n";
     DWORD bytesWritten = 0;
     WriteFile(
          stdinWrite,
          quitCommand,
          static_cast<DWORD>(sizeof(quitCommand) - 1),
          &bytesWritten,
          nullptr
     );
}

void StockfishProcess::close() noexcept {
     requestQuit();

     if (stdinWrite != nullptr && stdinWrite != INVALID_HANDLE_VALUE) {
          CloseHandle(stdinWrite);
          stdinWrite = nullptr;
     }

     if (stdoutRead != nullptr && stdoutRead != INVALID_HANDLE_VALUE) {
          CloseHandle(stdoutRead);
          stdoutRead = nullptr;
     }

     if (processInfo.hProcess != nullptr && processInfo.hProcess != INVALID_HANDLE_VALUE) {
          DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 1000);
          if (waitResult == WAIT_TIMEOUT) {
               TerminateProcess(processInfo.hProcess, 1);
               WaitForSingleObject(processInfo.hProcess, 1000);
          }
     }

     if (processInfo.hThread != nullptr && processInfo.hThread != INVALID_HANDLE_VALUE) {
          CloseHandle(processInfo.hThread);
     }

     if (processInfo.hProcess != nullptr && processInfo.hProcess != INVALID_HANDLE_VALUE) {
          CloseHandle(processInfo.hProcess);
     }

     processInfo = {};
     quitRequested = false;
}

StockfishProcess startStockfish(const std::wstring& stockfishPath) {
     SECURITY_ATTRIBUTES sa{};
     sa.nLength = sizeof(SECURITY_ATTRIBUTES);
     sa.bInheritHandle = TRUE;
     sa.lpSecurityDescriptor = nullptr;

     HandleGuard stdoutRead;
     HandleGuard stdoutWrite;
     HandleGuard stdinRead;
     HandleGuard stdinWrite;

     if (!CreatePipe(stdoutRead.put(), stdoutWrite.put(), &sa, 0)) {
          throw std::runtime_error("Failed to create stdout pipe");
     }

     if (!SetHandleInformation(stdoutRead.get(), HANDLE_FLAG_INHERIT, 0)) {
          throw std::runtime_error("Failed to configure stdout pipe");
     }

     if (!CreatePipe(stdinRead.put(), stdinWrite.put(), &sa, 0)) {
          throw std::runtime_error("Failed to create stdin pipe");
     }

     if (!SetHandleInformation(stdinWrite.get(), HANDLE_FLAG_INHERIT, 0)) {
          throw std::runtime_error("Failed to configure stdin pipe");
     }

     STARTUPINFOW startupInfo{};
     startupInfo.cb = sizeof(STARTUPINFOW);
     startupInfo.dwFlags = STARTF_USESTDHANDLES;
     startupInfo.hStdInput = stdinRead.get();
     startupInfo.hStdOutput = stdoutWrite.get();
     startupInfo.hStdError = stdoutWrite.get();

     PROCESS_INFORMATION processInfo{};

     std::wstring commandLine = L"\"" + stockfishPath + L"\"";

     BOOL success = CreateProcessW(
          nullptr,
          commandLine.data(),
          nullptr,
          nullptr,
          TRUE,
          CREATE_NO_WINDOW,
          nullptr,
          nullptr,
          &startupInfo,
          &processInfo
     );

     if (!success) {
          throw std::runtime_error("Failed to start Stockfish");
     }

     StockfishProcess sf;
     sf.processInfo = processInfo;
     sf.stdinWrite = stdinWrite.release();
     sf.stdoutRead = stdoutRead.release();

     return sf;
}

void sendCommand(StockfishProcess& sf, const std::string& command) {
     std::string line = command + "\n";

     DWORD bytesWritten = 0;
     BOOL success = WriteFile(
          sf.stdinWrite,
          line.c_str(),
          static_cast<DWORD>(line.size()),
          &bytesWritten,
          nullptr
     );

     if (!success || bytesWritten != line.size()) {
          throw std::runtime_error("Failed to write command to Stockfish");
     }
}

std::string readLine(StockfishProcess& sf) {
     std::string line;
     char ch;
     DWORD bytesRead = 0;

     while (true) {
          BOOL success = ReadFile(sf.stdoutRead, &ch, 1, &bytesRead, nullptr);

          if (!success) {
               throw std::runtime_error("Failed to read from Stockfish");
          }

          if (bytesRead == 0) {
               throw std::runtime_error("Stockfish closed its output pipe unexpectedly");
          }

          if (ch == '\r') {
               continue;
          }

          if (ch == '\n') {
               break;
          }

          line += ch;
     }

     return line;
}

std::string readUntil(StockfishProcess& sf, const std::string& marker) {
    std::string output;

    while (true) {
          std::string line = readLine(sf);
          output += line + "\n";

          if (line.find(marker) != std::string::npos) {
               break;
          }
    }

    return output;
}