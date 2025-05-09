#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <trompeloeil.hpp>
#include "catch2/trompeloeil.hpp"
#include "telemetry/interfaces.h"
#include "telemetry/telemetry_processor.h"
#include <optional>
#include <vector>
#include <string>

// Mock implementation of ITelemetryStorage using Trompeloeil
class MockTelemetryStorage : public ITelemetryStorage {
public:
    MAKE_MOCK3(saveEvent, bool(const std::string&, const std::vector<double>&, uint64_t));
    MAKE_MOCK3(getFilteredEvents, std::vector<EventData>(const std::string&, 
                                                        std::optional<uint64_t>, 
                                                        std::optional<uint64_t>));
};

// Helper to create a test event path of 10 values
std::vector<double> createTestPath(double value = 1.0) {
    return std::vector<double>(10, value);
}

SCENARIO("Telemetry processor handles event data", "[telemetry]") {
    GIVEN("A telemetry processor with a mock storage backend") {
        // Create the mock storage
        MockTelemetryStorage mockStorage;
        
        // Create the processor with the mock storage
        TelemetryProcessor processor(mockStorage);
        
        WHEN("An event with 10 values is saved") {
            auto eventName = "test_event";
            auto values = createTestPath(1.5);
            uint64_t timestamp = 1617235200; // 2021-04-01 00:00:00 UTC
            
            // Set expectations on the mock
            REQUIRE_CALL(mockStorage, saveEvent(eventName, values, timestamp))
                .TIMES(1)
                .RETURN(true);
            
            // Execute the function under test
            auto result = processor.saveEvent(eventName, values, timestamp);
            
            THEN("The save operation succeeds") {
                REQUIRE(result == true);
            }
        }
        
        WHEN("An event with fewer than 10 values is saved") {
            auto eventName = "test_event";
            std::vector<double> values(5, 1.0); // Only 5 values
            uint64_t timestamp = 1617235200;
            
            // No expectations on mock - should not be called
            
            auto result = processor.saveEvent(eventName, values, timestamp);
            
            THEN("The save operation fails without calling storage") {
                REQUIRE(result == false);
            }
        }
    }
}

SCENARIO("Calculating mean path length", "[telemetry]") {
    GIVEN("A telemetry processor with mocked events in storage") {
        // Create the mock storage
        MockTelemetryStorage mockStorage;
        
        // Create the processor with the mock storage
        TelemetryProcessor processor(mockStorage);
        
        // Mock event data
        auto eventName = "user_flow";
        
        WHEN("Mean length is calculated without time filters") {
            // Define test data
            std::vector<EventData> testEvents = {
                {createTestPath(1.0), 1617235200}, // Sum = 10.0
                {createTestPath(2.0), 1617321600}, // Sum = 20.0
                {createTestPath(3.0), 1617408000}  // Sum = 30.0
            };
            
            // Set expectations on the mock
            REQUIRE_CALL(mockStorage, getFilteredEvents(eventName, std::optional<uint64_t>(), std::optional<uint64_t>()))
                .TIMES(1)
                .RETURN(testEvents);
            
            // Calculate mean
            auto mean = processor.calculateMeanLength(eventName);
            
            THEN("The mean is correctly calculated from all events") {
                // Mean = (10 + 20 + 30) / 3 = 20.0
                REQUIRE_THAT(mean, Catch::Matchers::WithinRel(20.0, 0.0001)); // Within 0.01% tolerance
            }
        }
        
        WHEN("Mean length is calculated with a start timestamp filter") {
            // Define test data for this specific case
            std::vector<EventData> filteredEvents = {
                {createTestPath(2.0), 1617321600}, // Sum = 20.0
                {createTestPath(3.0), 1617408000}  // Sum = 30.0
            };
            
            // Only include events from 2021-04-02 onward
            std::optional<uint64_t> startTimestamp = 1617321600;
            
            // Set expectations on the mock
            REQUIRE_CALL(mockStorage, getFilteredEvents(eventName, startTimestamp, std::optional<uint64_t>()))
                .TIMES(1)
                .RETURN(filteredEvents);
            
            // Calculate mean
            auto mean = processor.calculateMeanLength(eventName, startTimestamp);
            
            THEN("Only events after the start time are included") {
                // Mean = (20 + 30) / 2 = 25.0
                REQUIRE_THAT(mean, Catch::Matchers::WithinRel(25.0, 0.0001)); // Within 0.01% tolerance
            }
        }
        
        WHEN("Mean length is calculated with an end timestamp filter") {
            // Define test data for this specific case
            std::vector<EventData> filteredEvents = {
                {createTestPath(1.0), 1617235200}, // Sum = 10.0
                {createTestPath(2.0), 1617321600}  // Sum = 20.0
            };
            
            // Only include events up to 2021-04-02
            std::optional<uint64_t> endTimestamp = 1617321600;
            
            // Set expectations on the mock
            REQUIRE_CALL(mockStorage, getFilteredEvents(eventName, std::optional<uint64_t>(), endTimestamp))
                .TIMES(1)
                .RETURN(filteredEvents);
            
            // Calculate mean
            auto mean = processor.calculateMeanLength(eventName, std::optional<uint64_t>(), endTimestamp);
            
            THEN("Only events before the end time are included") {
                // Mean = (10 + 20) / 2 = 15.0
                REQUIRE_THAT(mean, Catch::Matchers::WithinRel(15.0, 0.0001)); // Within 0.01% tolerance
            }
        }
        
        WHEN("Mean length is calculated for a non-existent event") {
            // Set expectations on the mock - empty result
            REQUIRE_CALL(mockStorage, getFilteredEvents(eventName, std::optional<uint64_t>(), std::optional<uint64_t>()))
                .TIMES(1)
                .RETURN(std::vector<EventData>());
            
            // Calculate mean
            auto mean = processor.calculateMeanLength(eventName);
            
            THEN("The result is zero") {
                REQUIRE_THAT(mean, Catch::Matchers::WithinAbs(0.0, 0.000001)); // Exact zero within epsilon
            }
        }
    }
}
