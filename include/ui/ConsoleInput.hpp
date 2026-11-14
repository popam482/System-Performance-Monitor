#pragma once

#include <string>

enum class InputState { Pending, Line, EndOfInput, Interrupted };

// Non-blocking, line-oriented input, including redirected files and pipes.
class ConsoleInput {
public:
    InputState poll(std::string& line);

private:
    std::string buffer;
    bool ended = false;
};
