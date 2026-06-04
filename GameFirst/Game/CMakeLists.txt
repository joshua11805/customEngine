project(Game)

# This grabs all files in this directory that end with .cpp, .h, and .hpp
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "*.cpp" "*.h" "*.hpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

# Enable some specific compile flags on Windows/Mac
include(../../cmake/SetCompileFlags.cmake)

# Build target and specify source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES}
		../Engine/TestCube.cpp
		../Engine/TestCube.h)

# debug define
target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)

# Precompiled Headers
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

# Add additional include directories
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_include_directories(${PROJECT_NAME} PRIVATE ../Engine)
target_include_directories(${PROJECT_NAME} PRIVATE ../../external/SDL3/include)
target_include_directories(${PROJECT_NAME} PRIVATE ../../external)

# Add Engine library
#add_subdirectory(../Engine build)
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

# Link against dependencies
if(WIN32)
	target_link_directories(${PROJECT_NAME} PRIVATE ../../external/SDL3/lib/x64)
	target_link_libraries(${PROJECT_NAME} PRIVATE SDL3 SDL3_shadercross)
	target_link_libraries(${PROJECT_NAME} PRIVATE dxcompiler.dll SDL3.dll SDL3_shadercross.dll)
else()
	target_link_directories(${PROJECT_NAME} PRIVATE ../../external/SDL3/lib/mac)
	target_link_libraries(${PROJECT_NAME} PRIVATE libSDL3_shadercross.a)
	target_link_libraries(${PROJECT_NAME} PRIVATE libSDL3.0.dylib libdxcompiler.dylib libspirv-cross-c-shared.0.dylib)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND /usr/libexec/PlistBuddy -c "Add :com.apple.security.cs.disable-library-validation bool true" coredump.entitlements
			COMMAND codesign -s - -f --entitlements coredump.entitlements $<TARGET_FILE:${PROJECT_NAME}>
			COMMAND rm -f coredump.entitlements
			VERBATIM)
endif()

# On Windows, we also need to copy the DLLs to the target directory
if(WIN32)
	file(GLOB_RECURSE MYDLLS "${PROJECT_SOURCE_DIR}/../../external/SDL3/lib/x64/*.dll")
	foreach(CurrentDllFile IN LISTS MYDLLS)
		add_custom_command(TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy "${CurrentDllFile}" "${CMAKE_CURRENT_BINARY_DIR}"
			COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
		)
	endforeach()
endif()
