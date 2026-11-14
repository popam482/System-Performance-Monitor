#include "ui/ConsoleInput.hpp"
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#endif

InputState ConsoleInput::poll(std::string& line) {
    if (ended) return InputState::EndOfInput;
    while (true) {
        char character = 0;
        bool interactive = false;
        bool eof = false;
#ifdef _WIN32
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode = 0;
        interactive = GetConsoleMode(input, &mode) != 0;
        if (interactive) {
            if (!_kbhit()) return InputState::Pending;
            const int key = _getch();
            if (key == 0 || key == 0xE0) {
                _getch(); // Ignore extended keys (arrows, function keys).
                continue;
            }
            character = static_cast<char>(key);
        } else {
            // A synchronous read from an empty pipe would prevent Ctrl+C and
            // dashboard navigation. Check availability before reading.
            if (GetFileType(input) == FILE_TYPE_PIPE) {
                DWORD available = 0;
                if (!PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr)) eof = true;
                else if (available == 0) return InputState::Pending;
            }
            DWORD count = 0;
            if (!eof) eof = !ReadFile(input, &character, 1, &count, nullptr) || count == 0;
        }
#else
        interactive = isatty(STDIN_FILENO) != 0;
        pollfd input{STDIN_FILENO, POLLIN, 0};
        const int ready = ::poll(&input, 1, 0);
        if (ready < 0 && errno == EINTR) return InputState::Pending;
        if (ready == 0) return InputState::Pending;
        eof = ready < 0 || ::read(STDIN_FILENO, &character, 1) <= 0;
#endif
        if (eof || character == '\x1A' || character == '\x04') {
            ended = true;
            if (buffer.empty()) return InputState::EndOfInput;
            line = std::move(buffer);
            buffer.clear();
            return InputState::Line; // Preserve the final line without a newline.
        }
        if (character == '\x03') return InputState::Interrupted;
        if (character == '\n' || (interactive && character == '\r')) {
            if (interactive) std::cout << '\n';
            line = std::move(buffer);
            buffer.clear();
            return InputState::Line;
        }
        if (character == '\r') continue; // CRLF in redirected input.
#ifdef _WIN32
        if (interactive && (character == '\b' || character == '\x7F')) {
            if (!buffer.empty()) {
                buffer.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }
        if (interactive) std::cout << character << std::flush;
#endif
        buffer += character;
    }
}
