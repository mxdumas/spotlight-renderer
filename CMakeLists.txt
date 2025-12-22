cmake_minimum_required(VERSION 3.15)
project(SpotlightRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

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
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    external
)

# Copy assets to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data)
