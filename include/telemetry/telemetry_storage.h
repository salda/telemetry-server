#pragma once

#include <string>
#include <vector>
#include <map>
#include <shared_mutex>
#include "interfaces.h"

// Thread-safe storage for telemetry events
class TelemetryStorage : public ITelemetryStorage {
public:
    TelemetryStorage() = default;
    ~TelemetryStorage() override = default;
    
    // Prevent copying or moving
    TelemetryStorage(const TelemetryStorage&) = delete;
    TelemetryStorage& operator=(const TelemetryStorage&) = delete;
    TelemetryStorage(TelemetryStorage&&) = delete;
    TelemetryStorage& operator=(TelemetryStorage&&) = delete;
    
    // Implements ITelemetryStorage
    bool saveEvent(const std::string& eventName, 
                  const std::vector<double>& values, 
                  uint64_t timestamp) override;

    std::vector<EventData> getFilteredEvents(
        const std::string& eventName, 
        std::optional<uint64_t> startTimestamp = std::nullopt, 
        std::optional<uint64_t> endTimestamp = std::nullopt) override;

private:
    std::map<std::string, std::vector<EventData>> events_;
    std::shared_mutex mutex_; // C++17 shared mutex for reader-writer lock
};
