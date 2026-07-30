cmake_minimum_required(VERSION 3.20)
project(OpenGLProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Identify your source files (Make sure glad.c is included!)
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/glad.c
    src/Triangle.cpp
)

# CMake complie glad.c 
set_source_files_properties(src/glad.c PROPERTIES LANGUAGE CXX)

# 2. Tell CMake where your header folders are (GLFW, GLAD, GLM)
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
)

# 3. Locate the built-in Windows OpenGL library
find_package(OpenGL REQUIRED)

# CMake can see inside root all files 
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# 4. Link GLFW and the Windows OpenGL driver to your project
# (Assumes you placed your compiled glfw3.lib in a path CMake can see, or built it inline)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw3
    OpenGL::GL
)

# 5. Asstes to build Directory shaders 
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets"
    COMMENT "Copying assets folder to executable output directory..."
)