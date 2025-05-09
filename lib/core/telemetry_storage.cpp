#include "telemetry/telemetry_storage.h"
#include <algorithm>
#include <mutex>       // For std::unique_lock
#include <shared_mutex> // For std::shared_mutex
#include <ranges>

bool TelemetryStorage::saveEvent(const std::string& eventName, 
                                const std::vector<double>& values, 
                                uint64_t timestamp) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    events_[eventName].push_back(EventData{values, timestamp});
    return true;
}

std::vector<EventData> TelemetryStorage::getFilteredEvents(
    const std::string& eventName, 
    std::optional<uint64_t> startTimestamp, 
    std::optional<uint64_t> endTimestamp) {
    
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = events_.find(eventName);
    if (it == events_.end()) {
        return {};
    }

    const auto& eventList = it->second;
    
    // Return early if no events or no filtering needed
    if (eventList.empty()) {
        return {};
    }
    
    if (!startTimestamp && !endTimestamp) {
        return eventList;
    }
    
    // Filter events by timestamp range
    auto filteredView = eventList | std::views::filter([&](const EventData& data) {
        const bool afterStart = !startTimestamp || data.timestamp >= *startTimestamp;
        const bool beforeEnd = !endTimestamp || data.timestamp <= *endTimestamp;
        return afterStart && beforeEnd;
    });
    
    return std::vector<EventData>(filteredView.begin(), filteredView.end());
}
