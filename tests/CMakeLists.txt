# Processor tests
add_executable(telemetry-processor-tests
  telemetry_tests.cpp
)

target_include_directories(telemetry-processor-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${trompeloeil_SOURCE_DIR}/include
)

target_link_libraries(telemetry-processor-tests PRIVATE
  telemetry-core
  Catch2::Catch2WithMain
)

# HTTP server tests
add_executable(telemetry-http-tests
  http_server_tests.cpp
)

target_include_directories(telemetry-http-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${trompeloeil_SOURCE_DIR}/include
)

target_link_libraries(telemetry-http-tests PRIVATE
  telemetry-http
  Catch2::Catch2WithMain
)

# Find curl for HTTP tests
find_program(CURL_EXECUTABLE curl)
if(NOT CURL_EXECUTABLE)
  message(WARNING "curl executable not found. HTTP tests may fail if curl is not in the PATH.")
  set(CURL_EXECUTABLE "curl")
endif()
target_compile_definitions(telemetry-http-tests PRIVATE CURL_EXECUTABLE="${CURL_EXECUTABLE}")

# Register tests with CTest
add_test(NAME processor_tests COMMAND telemetry-processor-tests)
add_test(NAME http_tests COMMAND telemetry-http-tests)
