add_library(metalib INTERFACE)
add_library(metalib::metalib ALIAS metalib)

target_include_directories(metalib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(metalib INTERFACE cxx_std_26)

# Reflection
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(metalib_reflection_options -freflection)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # bloomberg/clang-p2996
    set(metalib_reflection_options -freflection-latest -stdlib=libc++)
    target_link_options(metalib INTERFACE -stdlib=libc++)
else()
    message(FATAL_ERROR "metalib: no reflection setup for ${CMAKE_CXX_COMPILER_ID}.")
endif()
target_compile_options(metalib INTERFACE ${metalib_reflection_options})

# Named module: metalib.cppm exports, by using-declarations, every entity the headers mark [[=metalib::exported]].
# module/generate_module.cpp finds them by reflection; it is compiled and run here, at configure time, again whenever
# a header changes.
if(METALIB_BUILD_MODULE)
    set(generator_source ${CMAKE_CURRENT_SOURCE_DIR}/module/generate_module.cpp)
    set(generator ${CMAKE_CURRENT_BINARY_DIR}/module/generate_module${CMAKE_EXECUTABLE_SUFFIX})
    set(generated ${CMAKE_CURRENT_BINARY_DIR}/module/generated.cppm)
    set(module_file ${CMAKE_CURRENT_BINARY_DIR}/module/metalib.cppm)
    file(GLOB module_inputs ${CMAKE_CURRENT_SOURCE_DIR}/include/metalib/*.hpp)
    list(APPEND module_inputs ${generator_source})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${module_inputs})

    set(outdated FALSE)
    if(NOT EXISTS ${generated} OR NOT EXISTS ${module_file})
        set(outdated TRUE)
    endif()
    foreach(input IN LISTS module_inputs)
        if(${input} IS_NEWER_THAN ${generated})
            set(outdated TRUE)
        endif()
    endforeach()

    if(outdated)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/module)
        message(STATUS "metalib: generating metalib.cppm")
        execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} -std=c++26 ${metalib_reflection_options}
                        -I ${CMAKE_CURRENT_SOURCE_DIR}/include ${generator_source} -o ${generator}
                RESULT_VARIABLE result ERROR_VARIABLE errors)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "metalib: cannot compile ${generator_source}:\n${errors}")
        endif()
        # the compiler's own directory first, for its runtime libraries
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_directory)
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E env --modify PATH=path_list_prepend:${compiler_directory}
                        ${generator} ${generated}
                RESULT_VARIABLE result ERROR_VARIABLE errors)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "metalib: cannot generate metalib.cppm (${result}):\n${errors}")
        endif()
    endif()
    # metalib.cppm keeps its timestamp when the generated text is the same
    configure_file(${generated} ${module_file} COPYONLY)

    add_library(metalib_module)
    add_library(metalib::module ALIAS metalib_module)
    target_sources(metalib_module PUBLIC
            FILE_SET CXX_MODULES BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/module FILES ${module_file})
    target_link_libraries(metalib_module PUBLIC metalib)
endif()
