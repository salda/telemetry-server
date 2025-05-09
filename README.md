# Telemetry Server

A C++20 REST API server for tracking user interaction times across application screens.

## Overview

This telemetry server implements two REST API endpoints:

1. `POST /paths/{event}` - Saves event data with 10 time duration values
2. `GET /paths/{event}/meanLength` - Calculates the mean path length with optional time filtering

The system is designed using modern C++20 features and follows SOLID principles with interface-based design.

## Requirements

### System Dependencies

These need to be installed on your system:

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 19.27+)
- CMake 3.14+
- OpenSSL development libraries (including SSL and Crypto components)
- Threading support:
  - POSIX Threads (pthread) on Linux/macOS
  - Native threading libraries on Windows (automatically provided with the compiler)
- curl (required for running tests)

### Library Dependencies

These are automatically fetched and built by CMake:

- nlohmann/json v3.11.3 (JSON parsing)
- Pistache v0.4.26 (HTTP server framework)
- Howard Hinnant date library v3.0.3 (date handling)
- Catch2 v3.8.0 (testing framework)
- Trompeloeil v49 (mocking library)

### Installing dependencies on different platforms

#### Arch Linux

```bash
sudo pacman -S gcc cmake make openssl curl
```

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake libssl-dev curl
```

#### macOS

```bash
brew install cmake openssl curl
```

**Note:** On macOS, OpenSSL installed via Homebrew isn't linked to the system path by default. You might need to specify its path when running CMake:

```bash
cmake -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) ..
```

#### Windows

1. **Compiler & Build Tools:**
   - Install [Visual Studio](https://visualstudio.microsoft.com/downloads/) with "Desktop development with C++" workload, or
   - Install [MinGW-w64](https://www.mingw-w64.org/downloads/) or [MSYS2](https://www.msys2.org/)

2. **CMake:**
   - Download and install from [cmake.org](https://cmake.org/download/)
   - Add it to your PATH

3. **OpenSSL:**
   - Download the installer from [slproweb.com/products/Win32OpenSSL.html](https://slproweb.com/products/Win32OpenSSL.html) (Win64 version recommended)
   - Install it and add the bin directory to your PATH

4. **curl:**
   - Download from [curl.se/windows/](https://curl.se/windows/)
   - Extract and add the bin directory to your PATH, or
   - Install via [Chocolatey](https://chocolatey.org/): `choco install curl`

## Project Structure

```
telemetry-server/
├── CMakeLists.txt                     # Main CMake file
│
├── cmake/                             # CMake modules
│   ├── Dependencies.cmake             # External dependencies management
│   ├── Components.cmake               # Component build logic
│   └── Testing.cmake                  # Test configuration
│
├── include/                           # Public headers
│   └── telemetry/                     # Interface headers
│       ├── interfaces.h               # Interface definitions
│       ├── http_server.h              # HTTP server interface
│       ├── telemetry_processor.h      # Processor interface
│       └── telemetry_storage.h        # Storage interface
│
├── lib/                               # Library components
│   ├── CMakeLists.txt                 # Library build configuration
│   │
│   ├── core/                          # Business logic
│   │   ├── telemetry_processor.cpp    # Processor implementation
│   │   └── telemetry_storage.cpp      # Storage implementation
│   │
│   └── http/                          # I/O components
│       └── http_server.cpp            # HTTP server implementation
│
├── src/                               # Main executable
│   ├── CMakeLists.txt                 # Executable build configuration
│   └── main.cpp                       # Application entry point
│
└── tests/                             # Tests
    ├── CMakeLists.txt                 # Test build configuration
    ├── telemetry_tests.cpp            # Core functionality tests
    └── http_server_tests.cpp          # HTTP server tests
```

## Building

### Linux/macOS

```bash
# Create and navigate to build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make
```

### Windows (PowerShell)

```powershell
# Create and navigate to build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .
```

## Running the Server

```bash
# Linux/macOS
./src/telemetry-server 0.0.0.0 8080

# Windows
.\src\Debug\telemetry-server.exe 0.0.0.0 8080
```

## Running Tests

```bash
# Linux/macOS
./tests/telemetry-processor-tests
./tests/telemetry-http-tests

# Windows
.\tests\Debug\telemetry-processor-tests.exe
.\tests\Debug\telemetry-http-tests.exe
```

## API Documentation

### Save Event Data

**Endpoint:** `POST /paths/{event}`

**Request:**
```json
{
  "values": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
  "date": 1617235200
}
```

**Response:** `{}`

### Get Mean Path Length

**Endpoint:** `GET /paths/{event}/meanLength`

**Request:**
```json
{
  "resultUnit": "seconds",
  "startTimestamp": 1617235200,
  "endTimestamp": 1617408000
}
```

**Response:**
```json
{
  "mean": 15.5
}
```

### Testing API Endpoints Manually

You can use curl to test the API endpoints:

```bash
# Save Event Data
curl -X POST \
  http://localhost:8080/paths/user_flow \
  -H "Content-Type: application/json" \
  -d '{
    "values": [1.5, 2.0, 3.5, 1.0, 2.5, 3.0, 1.5, 2.0, 2.5, 3.5],
    "date": 1617235200
  }'

# Get Mean Path Length
curl -X GET \
  http://localhost:8080/paths/user_flow/meanLength \
  -H "Content-Type: application/json" \
  -d '{
    "resultUnit": "seconds"
  }'

# Get Mean Path Length with Time Range
curl -X GET \
  http://localhost:8080/paths/user_flow/meanLength \
  -H "Content-Type: application/json" \
  -d '{
    "resultUnit": "milliseconds",
    "startTimestamp": 1617235200,
    "endTimestamp": 1617408000
  }'
```

## Design Decisions and Technical Challenges

### Library Selection Considerations

During development, I faced several challenges with library selection:

- **HTTP Server Framework**: I avoided Boost despite its popularity due to its heavy dependencies, build complexities, and less modern API design. I explored alternatives:
  - **cpp-httplib**: Initially promising but rejected due to limitations handling GET requests with a body payload, which was required by our API specification
  - **Drogon**: Very performant but proved incompatible with BDD testing approach due to its singleton architecture
  - **Pistache**: Finally selected as it provided the best balance of modern API design, performance, and compatibility with our BDD testing requirements

- **Testing Framework**: Encountered some build integration challenges with Catch2 and Trompeloeil, but selected them anyway as they were the most suitable modern frameworks supporting BDD-style testing.

### Architecture Design Choices

- **Interface-Based Design**: While I personally prefer link-time substitution techniques for flexibility, I chose an interface-based design implementing the Dependency Inversion Principle for this project. This approach improves testability by allowing component isolation and easy mocking.

- **PIMPL Pattern**: Implemented the Pointer to Implementation (PIMPL) idiom in the HTTP server component to:
  - Hide implementation details and dependencies from header files
  - Reduce compilation dependencies and build times
  - Create a cleaner separation between the interface and implementation
  - Allow changing implementation details without affecting clients of the class

## Design Details

### Architecture

The project uses interface-based design with dependency injection following SOLID principles:

- **ITelemetryStorage** - Interface for data storage operations
- **ITelemetryProcessor** - Interface for business logic operations
- **IHttpServer** - Interface for server operations

### C++ Features Used

#### C++17 Features
- `std::optional` for optional parameters
- `std::shared_mutex` for reader-writer concurrency
- Smart pointers for memory management
- Standard algorithms from STL (`std::accumulate`, `std::transform`, `std::copy_if`)
- Lambda expressions

#### C++20 Features
- Ranges and views for filtering operations
- Parallel algorithms with execution policies 
- Direct dereferencing of optionals
- More concise syntax for common operations

### Testing Approach

BDD-style testing with Catch2 and Trompeloeil:

- Scenario-based test organization (GIVEN/WHEN/THEN)
- Mock objects for isolating components
- Expectations-based verification

## Performance Considerations

- Thread-safe storage with reader-writer lock
- Efficient filtering with ranges and views
- Parallel algorithms for computation on multicore systems
- Asynchronous HTTP server with thread pool

## License

This project is available under the MIT License.

```
MIT License

Copyright (c) 2025 [Your Name or Organization]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Third-Party Licenses

This project uses several third-party libraries, each with its own license:

- **nlohmann/json**: Licensed under the MIT License
- **Pistache**: Licensed under the Apache License 2.0
- **Howard Hinnant date library**: Licensed under the MIT License
- **Catch2**: Licensed under the Boost Software License 1.0
- **Trompeloeil**: Licensed under the Boost Software License 1.0

See the respective project repositories for full license details.