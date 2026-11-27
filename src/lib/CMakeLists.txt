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

# Named module: one interface file for every namespace the headers mark with metalib::codegen::export_module_name,
# exporting, by using-declarations, what they mark [[=metalib::codegen::exported]]. module/generate_module.cpp writes
# the files with metalib::codegen::module_generator; it is compiled and run here, at configure time, again whenever
# a header changes.
if(METALIB_BUILD_MODULE)
    set(generator_source ${CMAKE_CURRENT_SOURCE_DIR}/module/generate_module.cpp)
    set(generator ${CMAKE_CURRENT_BINARY_DIR}/module/generate_module${CMAKE_EXECUTABLE_SUFFIX})
    set(generated_directory ${CMAKE_CURRENT_BINARY_DIR}/module/generated)
    set(generated_stamp ${CMAKE_CURRENT_BINARY_DIR}/module/generated.stamp)
    set(module_directory ${CMAKE_CURRENT_BINARY_DIR}/module/interface)
    file(GLOB_RECURSE module_inputs ${CMAKE_CURRENT_SOURCE_DIR}/include/metalib/*.hpp)
    list(APPEND module_inputs ${generator_source})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${module_inputs})

    set(outdated FALSE)
    if(NOT EXISTS ${generated_stamp})
        set(outdated TRUE)
    endif()
    foreach(input IN LISTS module_inputs)
        if(${input} IS_NEWER_THAN ${generated_stamp})
            set(outdated TRUE)
        endif()
    endforeach()

    if(outdated)
        file(REMOVE_RECURSE ${generated_directory})
        file(MAKE_DIRECTORY ${generated_directory})
        message(STATUS "metalib: generating the module interfaces")
        execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} -std=c++26 ${metalib_reflection_options} -DMETALIB_CODEGEN
                        -I ${CMAKE_CURRENT_SOURCE_DIR}/include ${generator_source} -o ${generator}
                RESULT_VARIABLE result ERROR_VARIABLE errors)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "metalib: cannot compile ${generator_source}:\n${errors}")
        endif()
        # the compiler's own directory first, for its runtime libraries
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_directory)
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E env --modify PATH=path_list_prepend:${compiler_directory}
                        ${generator} ${generated_directory}
                RESULT_VARIABLE result ERROR_VARIABLE errors)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "metalib: cannot generate the module interfaces (${result}):\n${errors}")
        endif()
        file(TOUCH ${generated_stamp})
    endif()

    # an interface file keeps its timestamp when its generated text is the same; one no longer generated is removed
    file(GLOB_RECURSE generated_files RELATIVE ${generated_directory} ${generated_directory}/*.cppm)
    file(GLOB_RECURSE present_files RELATIVE ${module_directory} ${module_directory}/*.cppm)
    foreach(present IN LISTS present_files)
        if(NOT present IN_LIST generated_files)
            file(REMOVE ${module_directory}/${present})
        endif()
    endforeach()
    set(module_files)
    foreach(generated IN LISTS generated_files)
        configure_file(${generated_directory}/${generated} ${module_directory}/${generated} COPYONLY)
        list(APPEND module_files ${module_directory}/${generated})
    endforeach()

    add_library(metalib_module)
    add_library(metalib::module ALIAS metalib_module)
    target_sources(metalib_module PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${module_directory} FILES ${module_files})
    target_link_libraries(metalib_module PUBLIC metalib)
endif()
