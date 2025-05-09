#pragma once

#include <string>
#include <vector>
#include <optional>
#include "interfaces.h"

class TelemetryProcessor : public ITelemetryProcessor {
public:
    explicit TelemetryProcessor(ITelemetryStorage& storage);
    ~TelemetryProcessor() override = default;

    // Prevent copying or moving
    TelemetryProcessor(const TelemetryProcessor&) = delete;
    TelemetryProcessor& operator=(const TelemetryProcessor&) = delete;
    TelemetryProcessor(TelemetryProcessor&&) = delete;
    TelemetryProcessor& operator=(TelemetryProcessor&&) = delete;

    // Implementation of ITelemetryProcessor
    bool saveEvent(const std::string& eventName, 
                  const std::vector<double>& values, 
                  uint64_t timestamp) override;

    double calculateMeanLength(
        const std::string& eventName, 
        std::optional<uint64_t> startTimestamp = std::nullopt, 
        std::optional<uint64_t> endTimestamp = std::nullopt) override;

private:
    ITelemetryStorage& storage_;
};
