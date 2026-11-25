cmake_minimum_required(VERSION 4.3)
project(metalib VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(METALIB_BUILD_TESTS "Build the metalib tests" ${PROJECT_IS_TOP_LEVEL})

if(METALIB_BUILD_TESTS)
    include(FetchContent)

    # Header units
    FetchContent_Declare(cmake_header_units
            GIT_REPOSITORY https://github.com/YiRanMushroom/cmake-header-units.git
            GIT_TAG        release
            GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(cmake_header_units)
    list(APPEND CMAKE_MODULE_PATH "${cmake_header_units_SOURCE_DIR}/cmake")
    include(HeaderUnits)

    enable_testing()
endif()

add_subdirectory(src/lib)

if(METALIB_BUILD_TESTS)
    add_subdirectory(src/test)
endif()
