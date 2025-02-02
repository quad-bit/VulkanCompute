cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/cmake/vcpkg.cmake)
set(TARGET_NAME "VulkanPlayground")

project (${TARGET_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets/")

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 3.3 REQUIRED)

set(CORE_FILES
    inc/VulkanManager.h
    inc/Utils.h
    inc/WindowManager.h
    inc/ValidationManager.h
    inc/GraphicsTask.h

    src/VulkanManager.cpp
    src/ValidationManager.cpp
    src/WindowManager.cpp
    src/Utils.cpp
    src/GraphicsTask.cpp

    src/main.cpp
)

add_Executable(${TARGET_NAME} ${CORE_FILES})

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

target_compile_definitions(${TARGET_NAME} PUBLIC
    ASSETS_PATH="${ASSETS_PATH}"
    SPV_PATH="${CMAKE_BINARY_DIR}/Spvs/"
    GLFW_ENABLED
)

target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan glfw glm::glm)

# add_custom_command(
#     TARGET ${TARGET_NAME}
#     DEPENDS ${TARGET_NAME}
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Spvs
#     COMMAND glslangvalidator -V ${ASSETS_PATH}/Mandlebrot.comp -o ${CMAKE_BINARY_DIR}/Spvs/Mandlebrot.spv
#     COMMAND glslangvalidator -V ${ASSETS_PATH}/FullScreenQuadVert.vert -o ${CMAKE_BINARY_DIR}/Spvs/FullScreenQuadVert.spv
#     COMMAND glslangvalidator -V ${ASSETS_PATH}/FullScreenQuadFrag.frag -o ${CMAKE_BINARY_DIR}/Spvs/FullScreenQuadFrag.spv
# )
