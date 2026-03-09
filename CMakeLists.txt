cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/cmake/vcpkg.cmake)
set(TARGET_NAME "VulkanPlayground")

project (${TARGET_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets/")

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(imgui REQUIRED)
find_package(Stb REQUIRED)
find_package(Taskflow REQUIRED)

FIND_PATH(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
if(TINYGLTF_INCLUDE_DIRS STREQUAL "")
    message(FATAL_ERROR "Tiny not found")
endif()

set(CORE_FILES
    inc/VulkanManager.h
    inc/Utils.h
    inc/WindowManager.h
    inc/ValidationManager.h
    inc/GraphicsTask.h
    inc/ComputeTask.h
    inc/Camera.h
    inc/Transform.h
    inc/Common.h
    inc/Timer.h
    inc/ImguiUtil.h
    inc/ImguiGlfwHelper.h
    inc/GltfLoader.h
    inc/SceneNode.h
    inc/SceneManager.h
    inc/Mesh.h
    inc/ImguiEditor.h
    inc/TimelineSemaphore.h
    inc/WireFrameTask.h
    inc/CameraUpdated.h

    src/VulkanManager.cpp
    src/ValidationManager.cpp
    src/WindowManager.cpp
    src/Utils.cpp
    src/GraphicsTask.cpp
    src/ComputeTask.cpp
    src/Camera.cpp
    src/Transform.cpp
    src/ImguiUtil.cpp
    src/GltfLoader.cpp
    src/SceneNode.cpp
    src/SceneManager.cpp
    src/ImguiEditor.cpp
    src/WireFrameTask.cpp
    src/CameraUpdated.cpp

    src/main.cpp
)

add_Executable(${TARGET_NAME} ${CORE_FILES})

target_include_directories(${TARGET_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:inc>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    PRIVATE
        ${TINYGLTF_INCLUDE_DIRS}
        ${Stb_INCLUDE_DIR}
)

target_compile_definitions(${TARGET_NAME} PUBLIC
    ASSETS_PATH="${ASSETS_PATH}"
    SPV_PATH="${CMAKE_BINARY_DIR}/Spvs/"
    GLFW_ENABLED
)

# target_include_directories(${TARGET_NAME} PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan glfw glm::glm imgui::imgui Taskflow::Taskflow)

add_custom_target(
    GeneratShaderBinaries
    DEPENDS ${ASSETS_PATH}
)

add_custom_command(
    TARGET GeneratShaderBinaries
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Spvs

    COMMAND glslangvalidator -V ${ASSETS_PATH}/shaders/UnlitColor.vert -o ${CMAKE_BINARY_DIR}/Spvs/UnlitColorVert.spv
    COMMAND glslangvalidator -V ${ASSETS_PATH}/shaders/UnlitColor.frag -o ${CMAKE_BINARY_DIR}/Spvs/UnlitColorFrag.spv

    COMMAND glslangvalidator -V ${ASSETS_PATH}/shaders/ClothSim.comp -o ${CMAKE_BINARY_DIR}/Spvs/ClothSim.spv
    COMMAND glslangvalidator -V ${ASSETS_PATH}/shaders/UnlitTextured.vert -o ${CMAKE_BINARY_DIR}/Spvs/UnlitTexturedVert.spv
    COMMAND glslangvalidator -V ${ASSETS_PATH}/shaders/UnlitTextured.frag -o ${CMAKE_BINARY_DIR}/Spvs/UnlitTexturedFrag.spv
)

add_dependencies(${TARGET_NAME} GeneratShaderBinaries)


