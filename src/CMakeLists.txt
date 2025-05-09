# Main executable
add_executable(telemetry-server
  main.cpp
)

target_link_libraries(telemetry-server PRIVATE
  telemetry-core
  telemetry-http
)

# Installation rule
install(TARGETS telemetry-server
  RUNTIME DESTINATION bin
)
