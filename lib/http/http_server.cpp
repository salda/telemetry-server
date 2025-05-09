#include "telemetry/http_server.h"
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>

using json = nlohmann::json;

// Private implementation of the HTTP server class
class TelemetryHttpServer::Impl {
public:
    Impl(const ServerConfig& config, ITelemetryProcessor& processor)
        : config_(config), 
          processor_(processor),
          router_(),
          httpEndpoint_(std::make_shared<Pistache::Http::Endpoint>(Pistache::Address(config_.address, config_.port))) {
        
        // Configure the HTTP endpoint
        auto options = Pistache::Http::Endpoint::options()
            .threads(config_.threadCount)
            .flags(Pistache::Tcp::Options::ReuseAddr);
        
        httpEndpoint_->init(options);
        setupRoutes();
    }

    bool run() {
        std::cout << "Starting server on " << config_.address << ":" << config_.port 
                  << " with " << config_.threadCount << " threads" << std::endl;
        
        try {
            // Start the server
            httpEndpoint_->serve();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error starting server: " << e.what() << std::endl;
            return false;
        }
    }

    void stop() {
        httpEndpoint_->shutdown();
    }

private:
    void sendJsonResponse(Pistache::Http::ResponseWriter& response, 
                          Pistache::Http::Code code, 
                          const json& body) {
        response.headers().add<Pistache::Http::Header::ContentType>(
            Pistache::Http::Mime::MediaType::fromString("application/json"));
        response.send(code, body.dump());
    }

    void setupRoutes() {
        using namespace Pistache::Rest;
        
        // Set up the routes - use router_ directly, not a shared_ptr
        Routes::Post(router_, "/paths/:event", Routes::bind(&Impl::saveEvent, this));
        Routes::Get(router_, "/paths/:event/meanLength", Routes::bind(&Impl::getMeanLength, this));

        // Set up a catch-all 404 handler
        router_.addNotFoundHandler(Routes::bind(&Impl::notFoundHandler, this));

        // Install the router
        httpEndpoint_->setHandler(router_.handler());
    }

    void saveEvent(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
        // Get event name from route parameter
        auto eventName = request.param(":event").as<std::string>();
        
        // Parse JSON body from request
        json requestBody;
        try {
            requestBody = json::parse(request.body());
        } catch (const json::exception& e) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", std::string("Invalid JSON: ") + e.what()}});
            return;
        }

        // Validate request body
        if (!requestBody.contains("values") || !requestBody.contains("date")) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "Missing required fields: values, date"}});
            return;
        }

        // Extract and validate values
        if (!requestBody["values"].is_array()) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "Values must be an array"}});
            return;
        }

        std::vector<double> values;
        try {
            values.reserve(10);  // Preallocate capacity for exactly 10 elements
            for (const auto& val : requestBody["values"]) {
                if (!val.is_number()) {
                    sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                        json{{"error", "All values must be numeric"}});
                    return;
                }
                values.push_back(val.get<double>());
            }
        } catch (const json::exception& e) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                json{{"error", std::string("Invalid values array: ") + e.what()}});
            return;
        }
        
        // Extract and validate timestamp
        uint64_t timestamp;
        try {
            if (!requestBody["date"].is_number_integer()) {
                sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                    json{{"error", "Date must be an integer timestamp"}});
                return;
            }
            timestamp = requestBody["date"].get<uint64_t>();
        } catch (const json::exception& e) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                json{{"error", std::string("Invalid date format: ") + e.what()}});
            return;
        }

        // Process the event
        if (!processor_.saveEvent(eventName, values, timestamp)) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "Values array must contain exactly 10 elements"}});
            return;
        }

        // Return success response
        sendJsonResponse(response, Pistache::Http::Code::Ok, json::object());
    }

    void getMeanLength(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
        // Get event name from route parameter
        auto eventName = request.param(":event").as<std::string>();
        
        // Parse JSON body from request
        json requestBody;
        try {
            requestBody = json::parse(request.body());
        } catch (const json::exception& e) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", std::string("Invalid JSON: ") + e.what()}});
            return;
        }

        // Validate request body
        if (!requestBody.contains("resultUnit")) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "Missing required field: resultUnit"}});
            return;
        }

        // Validate result unit
        if (!requestBody["resultUnit"].is_string()) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                json{{"error", "resultUnit must be a string"}});
            return;
        }

        std::string resultUnit;
        try {
            resultUnit = requestBody["resultUnit"].get<std::string>();
        } catch (const json::exception& e) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                json{{"error", std::string("Invalid resultUnit format: ") + e.what()}});
            return;
        }

        if (resultUnit != "seconds" && resultUnit != "milliseconds") {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "resultUnit must be 'seconds' or 'milliseconds'"}});
            return;
        }

        // Extract optional timestamp filters
        std::optional<uint64_t> startTimestamp;
        std::optional<uint64_t> endTimestamp;

        if (requestBody.contains("startTimestamp")) {
            try {
                if (!requestBody["startTimestamp"].is_number_integer()) {
                    sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                        json{{"error", "startTimestamp must be an integer"}});
                    return;
                }
                startTimestamp = requestBody["startTimestamp"].get<uint64_t>();
            } catch (const json::exception& e) {
                sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                    json{{"error", std::string("Invalid startTimestamp format: ") + e.what()}});
                return;
            }
        }

        if (requestBody.contains("endTimestamp")) {
            try {
                if (!requestBody["endTimestamp"].is_number_integer()) {
                    sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                        json{{"error", "endTimestamp must be an integer"}});
                    return;
                }
                endTimestamp = requestBody["endTimestamp"].get<uint64_t>();
            } catch (const json::exception& e) {
                sendJsonResponse(response, Pistache::Http::Code::Bad_Request,
                    json{{"error", std::string("Invalid endTimestamp format: ") + e.what()}});
                return;
            }
        }

        if (startTimestamp && endTimestamp && *startTimestamp > *endTimestamp) {
            sendJsonResponse(response, Pistache::Http::Code::Bad_Request, 
                json{{"error", "startTimestamp must be less than or equal to endTimestamp"}});
            return;
        }
        
        // Calculate the mean
        double mean = processor_.calculateMeanLength(eventName, startTimestamp, endTimestamp);

        // Convert to milliseconds if requested
        if (resultUnit == "milliseconds") {
            mean *= 1000;
        }

        // Return result
        sendJsonResponse(response, Pistache::Http::Code::Ok, json{{"mean", mean}});
    }

    void notFoundHandler(const Pistache::Rest::Request&, Pistache::Http::ResponseWriter response) {
        sendJsonResponse(response, Pistache::Http::Code::Not_Found, json{{"error", "Resource not found"}});
    }

    ServerConfig config_;
    ITelemetryProcessor& processor_;
    Pistache::Rest::Router router_;
    std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint_;
};

// Implementation of the public interface
TelemetryHttpServer::TelemetryHttpServer(const ServerConfig& config, ITelemetryProcessor& processor)
    : pImpl(std::make_unique<Impl>(config, processor)) {
}

TelemetryHttpServer::~TelemetryHttpServer() = default;

bool TelemetryHttpServer::run() {
    return pImpl->run();
}

void TelemetryHttpServer::stop() {
    pImpl->stop();
}
