#include "telemetry/telemetry_processor.h"
#include <numeric>
#include <algorithm>
#include <execution>

TelemetryProcessor::TelemetryProcessor(ITelemetryStorage& storage) 
    : storage_(storage) {
}

bool TelemetryProcessor::saveEvent(const std::string& eventName, 
                                  const std::vector<double>& values, 
                                  uint64_t timestamp) {
    // Validate path length (must be exactly 10 elements)
    if (values.size() != 10) {
        return false;
    }
    
    // Save to storage
    return storage_.saveEvent(eventName, values, timestamp);
}

double TelemetryProcessor::calculateMeanLength(
    const std::string& eventName, 
    std::optional<uint64_t> startTimestamp, 
    std::optional<uint64_t> endTimestamp) {
    
    // Retrieve filtered events
    auto filteredEvents = storage_.getFilteredEvents(
        eventName, startTimestamp, endTimestamp);
    
    if (filteredEvents.empty()) {
        return 0.0;
    }
    
    // Compute the total sum over all events by summing each event's sum directly.
    double totalSum = 0.0;
    for (const auto& event : filteredEvents) {
        totalSum += std::reduce(std::execution::par, 
                               event.values.begin(), 
                               event.values.end());
    }
        
    return totalSum / filteredEvents.size();
}
