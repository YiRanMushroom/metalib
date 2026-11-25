FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.16.1
        GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)

# MinGW runtime DLLs on PATH for test discovery and CTest
set(dl_paths)
if(MINGW)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH dl_paths)
endif()
# Compiler's own libstdc++ directory as run path elsewhere
set(libstdcxx_dir)
if(NOT MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so
            OUTPUT_VARIABLE libstdcxx OUTPUT_STRIP_TRAILING_WHITESPACE)
    cmake_path(GET libstdcxx PARENT_PATH libstdcxx_dir)
endif()

# One executable per test file: the files define unrelated test types with the same names
function(metalib_add_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE metalib::metalib Catch2::Catch2WithMain)
    target_auto_header_units(${name})
    if(libstdcxx_dir)
        set_target_properties(${name} PROPERTIES BUILD_RPATH "${libstdcxx_dir}")
    endif()
    catch_discover_tests(${name} DL_PATHS ${dl_paths} TEST_PREFIX "${name}: ")
endfunction()

set(metalib_test_sources
        ability_set_test.cpp
        ability_test.cpp
        class_test.cpp
        enumerator_test.cpp
        flatten_test.cpp
        identifier_test.cpp
        kinds_test.cpp
        meta_result_runtime_test.cpp
        misc_test.cpp
        namespace_test.cpp
        nested_type_test.cpp
        template_test.cpp
        type_test.cpp
)

foreach(source IN LISTS metalib_test_sources)
    cmake_path(GET source STEM name)
    metalib_add_test(${name} ${source})
endforeach()

# identifier checks again with the path for compilers without std::meta::exception
metalib_add_test(identifier_no_meta_exception_test identifier_test.cpp)
target_compile_definitions(identifier_no_meta_exception_test PRIVATE METALIB_HAS_META_EXCEPTION=0)
