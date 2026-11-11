#include <chrono>
#include <iostream>
#include <thread>

#include "core/SystemMonitor.hpp"
#include "ui/ConsoleDisplay.hpp"

// If you do NOT see BUILD_MARKER_2026_11_11 in the console at startup,
// you are still running an old executable / old sources.

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
    std::cout << "BUILD_MARKER_2026_11_11\n";
    std::cout << "System Performance Monitor starting...\n" << std::flush;

    SystemMonitor systemMonitor;

    bool running = true;
    while (running) {
        displayMenu(systemMonitor.getIntervalMs(), systemMonitor.isLoggingEnabled());

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1: {
                systemMonitor.stop();

                std::cout << "\n[DBG] entered dashboard case\n" << std::flush;
                std::cout << "Starting Live Dashboard (sync, no GPU)...\n" << std::flush;

                int frame = 0;
                while (true) {
                    ++frame;
                    std::cout << "\n[DBG] frame " << frame << " before sampleOnce\n" << std::flush;

                    SystemSnapshot snapshot = systemMonitor.sampleOnce();

                    std::cout << "[DBG] frame " << frame << " after sampleOnce\n" << std::flush;

                    ConsoleDisplay::renderDashboard(
                        snapshot.cpuData,
                        snapshot.ramData,
                        snapshot.gpuData,
                        snapshot.diskData
                    );

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(systemMonitor.getIntervalMs())
                    );
                }
                break;
            }
            case 2: {
                int newInterval = 0;
                std::cout << "\nEnter new interval in milliseconds (min 100): " << std::flush;
                if (std::cin >> newInterval && newInterval >= 100) {
                    systemMonitor.setIntervalMs(newInterval);
                    std::cout << "Interval updated to " << newInterval << " ms.\n";
                } else {
                    std::cout << "Invalid interval!\n";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }
                break;
            }
            case 3: {
                const bool currentState = systemMonitor.isLoggingEnabled();
                systemMonitor.enableLogging(!currentState);
                std::cout << "\nCSV Logging "
                          << (!currentState ? "ENABLED (system_metrics.csv)" : "DISABLED")
                          << ".\n";
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
    return 0;
}
