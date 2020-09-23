# FetchContent_MakeAvailable is available since CMake version 3.14
cmake_minimum_required(VERSION 3.14)

# Setting up dlib as an external package
# set(HUNTER_Boost_COMPONENTS Filesystem Python)
# set(HUNTER_PACKAGES Boost dlib ZLIB)
set(HUNTER_URL "https://github.com/cpp-pm/hunter/archive/v0.23.224.tar.gz")
set(HUNTER_SHA1 "18e57a43efc435f2e1dae1291e82e42afbf940be")

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

# Using the dlib package
# find_package(dlib REQUIRED)

project(example-gl)

add_library(glad_lib src/glad.c)
target_include_directories(glad_lib PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/glad)

# find_package(GLFW REQUIRED glfw3)
# include_directories(${GLFW_INCLUDE_DIRS})
# Add this line to tell the linker where to find the libraries.
message(STATUS "GLFW_INCLUDE_DIRS at ${GLFW_INCLUDE_DIRS} FOUND")
message(STATUS "GLFW_LIBRARIES at ${GLFW_LIBRARIES} FOUND")

add_library(basic_library
        src/stb_impl.cxx
        src/Common/InitShader.cpp)
target_compile_options(basic_library PRIVATE
        -std=c++11
        -stdlib=libc++
        -g)
target_include_directories(basic_library PRIVATE
        /usr/local/Cellar/glfw/3.3.2/include
        /usr/local/include
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/glad
        ${CMAKE_CURRENT_LIST_DIR}/include/stb)


add_executable(basic_window src/main2.cxx)
add_executable(triangle src/main.cpp)
add_executable(delayance src/delayance.cxx)

set(ALL_TARGETS_IN_PROJ basic_window triangle delayance)
foreach(ONE_TARG IN LISTS ALL_TARGETS_IN_PROJ)
    target_link_directories(${ONE_TARG} PRIVATE ${GLFW_LIBRARY_DIRS})
    target_include_directories(${ONE_TARG} PRIVATE
            /usr/local/Cellar/glfw/3.3.2/include
            /usr/local/include
            ${CMAKE_CURRENT_LIST_DIR}/include
            ${CMAKE_CURRENT_LIST_DIR}/include/glad
            ${CMAKE_CURRENT_LIST_DIR}/include/stb)
    target_link_options(${ONE_TARG} PRIVATE
            -L/usr/local/lib
            -lglfw)
    target_link_libraries(${ONE_TARG} PRIVATE
            ${GLFW_LIBRARIES}
            basic_library
            glad_lib)
endforeach()

#"-framework CoreVideo"
#"-framework OpenGL"
#"-framework IOKit"
#"-framework Cocoa"
#"-framework Carbon"