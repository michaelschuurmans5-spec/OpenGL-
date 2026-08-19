cmake_minimum_required(VERSION 3.20)

project(OpenGLProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# ASSIMP
# =========================================================

# Build Assimp as a static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

# Don't build Assimp tests
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/assimp
)
# =========================================================
# OPENGL PROJECT
# =========================================================

add_executable(${PROJECT_NAME}

    src/main.cpp
    src/glad.c
    src/Triangle.cpp
    src/Shader.cpp
    src/stb_image.cpp
    src/Model.cpp
    src/Sky.cpp

    # ImGui
    include/imgui/imgui.cpp
    include/imgui/imgui_draw.cpp
    include/imgui/imgui_tables.cpp
    include/imgui/imgui_widgets.cpp
    include/imgui/imgui_demo.cpp

    include/imgui/backends/imgui_impl_glfw.cpp
    include/imgui/backends/imgui_impl_opengl3.cpp

    # ImGuizmo
    include/imgui/backends/ImGuizmo.cpp

    # Headers do NOT need to be listed here.
 include/GBuffer.h src/GBuffer.cpp)

# =========================================================
# IMGUI / IMGUIZMO
# =========================================================

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

# =========================================================
# GLAD
# =========================================================

set_source_files_properties(
    src/glad.c
    PROPERTIES
    LANGUAGE CXX
)

# =========================================================
# INCLUDE DIRECTORIES
# =========================================================

target_include_directories(${PROJECT_NAME}
    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/include

    ${CMAKE_CURRENT_SOURCE_DIR}/include/glm

    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui

    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/backends
)

# =========================================================
# OPENGL
# =========================================================

find_package(OpenGL REQUIRED)

# =========================================================
# LIBRARIES
# =========================================================

target_link_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE

    glfw3
    OpenGL::GL

    # Assimp
    assimp
)

# =========================================================
# COPY ASSETS (Includes Assets/Shaders, Models, Textures)
# =========================================================

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets"

    COMMENT "Copying Assets folder to output directory..."
)