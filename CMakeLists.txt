cmake_minimum_required(VERSION 3.18)

# Option to enable/disable CUDA support
option(USE_CUDA "Enable CUDA support for GPU acceleration" OFF)

if(USE_CUDA)
    project(ParticleSimulator LANGUAGES CXX CUDA)
else()
    project(ParticleSimulator LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(USE_CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED OFF)
endif()

# Qt6 MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
if(USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()
find_package(raylib CONFIG REQUIRED)

# Source files
set(CORE_SOURCES
    src/core/Application.cpp
    src/core/SimulationState.cpp
)

set(PHYSICS_SOURCES
    src/physics/CPUPhysicsEngine.cpp
)

if(USE_CUDA)
    list(APPEND PHYSICS_SOURCES src/physics/GPUPhysicsEngine.cpp)
endif()

set(RENDERER_SOURCES
    src/renderer/Renderer.cpp
    src/renderer/RaylibWindow.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/ControlPanel.cpp
)

if(USE_CUDA)
    set(CUDA_SOURCES
        src/cuda/CudaKernels.cu
        src/cuda/CudaUtils.cu
        src/cuda/CollisionGrid.cu
    )
endif()

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${PHYSICS_SOURCES}
    ${RENDERER_SOURCES}
    ${UI_SOURCES}
)

# Create executable
if(USE_CUDA)
    add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${CUDA_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${ALL_SOURCES})
endif()

# CUDA configuration
if(USE_CUDA)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES "50;60;70;75;80;86"
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

if(USE_CUDA)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    raylib
)

if(USE_CUDA)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        CUDA::cudart
    )
endif()

# Windows specific
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Copy Qt DLLs
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

    # Copy Qt platform plugin
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${Qt6_DIR}/../../../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/
    )
endif()

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
)

if(USE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CUDA)
endif()
