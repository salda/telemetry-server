# ----- APPLICATION TARGET -----
function(setup_telemetry_server)
  set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry_storage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry_processor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http_server.cpp"
  )

  add_executable(telemetry-server ${SOURCES})
  target_include_directories(telemetry-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/telemetry"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(telemetry-server PRIVATE
    Threads::Threads
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    pistache
  )

  # Enable compiler warnings
  if(MSVC)
    target_compile_options(telemetry-server PRIVATE /W4)
  else()
    target_compile_options(telemetry-server PRIVATE -Wall -Wextra)
  endif()
endfunction()

setup_telemetry_server()
