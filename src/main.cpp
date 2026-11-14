#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "core/SystemMonitor.hpp"
#include "ui/ConsoleDisplay.hpp"
#include "ui/ConsoleInput.hpp"

#ifdef _WIN32
#include <windows.h>
#include <atomic>
#else
#include <csignal>
#endif

namespace {
#ifdef _WIN32
std::atomic<bool> exitRequested{false};
BOOL WINAPI handleConsoleControl(DWORD event) {
    if (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT) return FALSE;
    exitRequested.store(true);
    return TRUE; // Cleanup is performed on the main thread, not in this callback.
}
bool shouldExit() { return exitRequested.load(); }
class ShutdownHandler {
public:
    ShutdownHandler() : active(SetConsoleCtrlHandler(handleConsoleControl, TRUE) != 0) {}
    ~ShutdownHandler() { if (active) SetConsoleCtrlHandler(handleConsoleControl, FALSE); }
    bool installed() const { return active; }
private:
    bool active;
};
#else
volatile std::sig_atomic_t exitRequested = 0;
void handleInterrupt(int) { exitRequested = 1; }
bool shouldExit() { return exitRequested != 0; }
class ShutdownHandler {
public:
    ShutdownHandler() : previous(std::signal(SIGINT, handleInterrupt)) {}
    ~ShutdownHandler() { if (installed()) std::signal(SIGINT, previous); }
    bool installed() const { return previous != SIG_ERR; }
private:
    using Handler = void (*)(int);
    Handler previous;
};
#endif

bool readLine(ConsoleInput& input, std::string& line) {
    while (!shouldExit()) {
        const auto state = input.poll(line);
        if (state == InputState::Line) return !shouldExit();
        if (state != InputState::Pending) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    return false;
}

bool parseInteger(const std::string& line, int& value) {
    std::istringstream stream(line);
    if (!(stream >> value)) return false;
    stream >> std::ws;
    return stream.eof();
}

// Returns true to reopen the menu; false to close the application.
bool runDashboard(SystemMonitor& monitor, ConsoleInput& input) {
    monitor.stop(); // Collectors remain owned by the calling thread.
    std::cout << "\nStarting Live Dashboard...\n" << std::flush;
    while (!shouldExit()) {
        const auto snapshot = monitor.sampleOnce();
        ConsoleDisplay::renderDashboard(snapshot.cpuData, snapshot.ramData, snapshot.gpuData, snapshot.diskData);
        if (monitor.isLoggingEnabled())
            std::cout << " [ CSV Logging: ACTIVE -> system_metrics.csv ]\n" << std::flush;

        const auto nextSample = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(monitor.getIntervalMs());
        // Poll during the wait, so a long sampling interval cannot block exit.
        do {
            if (shouldExit()) return false;
            std::string line;
            const auto state = input.poll(line);
            if (state == InputState::EndOfInput || state == InputState::Interrupted) return false;
            if (state == InputState::Line) {
                std::istringstream stream(line);
                std::string command, extra;
                stream >> command;
                if (!(stream >> extra)) {
                    if (command == "q" || command == "Q") return true;
                    if (command == "x" || command == "X") return false;
                }
                if (!command.empty())
                    std::cout << "Use Q + Enter for menu or X + Enter to exit.\n" << std::flush;
            }
            const auto now = std::chrono::steady_clock::now();
            if (now >= nextSample) break;
            const auto remaining = nextSample - now;
            const auto pollInterval = std::chrono::steady_clock::duration(std::chrono::milliseconds(25));
            std::this_thread::sleep_for(remaining < pollInterval ? remaining : pollInterval);
        } while (true);
    }
    return false;
}
}

void displayMenu(int currentInterval, bool loggingActive) {
    std::cout << "\n=========================================\n";
    std::cout << "     SYSTEM PERFORMANCE MONITOR MENU     \n";
    std::cout << "=========================================\n";
    std::cout << " 1. Start Live Dashboard\n";
    std::cout << " 2. Set Sampling Interval (Current: " << currentInterval << " ms)\n";
    std::cout << " 3. Toggle CSV Logging (Status: " << (loggingActive ? "ENABLED" : "DISABLED") << ")\n";
    std::cout << " 4. Exit Application\n";
    std::cout << "=========================================\n";
    std::cout << " Select an option (1-4): " << std::flush;
}

int main() {
    ShutdownHandler shutdown;
    if (!shutdown.installed()) {
        std::cerr << "Could not install the console shutdown handler.\n";
        return 1;
    }
    SystemMonitor systemMonitor;
    ConsoleInput input;
    bool running = true;
    while (running && !shouldExit()) {
        displayMenu(systemMonitor.getIntervalMs(), systemMonitor.isLoggingEnabled());
        std::string line;
        if (!readLine(input, line)) break;
        int choice = 0;
        if (!parseInteger(line, choice)) {
            std::cout << "Invalid option!\n";
            continue;
        }
        switch (choice) {
            case 1:
                running = runDashboard(systemMonitor, input);
                break;
            case 2: {
                std::cout << "\nEnter new interval in milliseconds (min 100): " << std::flush;
                if (!readLine(input, line)) { running = false; break; }
                int interval = 0;
                if (parseInteger(line, interval) && interval >= 100) {
                    systemMonitor.setIntervalMs(interval);
                    std::cout << "Interval updated to " << interval << " ms.\n";
                } else {
                    std::cout << "Invalid interval!\n";
                }
                break;
            }
            case 3: {
                const bool active = !systemMonitor.isLoggingEnabled();
                systemMonitor.enableLogging(active);
                std::cout << "\nCSV Logging "
                          << (active ? "ENABLED (system_metrics.csv)" : "DISABLED") << ".\n";
                break;
            }
            case 4:
                running = false;
                break;
            default:
                std::cout << "Invalid option!\n";
                break;
        }
    }
    systemMonitor.stop();
    std::cout << "\nClosing application...\n";
    // SystemMonitor/CsvExport destructors close the export before returning.
    return 0;
}
