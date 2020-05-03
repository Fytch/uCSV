cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(uCSV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fno-rtti")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb -fno-omit-frame-pointer -D_GLIBCXX_DEBUG -fsanitize=address -fsanitize=leak -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined" )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG -flto")

add_library(uCSV INTERFACE)
target_include_directories(uCSV INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")

option(UCSV_BUILD_TESTS "build tests" OFF)
if(UCSV_BUILD_TESTS)
    include(FetchContent)
    if(${CMAKE_VERSION} VERSION_LESS 3.14)
        macro(FetchContent_MakeAvailable NAME)
            FetchContent_GetProperties(${NAME})
            if(NOT ${NAME}_POPULATED)
                FetchContent_Populate(${NAME})
                add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
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
