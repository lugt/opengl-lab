project(example-gl)
cmake_minimum_required(VERSION 3.0)

add_library(glad_lib src/glad.c)
target_include_directories(glad_lib PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/glad)

# find_package(GLFW REQUIRED glfw3)
# include_directories(${GLFW_INCLUDE_DIRS})
# Add this line to tell the linker where to find the libraries.
message(STATUS "GLFW_INCLUDE_DIRS at ${GLFW_INCLUDE_DIRS} FOUND")
message(STATUS "GLFW_LIBRARIES at ${GLFW_LIBRARIES} FOUND")

add_executable(basic_window
        src/main2.cxx)
target_compile_options(basic_window PRIVATE
        -std=c++11
        -stdlib=libc++
        )
#"-framework CoreVideo"
#"-framework OpenGL"
#"-framework IOKit"
#"-framework Cocoa"
#"-framework Carbon"
target_link_directories(basic_window PRIVATE ${GLFW_LIBRARY_DIRS})
target_include_directories(basic_window PRIVATE
        /usr/local/Cellar/glfw/3.3.2/include
        /usr/local/include
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/glad)
target_link_options(basic_window PRIVATE
        -L/usr/local/lib
        -lglfw)
target_link_libraries(basic_window PRIVATE
        ${GLFW_LIBRARIES}
        glad_lib)
