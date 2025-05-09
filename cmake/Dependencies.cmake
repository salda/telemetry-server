# ----- THIRD-PARTY DEPENDENCIES -----
include(FetchContent)

# Find required system packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# JSON library
function(setup_json_library)
  FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  set(JSON_MultipleHeaders ON CACHE INTERNAL "")
  FetchContent_MakeAvailable(json)
endfunction()

# Testing frameworks
function(setup_testing_libraries)
  FetchContent_Declare(Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_Declare(trompeloeil
    URL https://github.com/rollbear/trompeloeil/archive/refs/tags/v49.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )

  # Catch2 configuration
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "")
  set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "")
  set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "")
  set(CATCH_BUILD_TESTING OFF CACHE BOOL "")
  set(CATCH_ENABLE_COVERAGE OFF CACHE BOOL "")
  set(CATCH_ENABLE_WERROR OFF CACHE BOOL "")
  FetchContent_MakeAvailable(Catch2 trompeloeil)
endfunction()

# Date library and Pistache 
function(setup_http_libraries)
  # Date library - required by Pistache
  FetchContent_Declare(date
    URL https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(BUILD_TZ_LIB OFF CACHE BOOL "")
  set(USE_SYSTEM_TZ_DB ON CACHE BOOL "")
  set(USE_TZ_DB_IN_DOT OFF CACHE BOOL "")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "")
  FetchContent_MakeAvailable(date)

  # Pistache HTTP server library
  FetchContent_Declare(pistache
    URL https://github.com/pistacheio/pistache/archive/refs/tags/v0.4.26.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_GetProperties(pistache)
  if(NOT pistache_POPULATED)
    FetchContent_Populate(pistache)
  endif()
  file(GLOB_RECURSE PISTACHE_SOURCES "${pistache_SOURCE_DIR}/src/*.cc")
  add_library(pistache STATIC ${PISTACHE_SOURCES})
  target_include_directories(pistache PUBLIC 
    ${pistache_SOURCE_DIR}/include
    ${pistache_SOURCE_DIR}/third-party/rapidjson/include
    ${date_SOURCE_DIR}/include
  )
  target_link_libraries(pistache PUBLIC 
    Threads::Threads 
    OpenSSL::SSL 
    OpenSSL::Crypto
  )
  target_compile_definitions(pistache PUBLIC RAPIDJSON_HAS_STDSTRING=1)
  add_library(pistache::pistache ALIAS pistache)
endfunction()

# Setup all dependencies
setup_json_library()
setup_testing_libraries()
setup_http_libraries()
