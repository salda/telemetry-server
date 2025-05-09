#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

// Event data structure for storing telemetry path data
struct EventData {
    std::vector<double> values;
    uint64_t timestamp;
};

// Interface for telemetry data storage
class ITelemetryStorage {
public:
    virtual ~ITelemetryStorage() = default;
    
    // Saves telemetry event data
    virtual bool saveEvent(const std::string& eventName, 
                          const std::vector<double>& values, 
                          uint64_t timestamp) = 0;

    // Retrieves events filtered by optional time range
    virtual std::vector<EventData> getFilteredEvents(
        const std::string& eventName, 
        std::optional<uint64_t> startTimestamp = std::nullopt, 
        std::optional<uint64_t> endTimestamp = std::nullopt) = 0;
};

// Interface for telemetry processing
class ITelemetryProcessor {
public:
    virtual ~ITelemetryProcessor() = default;
    
    // Processes and saves a new telemetry event
    virtual bool saveEvent(const std::string& eventName, 
                          const std::vector<double>& values, 
                          uint64_t timestamp) = 0;

    // Calculates mean path length with optional time range filtering
    virtual double calculateMeanLength(
        const std::string& eventName, 
        std::optional<uint64_t> startTimestamp = std::nullopt, 
        std::optional<uint64_t> endTimestamp = std::nullopt) = 0;
};

// Configuration for the HTTP server
struct ServerConfig {
    std::string address;
    int port;
    int threadCount;
};

// Interface for HTTP server
class IHttpServer {
public:
    virtual ~IHttpServer() = default;
    
    // Starts the HTTP server
    virtual bool run() = 0;
    
    // Stops the server
    virtual void stop() = 0;
};
