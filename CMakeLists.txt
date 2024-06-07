cmake_minimum_required(VERSION 3.14)

project("Hello-Vulkan")

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_subdirectory(external/SDL)
add_subdirectory(external/Vulkan-Headers)

add_executable(engine 
    main.cpp
)

target_link_libraries(engine PUBLIC SDL3::SDL3 Vulkan::Headers)
