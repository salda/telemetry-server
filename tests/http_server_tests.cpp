#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <trompeloeil.hpp>
#include "catch2/trompeloeil.hpp"
#include "telemetry/interfaces.h"
#include "telemetry/http_server.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <chrono>
#include <optional>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <iostream>

using json = nlohmann::json;

// Add debug logger
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << __FUNCTION__ << ":" << __LINE__ << " - " << msg << std::endl

// Mock implementation of ITelemetryProcessor
class MockTelemetryProcessor : public ITelemetryProcessor {
public:
    MAKE_MOCK3(saveEvent, bool(const std::string&, const std::vector<double>&, uint64_t));
    MAKE_MOCK3(calculateMeanLength, double(const std::string&, std::optional<uint64_t>, std::optional<uint64_t>));
};

// Test Fixture class for HTTP server tests
class HttpServerTestFixture {
public:
    HttpServerTestFixture(int port) : 
        mockProcessor(new MockTelemetryProcessor()),
        port(port),
        isServerRunning(false) {
        DEBUG_LOG("Created test fixture for port " + std::to_string(port));
    }
    
    ~HttpServerTestFixture() {
        stopServer();
        delete mockProcessor;
        DEBUG_LOG("Destroyed test fixture for port " + std::to_string(port));
    }
    
    void startServer() {
        if (isServerRunning) {
            return;
        }
        
        // Configure server
        ServerConfig config;
        config.address = "127.0.0.1";
        config.port = port;
        config.threadCount = 1;
        
        // Create server
        server = std::make_unique<TelemetryHttpServer>(config, *mockProcessor);
        
        // Start server in a thread
        serverThread = std::make_unique<std::thread>([this]() {
            server->run();
        });
        
        // Allow server to start
        std::this_thread::sleep_for(std::chrono::seconds(1));
        isServerRunning = true;
        DEBUG_LOG("Server started on port " + std::to_string(port));
    }
    
    void stopServer() {
        if (!isServerRunning) {
            return;
        }
        
        DEBUG_LOG("Stopping server on port " + std::to_string(port));
        server->stop();
        serverThread->join();
        serverThread.reset();
        server.reset();
        isServerRunning = false;
        DEBUG_LOG("Server stopped on port " + std::to_string(port));
    }
    
    std::string getBaseUrl() const {
        return "http://localhost:" + std::to_string(port);
    }
    
    MockTelemetryProcessor* mockProcessor;
    int port;
    
private:
    std::unique_ptr<TelemetryHttpServer> server;
    std::unique_ptr<std::thread> serverThread;
    bool isServerRunning;
};

struct HttpResponse {
    int statusCode;
    std::map<std::string, std::string> headers;
    json body;
};

HttpResponse sendCurlRequest(const std::string& method, const std::string& url, const json& body) {
    // Get curl path from CMake-defined macro
    const char* curlPath = CURL_EXECUTABLE;
    if (!curlPath || strlen(curlPath) == 0) {
        curlPath = "curl"; // Fallback to system curl if not defined
    }
    
    // Create temporary files for the response, status code, and headers
    std::string tempFile = "/tmp/response_" + std::to_string(rand()) + ".json";
    std::string statusFile = "/tmp/status_" + std::to_string(rand()) + ".txt";
    std::string headersFile = "/tmp/headers_" + std::to_string(rand()) + ".txt";
    
    // Build curl command to capture response body, status code, and headers
    std::string command = std::string(curlPath) + " -s -X " + method + 
                          " -H \"Content-Type: application/json\" " +
                          "-d '" + body.dump() + "' " +
                          "-D " + headersFile + " " +  // Dump headers to file
                          "-w '%{http_code}' " +
                          url + " -o " + tempFile + " > " + statusFile;
    
    DEBUG_LOG("Executing: " + command);
    
    // Execute curl command
    int result = std::system(command.c_str());
    if (result != 0) {
        throw std::runtime_error("Failed to execute HTTP request: " + command);
    }
    
    // Read status code
    std::ifstream statusStream(statusFile);
    if (!statusStream.is_open()) {
        throw std::runtime_error("Failed to open status file: " + statusFile);
    }
    
    int statusCode;
    statusStream >> statusCode;
    statusStream.close();
    
    // Read headers
    std::map<std::string, std::string> headers;
    std::ifstream headersStream(headersFile);
    if (headersStream.is_open()) {
        std::string line;
        while (std::getline(headersStream, line)) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \r\n") + 1);
                
                headers[name] = value;
            }
        }
        headersStream.close();
    }
    
    // Read response from temporary file
    std::ifstream file(tempFile);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open response file: " + tempFile);
    }
    
    std::string responseStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Remove temporary files
    std::remove(tempFile.c_str());
    std::remove(statusFile.c_str());
    std::remove(headersFile.c_str());
    
    // Parse response as JSON
    json response;
    if (!responseStr.empty()) {
        try {
            response = json::parse(responseStr);
            DEBUG_LOG("Response: " + response.dump());
        } catch (const json::exception& e) {
            DEBUG_LOG("Invalid JSON: " + responseStr);
            // Just create empty JSON for non-JSON responses
            response = json::object();
        }
    } else {
        response = json::object();
        DEBUG_LOG("Empty response (valid empty JSON object)");
    }
    
    return {statusCode, headers, response};
}

SCENARIO("HTTP server handles REST API endpoints", "[http][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        DEBUG_LOG("Setting up test server");
        
        HttpServerTestFixture fixture(8095);
        fixture.startServer();
        
        WHEN("A POST request is sent to /paths/{event}") {
            DEBUG_LOG("Testing POST endpoint");
            
            // Test data
            std::string eventName = "test_event";
            std::vector<double> values(10, 1.5);
            uint64_t timestamp = 1617235200;
            
            // Prepare request
            json requestBody = {
                {"values", values},
                {"date", timestamp}
            };
            
            // Set expectations on mock
            REQUIRE_CALL(*fixture.mockProcessor, saveEvent(eventName, values, timestamp))
                .TIMES(1)
                .RETURN(true);
            
            // Send request using curl
            HttpResponse response = sendCurlRequest(
                "POST", 
                fixture.getBaseUrl() + "/paths/" + eventName, 
                requestBody
            );
            
            THEN("The server returns a successful response") {
                REQUIRE(response.statusCode == 200);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
                REQUIRE(response.body == json::object());
            }
        }
        
        WHEN("A GET request is sent to /paths/{event}/meanLength") {
            DEBUG_LOG("Testing GET endpoint");
            
            // Test data
            std::string eventName = "test_event";
            double expectedMean = 15.5;
            
            // Prepare request
            json requestBody = {
                {"resultUnit", "seconds"}
            };
            
            // Set expectations on mock
            REQUIRE_CALL(*fixture.mockProcessor, calculateMeanLength(eventName, std::optional<uint64_t>(), std::optional<uint64_t>()))
                .TIMES(1)
                .RETURN(expectedMean);
            
            // Send request using curl
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns the correct mean value") {
                REQUIRE(response.statusCode == 200);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
                REQUIRE_THAT(response.body["mean"].get<double>(), 
                           Catch::Matchers::WithinRel(expectedMean, 0.0001));
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}

SCENARIO("HTTP server can be initialized multiple times", "[http][init][bdd]") {
    GIVEN("A first HTTP server instance") {
        DEBUG_LOG("Creating first server instance");
        
        // Create first server
        HttpServerTestFixture fixture1(8096);
        fixture1.startServer();
        
        WHEN("The first server is stopped") {
            // Stop first server
            fixture1.stopServer();
            
            THEN("A second server can be started") {
                DEBUG_LOG("Creating second server instance");
                
                // Create second server
                HttpServerTestFixture fixture2(8097);
                fixture2.startServer();
                
                // Test that server is responsive
                // Set up a basic expectation
                REQUIRE_CALL(*fixture2.mockProcessor, calculateMeanLength(ANY(std::string), ANY(std::optional<uint64_t>), ANY(std::optional<uint64_t>)))
                    .TIMES(1)
                    .RETURN(42.0);
                
                // Make a request to the second server
                json requestBody = {
                    {"resultUnit", "seconds"}
                };
                
                HttpResponse response = sendCurlRequest(
                    "GET", 
                    fixture2.getBaseUrl() + "/paths/test_event/meanLength", 
                    requestBody
                );

                // Verify the server responded
                REQUIRE(response.statusCode == 200);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
                REQUIRE_THAT(response.body["mean"].get<double>(), 
                           Catch::Matchers::WithinRel(42.0, 0.0001));

                // Server is automatically stopped and cleaned up in fixture destructor
            }
        }
    }
}

SCENARIO("HTTP server validates timestamp parameters", "[http][validation][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        DEBUG_LOG("Setting up test server for timestamp validation test");
        
        HttpServerTestFixture fixture(8099);
        fixture.startServer();
        
        WHEN("A GET request is sent with startTimestamp > endTimestamp") {
            DEBUG_LOG("Testing timestamp validation");
            
            // Test data
            std::string eventName = "test_event";
            uint64_t startTimestamp = 1617408000; // Later date (2021-04-03)
            uint64_t endTimestamp = 1617235200;   // Earlier date (2021-04-01)
            
            // Prepare request
            json requestBody = {
                {"resultUnit", "seconds"},
                {"startTimestamp", startTimestamp},
                {"endTimestamp", endTimestamp}
            };
            
            // Setup mock for the 404 case (when validation is commented out)
            // This will only be called if we allow an invalid timestamp range to proceed
            ALLOW_CALL(*fixture.mockProcessor, calculateMeanLength(eventName, 
                                                     trompeloeil::eq(std::optional<uint64_t>(startTimestamp)), 
                                                     trompeloeil::eq(std::optional<uint64_t>(endTimestamp))))
                .RETURN(0.0);
            
            // Send request using enhanced curl function
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status with error message") {
                REQUIRE(response.statusCode == 400);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
                REQUIRE(response.body.contains("error"));
                REQUIRE(response.body["error"] == "startTimestamp must be less than or equal to endTimestamp");
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}

SCENARIO("HTTP server includes Content-Type headers in all responses", "[http][headers][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        // Set up server
        HttpServerTestFixture fixture(8101);
        fixture.startServer();
        
        WHEN("An error response is generated (invalid request body)") {
            // Prepare invalid request (missing required field)
            json requestBody = {
                {"missing", "resultUnit"}
            };
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/test_event/meanLength", 
                requestBody
            );
            
            THEN("The error response includes Content-Type: application/json header") {
                REQUIRE(response.statusCode == 400);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
            }
        }
        
        WHEN("A non-existent route is accessed") {
            // Send request to non-existent endpoint
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/non_existent_path", 
                json::object()
            );
            
            THEN("The 404 response includes Content-Type: application/json header") {
                REQUIRE(response.statusCode == 404);
                auto it = response.headers.find("Content-Type");
                REQUIRE(it != response.headers.end());
                REQUIRE(it->second.find("application/json") != std::string::npos);
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}

SCENARIO("HTTP server handles malformed input data appropriately", "[http][validation][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        HttpServerTestFixture fixture(8085);
        fixture.startServer();
        
        WHEN("A POST request is sent with non-integer date value") {
            std::string eventName = "test_event";
            std::vector<double> values(10, 1.5);
            
            // Prepare request with non-integer date
            json requestBody = {
                {"values", values},
                {"date", "2021-04-01"} // String instead of integer
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "POST", 
                fixture.getBaseUrl() + "/paths/" + eventName, 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions date format
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("Date") != std::string::npos);
            }
        }
        
        WHEN("A POST request is sent with non-numeric values in array") {
            std::string eventName = "test_event";
            
            // Create a JSON array with a non-numeric value
            json valuesArray = json::array();
            for (int i = 0; i < 9; i++) {
                valuesArray.push_back(1.5);
            }
            valuesArray.push_back("not_a_number");
            
            // Prepare request
            json requestBody = {
                {"values", valuesArray},
                {"date", 1617235200}
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "POST", 
                fixture.getBaseUrl() + "/paths/" + eventName, 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions values
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("values") != std::string::npos);
            }
        }
        
        WHEN("A GET request is sent with non-integer startTimestamp") {
            std::string eventName = "test_event";
            
            // Prepare request with invalid startTimestamp
            json requestBody = {
                {"resultUnit", "seconds"},
                {"startTimestamp", "yesterday"}, // String instead of integer
                {"endTimestamp", 1617408000}
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions startTimestamp
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("startTimestamp") != std::string::npos);
            }
        }
        
        WHEN("A GET request is sent with non-integer endTimestamp") {
            std::string eventName = "test_event";
            
            // Prepare request with invalid endTimestamp
            json requestBody = {
                {"resultUnit", "seconds"},
                {"startTimestamp", 1617235200},
                {"endTimestamp", "tomorrow"} // String instead of integer
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions endTimestamp
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("endTimestamp") != std::string::npos);
            }
        }
        
        WHEN("A GET request is sent with invalid resultUnit type") {
            std::string eventName = "test_event";
            
            // Prepare request with invalid resultUnit
            json requestBody = {
                {"resultUnit", 123} // Number instead of string
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions resultUnit
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("resultUnit") != std::string::npos);
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}

SCENARIO("HTTP server validates timestamp data correctly", "[http][validation][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        HttpServerTestFixture fixture(8086);
        fixture.startServer();
        
        WHEN("A POST request is sent with an excessively large timestamp") {
            std::string eventName = "test_event";
            std::vector<double> values(10, 1.5);
            
            // Create a very large timestamp - maximum uint64_t value
            auto largeTimestamp = std::numeric_limits<uint64_t>::max();
            
            // Prepare request
            json requestBody = {
                {"values", values},
                {"date", largeTimestamp}
            };
            
            // we NEED an expectation for the processor call with this large timestamp
            REQUIRE_CALL(*fixture.mockProcessor, saveEvent(eventName, values, largeTimestamp))
                .RETURN(true);
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "POST", 
                fixture.getBaseUrl() + "/paths/" + eventName, 
                requestBody
            );
            
            THEN("The server processes the request successfully") {
                REQUIRE(response.statusCode == 200);
                REQUIRE(response.body == json::object());
            }
        }

        WHEN("A GET request is sent with an invalid resultUnit value") {
            std::string eventName = "test_event";
            
            // Prepare request with invalid resultUnit value
            json requestBody = {
                {"resultUnit", "hours"} // Not one of the allowed values
            };
            
            // No expectations on mock - error should be caught before processor call
            
            // Send request
            HttpResponse response = sendCurlRequest(
                "GET", 
                fixture.getBaseUrl() + "/paths/" + eventName + "/meanLength", 
                requestBody
            );
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(response.statusCode == 400);
                REQUIRE(response.body.contains("error"));
                // Check that the error message mentions resultUnit
                std::string errorMsg = response.body["error"].get<std::string>();
                REQUIRE(errorMsg.find("resultUnit") != std::string::npos);
                REQUIRE(errorMsg.find("'seconds' or 'milliseconds'") != std::string::npos);
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}

SCENARIO("HTTP server handles malformed JSON correctly", "[http][validation][bdd]") {
    GIVEN("A running HTTP server with mock processor") {
        HttpServerTestFixture fixture(8087);
        fixture.startServer();
        
        WHEN("A POST request is sent with invalid JSON syntax") {
            std::string eventName = "test_event";
            
            // Prepare invalid JSON string (missing closing bracket)
            std::string invalidJson = R"({"values": [1,2,3,4,5,6,7,8,9,10], "date": 1617235200)";
            
            // Create a manually crafted curl command
            const char* curlPath = CURL_EXECUTABLE;
            if (!curlPath || strlen(curlPath) == 0) {
                curlPath = "curl"; // Fallback to system curl if not defined
            }
            
            std::string tempFile = "/tmp/response_" + std::to_string(rand()) + ".json";
            std::string statusFile = "/tmp/status_" + std::to_string(rand()) + ".txt";
            std::string headersFile = "/tmp/headers_" + std::to_string(rand()) + ".txt";
            
            std::string command = std::string(curlPath) + " -s -X POST" + 
                              " -H \"Content-Type: application/json\" " +
                              "-d '" + invalidJson + "' " +
                              "-D " + headersFile + " " +
                              "-w '%{http_code}' " +
                              fixture.getBaseUrl() + "/paths/" + eventName + 
                              " -o " + tempFile + " > " + statusFile;
            
            DEBUG_LOG("Executing: " + command);
            
            // Execute curl command
            int result = std::system(command.c_str());
            if (result != 0) {
                throw std::runtime_error("Failed to execute HTTP request with invalid JSON");
            }
            
            // Read status code
            std::ifstream statusStream(statusFile);
            int statusCode;
            statusStream >> statusCode;
            statusStream.close();
            
            // Clean up temporary files
            std::remove(tempFile.c_str());
            std::remove(statusFile.c_str());
            std::remove(headersFile.c_str());
            
            THEN("The server returns a 400 Bad Request status") {
                REQUIRE(statusCode == 400);
            }
        }
        
        // Server is automatically stopped and cleaned up in fixture destructor
    }
}
