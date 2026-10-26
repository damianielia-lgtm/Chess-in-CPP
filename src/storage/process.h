#pragma once

#include <windows.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "../errors.h"

class Process {
public:
    explicit Process(const std::string& executable) {
        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;

        HANDLE child_stdin_read = nullptr;
        HANDLE child_stdout_write = nullptr;
        HANDLE child_stderr = INVALID_HANDLE_VALUE;

        // The constructor must also close handles if startup fails.
        try {
            if (
                !CreatePipe(&child_stdin_read, &stdin_write_, &sa, 0) ||
                !CreatePipe(&stdout_read_, &child_stdout_write, &sa, 0)
            ) {
                throw ProcessError("Failed to create process pipes");
            }

            if (
                !SetHandleInformation(stdin_write_, HANDLE_FLAG_INHERIT, 0) ||
                !SetHandleInformation(stdout_read_, HANDLE_FLAG_INHERIT, 0)
            ) {
                throw ProcessError("Failed to configure process pipes");
            }

            // Discard stderr so diagnostics never enter the stdout stream.
            child_stderr = CreateFileA(
                "NUL",
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &sa,
                OPEN_EXISTING,
                0,
                nullptr
            );
            if (child_stderr == INVALID_HANDLE_VALUE) {
                throw ProcessError("Failed to open process stderr");
            }

            STARTUPINFOA startup{};
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdInput = child_stdin_read;
            startup.hStdOutput = child_stdout_write;
            startup.hStdError = child_stderr;

            PROCESS_INFORMATION info{};
            // CreateProcess needs a writable command line. Quotes allow spaces.
            std::string command = "\"" + executable + "\"";
            if (
                !CreateProcessA(
                    executable.c_str(),
                    command.data(),
                    nullptr,
                    nullptr,
                    TRUE,
                    CREATE_NO_WINDOW,
                    nullptr,
                    nullptr,
                    &startup,
                    &info
                )
            ) {
                throw ProcessError("Failed to start process: " + executable);
            }

            process_ = info.hProcess;
            CloseHandle(info.hThread);
        } catch (...) {
            if (child_stdin_read) { CloseHandle(child_stdin_read); }
            if (child_stdout_write) { CloseHandle(child_stdout_write); }
            if (child_stderr != INVALID_HANDLE_VALUE) { CloseHandle(child_stderr); }
            if (stdin_write_) { CloseHandle(stdin_write_); }
            if (stdout_read_) { CloseHandle(stdout_read_); }
            throw;
        }

        // The child now owns its inherited copies of these handles.
        CloseHandle(child_stdin_read);
        CloseHandle(child_stdout_write);
        CloseHandle(child_stderr);
    }

    ~Process() {
        // Closing stdin sends EOF. Closing a process handle does not kill it.
        CloseHandle(stdin_write_);
        CloseHandle(stdout_read_);
        CloseHandle(process_);
    }

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    void send_line(const std::string& line) {
        const std::string message = line + '\n';
        std::size_t sent = 0;
        while (sent < message.size()) {
            // Small chunks also avoid narrowing a large size to DWORD.
            const DWORD count = static_cast<DWORD>(
                (message.size() - sent > 4096) ? 4096 : message.size() - sent
            );
            DWORD written = 0;
            if (
                !WriteFile(
                    stdin_write_,
                    message.data() + sent,
                    count,
                    &written,
                    nullptr
                ) || written == 0
            ) {
                throw ProcessError("Failed to write to process");
            }
            sent += written;
        }
    }

    std::string read_line() {
        while (true) {
            const auto newline = read_buffer_.find('\n');
            if (newline != std::string::npos) {
                std::string line = read_buffer_.substr(0, newline);
                read_buffer_.erase(0, newline + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                return line;
            }

            char buffer[4096];
            DWORD received = 0;
            const BOOL success = ReadFile(
                stdout_read_,
                buffer,
                sizeof(buffer),
                &received,
                nullptr
            );
            if (!success && GetLastError() != ERROR_BROKEN_PIPE) {
                throw ProcessError("Failed to read from process");
            }
            if (!success || received == 0) {
                // Preserve a final line even if it has no trailing newline.
                if (!read_buffer_.empty()) {
                    std::string line;
                    line.swap(read_buffer_);
                    return line;
                }
                throw ProcessError("Process output closed");
            }
            read_buffer_.append(buffer, received);
        }
    }

    std::vector<std::string> read_until(const std::string& marker) {
        std::vector<std::string> output;

        while (true) {
            std::string line = read_line();
            output.push_back(line);

            if (line.find(marker) != std::string::npos) {
                break;
            }
        }

        return output;
    }

private:
    HANDLE process_ = nullptr;
    HANDLE stdin_write_ = nullptr;
    HANDLE stdout_read_ = nullptr;
    std::string read_buffer_;
};
