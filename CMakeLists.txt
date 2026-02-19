cmake_minimum_required(VERSION 3.28)

project(vulkan-renderer LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package (Vulkan REQUIRED)

set(APP_SOURCES
    src/utils/non_copyable.hpp
    src/vulkan_utils/device.cpp
    src/vulkan_utils/device.hpp
    src/vulkan_utils/fence.cpp
    src/vulkan_utils/fence.hpp
    src/vulkan_utils/instance.cpp
    src/vulkan_utils/instance.hpp
    src/vulkan_utils/pipeline.cpp
    src/vulkan_utils/pipeline.hpp
    src/vulkan_utils/semaphore.cpp
    src/vulkan_utils/semaphore.hpp
    src/vulkan_utils/swapchain.cpp
    src/vulkan_utils/swapchain.hpp
    src/vulkan_utils/vma.cpp
    src/vulkan_utils/volk.cpp
    src/vulkan_utils/vulkan_utils.cpp
    src/vulkan_utils/vulkan_utils.hpp
    src/main.cpp
    src/SDL_surface.cpp
    src/SDL_surface.hpp
)

add_executable(app
    CMakeLists.txt
    ${APP_SOURCES}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${APP_SOURCES})

target_compile_features(app PRIVATE cxx_std_23)

target_include_directories(app PRIVATE
    src
    third_party/vma/include
    third_party/volk/include
)

target_link_libraries(app PRIVATE
    glm::glm
    SDL3::SDL3
    Vulkan::Headers
)

find_program(SLANGC_EXECUTABLE
    NAMES
    slangc
    slangc.exe
)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

file(GLOB SHADER_FILES
    ${SHADER_SOURCE_DIR}/*.slang
)

set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME_WE)
    set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${SLANGC_EXECUTABLE}
            ${SHADER}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name -entry vert_main -entry frag_main
            -o ${OUTPUT_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${FILE_NAME}"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${OUTPUT_FILE})
endforeach()

add_custom_target(compile_shaders ALL
    DEPENDS ${COMPILED_SHADERS}
)

add_dependencies(app compile_shaders)
