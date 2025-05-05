cmake_minimum_required(VERSION 3.11) # FetchContent requires 3.11+
project(GalaxyInvaders CXX)

set(CMAKE_CXX_STANDARD 14) # SFML 2.5.1 works well with C++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch SFML using FetchContent ---
include(FetchContent)
# BUILD_SHARED_LIBS OFF is generally recommended for FetchContent
# unless you specifically need shared libs and manage deployment.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static libraries")
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.5.1 # Using SFML 2.5.1 tag
)
# Make SFML available, this downloads and configures SFML
FetchContent_MakeAvailable(SFML)

# --- Add Executable ---
add_executable(GalaxyInvaders main.cpp)

# --- Link SFML ---
# Link against the targets provided by FetchContent's SFML build
# For SFML 2.5.x, the targets are typically sfml-graphics, sfml-window, sfml-system
target_link_libraries(GalaxyInvaders PRIVATE sfml-graphics sfml-window sfml-system)

# --- Optional: Ensure font file is accessible ---
# If arial.ttf is in your source directory, this helps copy it to the build dir
# where the executable runs from by default in CLion.
set(ASSET_FILES
        resources/arial.ttf
        resources/player.png
        resources/enemy.jpg
        resources/bullet.png
        resources/enemy_bullet.png
)
foreach(ASSET_FILE ${ASSET_FILES})
    get_filename_component(ASSET_FILENAME ${ASSET_FILE} NAME)
    set(SOURCE_ASSET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_FILE}")
    set(DESTINATION_ASSET_PATH "$<TARGET_FILE_DIR:GalaxyInvaders>/${ASSET_FILENAME}")

    # Check if the source file exists
    if(EXISTS "${SOURCE_ASSET_PATH}")
        add_custom_command(TARGET GalaxyInvaders POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SOURCE_ASSET_PATH}"
                "${DESTINATION_ASSET_PATH}" # Copy directly into the target directory
                COMMENT "Copying ${ASSET_FILENAME} to build directory"
        )
    else()
        message(WARNING "Asset file not found: ${SOURCE_ASSET_PATH}")
    endif()
endforeach()
