project(Engine)

# This grabs all files in this directory that end with .cpp, .h, and .hpp
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp" "*.h" "*.hpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

set(IMGUI_DIR ../../external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
)
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

add_library(Engine STATIC ${SOURCE_FILES} ${IMGUI_SOURCES}
        Actor.cpp
        Actor.h
        Camera.cpp
        Camera.h
        Material.cpp
        Material.h
        Lighting.cpp
        Lighting.h
        Component.cpp
        Component.h
        PointLight.cpp
        PointLight.h
        BoneTransform.cpp
        BoneTransform.h
        Skeleton.cpp
        Skeleton.h
        Animation.cpp
        Animation.h
        Physics.cpp
        Physics.h
        CollisionBox.cpp
        CollisionBox.h
        Profiler.cpp
        JobManager.cpp
        JobManager.h
        ../Game/SimpleRotate.cpp
        ../Game/SimpleRotate.h
)

# debug define
target_compile_definitions(Engine PRIVATE _DEBUG)

# Precompiled Headers
target_precompile_headers(Engine PRIVATE pch.h)

# Add additional include directories
target_include_directories(Engine PRIVATE .)
target_include_directories(Engine PRIVATE ../../external/SDL3/include)
target_include_directories(Engine PRIVATE ../../external)
target_include_directories(Engine PRIVATE ../../external/cgltf)
target_include_directories(Engine PRIVATE ../../external/imgui)
target_include_directories(Engine PRIVATE ../../external/imgui/backends)
