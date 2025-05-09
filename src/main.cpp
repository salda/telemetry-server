#include <iostream>
#include <thread>
#include <algorithm>
#include "telemetry/interfaces.h"
#include "telemetry/telemetry_storage.h"
#include "telemetry/telemetry_processor.h"
#include "telemetry/http_server.h"

int main(int argc, char* argv[]) {
    try {
        // Check command line arguments
        if (argc != 3) {
            std::cerr << "Usage: telemetry-server <address> <port>\n"
                      << "Example: telemetry-server 0.0.0.0 8080\n";
            return EXIT_FAILURE;
        }

        // Parse command line arguments
        auto address = std::string(argv[1]);
        auto portStr = std::string(argv[2]);
        auto port = static_cast<int>(std::stoi(portStr));

        // Get optimal thread count for the system
        auto threadCount = std::max<int>(1, std::thread::hardware_concurrency());

        // Configure the server
        ServerConfig config{address, port, threadCount};

        // Initialize server components with stack allocation
        TelemetryStorage storage;
        TelemetryProcessor processor(storage);
        TelemetryHttpServer server(config, processor);
        
        // Run the server (this blocks until the server stops)
        if (!server.run()) {
            std::cerr << "Failed to start server!" << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
