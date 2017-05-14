# vulkan_triangle 2017

cmake_minimum_required(VERSION 3.7)
add_definitions(/W4 /MP /EHsc)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)

project(triangle)

message(STATUS "Trying to find Vulkan with find_package()")
find_package(Vulkan)

if (NOT Vulkan_FOUND)
    message(STATUS "Vulkan not found with find_package... trying manually")
    if (NOT Vulkan_INCLUDE_DIR)
        find_path(Vulkan_INCLUDE_DIR NAMES vulkan/vulkan.h
            HINTS "$ENV{VULKAN_SDK}/Include" "$ENV{VULKAN_SDK_PATH}/Include")
    endif()
    if (NOT Vulkan_LIBRARY)
        find_library(Vulkan_LIBRARY NAMES vulkan-1
            HINTS "$ENV{VULKAN_SDK}/Lib" "$ENV{VULKAN_SDK_PATH}/Lib")
    endif()
    if (Vulkan_INCLUDE_DIR AND Vulkan_LIBRARY)
        set(Vulkan_FOUND on)
        message(STATUS "Vulkan found from install paths")
    endif()
endif()

include_directories(${Vulkan_INCLUDE_DIR})

set(APP_SOURCE
    "src/main.cpp"
    "src/GfxResources.h" "src/GfxResources.cpp"
    "src/Engine.h" "src/Engine.cpp"
    "src/Renderer.h" "src/Renderer.cpp"
    "src/Window.h" "src/Window.cpp"
    "src/Utils.h"
    )

set(SHADERS "shaders/triangle.vert" "shaders/triangle.frag")
set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${CMAKE_PROJECT_NAME} ${APP_SOURCE} ${SHADERS})
target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARY})
