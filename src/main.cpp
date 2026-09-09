#include <thread>
#include <chrono>
#include <memory>
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/windows/WindowsGpuCollector.hpp"
#include "collectors/windows/WindowsDiskCollector.hpp"
#include "ui/ConsoleDisplay.hpp"
#include "core/SystemMonitor.hpp"
#include <iostream>


void displayMenu(int currentInterval){
    std::cout << "--------------------------\n" << std::endl;
    std::cout << "System Performance Monitor\n" << std::endl;
    std::cout << "--------------------------\n" << std::endl;
    std::cout << " 1. Start Live Dashboard\n";
    std::cout << " 2. Set Sampling Interval (Current: " << currentInterval << " ms)\n";
    std::cout << " 3. Exit Application\n";
    std::cout << "=========================================\n";
    std::cout << " Select an option (1-3): ";
}

int main() {

    SystemMonitor systemMonitor;
    systemMonitor.start();

    bool running = true;

    while(running) {
       displayMenu(systemMonitor.getIntervalMs());

       int choice;

       if((!std::cin >> choice)){
        std::cin.clear(); // clear the error flag
        std::cin.ignore(1000, '\n');
        continue;
       }

       switch(choice){
        case 1: {
                std::cout << "\nStarting Live Dashboard... Press Enter to return to menu.\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                bool inDashboard = true;
                while (inDashboard) {
                    SystemSnapshot snapshot = systemMonitor.getLatestSnapshot();
                    ConsoleDisplay::renderDashboard(snapshot.cpuData, snapshot.ramData, snapshot.gpuData, snapshot.diskData);
                    
                    std::cout << "\n [ Tip: Press Ctrl+C to return to menu ]\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(systemMonitor.getIntervalMs()));
                }
                break;
            }
            case 2: {
                int newInterval;
                std::cout << "\nEnter new interval in milliseconds (e.g. 500, 1000, 2000): ";
                if (std::cin >> newInterval && newInterval >= 100) {
                    systemMonitor.setIntervalMs(newInterval);
                    std::cout << "Interval updated to " << newInterval << " ms.\n";
                } else {
                    std::cout << "Invalid interval! Must be at least 100 ms.\n";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }
                break;
            }
            case 3:
                running = false;
                break;
            default:
                std::cout << "Invalid option! Try again.\n";
                break;
       }
    }

   

    return 0;
}