# Create the telemetry core library (business logic)
add_library(telemetry-core
  core/telemetry_processor.cpp
  core/telemetry_storage.cpp
)

target_include_directories(telemetry-core PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(telemetry-core PUBLIC
  nlohmann_json::nlohmann_json
  Threads::Threads
)

# Create the HTTP server library (I/O)
add_library(telemetry-http
  http/http_server.cpp
)

target_include_directories(telemetry-http PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(telemetry-http PUBLIC
  telemetry-core  # HTTP depends on core
  pistache
  OpenSSL::SSL
  OpenSSL::Crypto
)
