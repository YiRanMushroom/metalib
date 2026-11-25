add_library(metalib INTERFACE)
add_library(metalib::metalib ALIAS metalib)

target_include_directories(metalib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(metalib INTERFACE cxx_std_26)

# Reflection
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(metalib INTERFACE -freflection)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # bloomberg/clang-p2996
    target_compile_options(metalib INTERFACE -freflection-latest -stdlib=libc++)
    target_link_options(metalib INTERFACE -stdlib=libc++)
else()
    message(FATAL_ERROR "metalib: no reflection setup for ${CMAKE_CXX_COMPILER_ID}.")
endif()
