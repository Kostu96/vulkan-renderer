cmake_minimum_required(VERSION 3.28)

project(vulkan-renderer LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package (Vulkan REQUIRED)

add_executable(app
    src/main.cpp
    src/volk.cpp
)

target_compile_features(app PRIVATE cxx_std_23)

target_include_directories(app PRIVATE
    third_party/volk/include
)

target_link_libraries(app PRIVATE
    SDL3::SDL3
    Vulkan::Headers
    #Vulkan::Vulkan
)
