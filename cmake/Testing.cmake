# ----- TEST TARGETS -----
function(setup_processor_tests)
  set(TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry_tests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry_processor.cpp"
  )
  add_executable(telemetry-processor-tests ${TEST_SOURCES})
  target_include_directories(telemetry-processor-tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/telemetry"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${trompeloeil_SOURCE_DIR}/include"
  )
  target_link_libraries(telemetry-processor-tests PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    pistache
    Catch2::Catch2WithMain
  )
endfunction()

function(setup_http_tests)
  set(HTTP_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/http_server_tests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http_server.cpp"
  )
  add_executable(telemetry-http-tests ${HTTP_TEST_SOURCES})
  target_include_directories(telemetry-http-tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/telemetry"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${trompeloeil_SOURCE_DIR}/include"
  )
  target_link_libraries(telemetry-http-tests PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    pistache
    Catch2::Catch2WithMain
  )

  # Find curl for HTTP tests
  find_program(CURL_EXECUTABLE curl)
  if(NOT CURL_EXECUTABLE)
    message(WARNING "curl executable not found. HTTP tests may fail if curl is not in the PATH.")
    set(CURL_EXECUTABLE "curl")
  endif()
  target_compile_definitions(telemetry-http-tests PRIVATE CURL_EXECUTABLE="${CURL_EXECUTABLE}")
endfunction()

function(setup_test_registration)
  # Register tests with CTest
  include(CTest)
  add_test(NAME processor_tests COMMAND telemetry-processor-tests)
  add_test(NAME http_tests COMMAND telemetry-http-tests)
endfunction()

setup_processor_tests()
setup_http_tests()
setup_test_registration()
