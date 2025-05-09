#pragma once

#include <string>
#include <memory>
#include "interfaces.h"

class TelemetryHttpServer : public IHttpServer {
public:
    explicit TelemetryHttpServer(const ServerConfig& config, ITelemetryProcessor& processor);
    ~TelemetryHttpServer() override;

    // Prevent copying or moving
    TelemetryHttpServer(const TelemetryHttpServer&) = delete;
    TelemetryHttpServer& operator=(const TelemetryHttpServer&) = delete;
    TelemetryHttpServer(TelemetryHttpServer&&) = delete;
    TelemetryHttpServer& operator=(TelemetryHttpServer&&) = delete;

    bool run() override;
    void stop() override;

private:
    // Private implementation details - not exposed in header
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
