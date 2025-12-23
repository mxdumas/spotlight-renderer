cmake_minimum_required(VERSION 3.15)
project(SpotlightRenderer LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)
list(FILTER SOURCES EXCLUDE REGEX "src/main.cpp")
file(GLOB IMGUI_SOURCES 
    external/imgui/imgui.cpp 
    external/imgui/imgui_draw.cpp 
    external/imgui/imgui_widgets.cpp 
    external/imgui/imgui_tables.cpp 
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_impl_win32.cpp
    external/imgui/imgui_impl_dx11.cpp
)

# External dependencies via vcpkg
find_package(miniz CONFIG REQUIRED)

set(EXTERNAL_SOURCES
    external/pugixml/pugixml.cpp
)

add_executable(${PROJECT_NAME} WIN32 src/main.cpp ${SOURCES} ${IMGUI_SOURCES} ${EXTERNAL_SOURCES})

# Target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_EXTENSIONS OFF
)

# Standard Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    d3dcompiler
    dxgi
    dxguid
    miniz::miniz
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    external
    external/imgui
    external/pugixml
    external/tinygltf
)

# Copy assets to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data)

# Tests
enable_testing()
add_executable(TestVolumetricJitter tests/test_volumetric_jitter.cpp)
target_include_directories(TestVolumetricJitter PRIVATE src external external/imgui)
target_link_libraries(TestVolumetricJitter PRIVATE d3d11 dxgi d3dcompiler)
add_test(NAME VolumetricJitterTest COMMAND TestVolumetricJitter)

add_executable(TestRayMath tests/test_ray_math.cpp)
target_include_directories(TestRayMath PRIVATE src)
add_test(NAME RayMathTest COMMAND TestRayMath)
