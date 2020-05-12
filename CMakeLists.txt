cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(uCSV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(uCSV INTERFACE)
target_include_directories(uCSV INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")

option(UCSV_BUILD_TESTS "build tests" OFF)
if(UCSV_BUILD_TESTS)
    include(FetchContent)
    if(NOT COMMAND FetchContent_MakeAvailable)
        macro(FetchContent_MakeAvailable NAME)
            FetchContent_GetProperties(${NAME})
            string(TOLOWER "${NAME}" lcName)
            if(NOT ${lcName}_POPULATED)
                FetchContent_Populate(${NAME})
                add_subdirectory("${${lcName}_SOURCE_DIR}" "${${lcName}_BINARY_DIR}")
            endif()
        endmacro()
    endif()

    FetchContent_Declare(
    	Catch2
    	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    	GIT_TAG        v2.11.3
    )
    FetchContent_MakeAvailable(Catch2)

    file(GLOB_RECURSE test_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test/*.cpp")
    add_executable(test ${test_src})
    target_link_libraries(test uCSV Catch2)
endif()
