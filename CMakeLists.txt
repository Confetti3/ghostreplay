cmake_minimum_required(VERSION 3.25)
project(GhostReplay VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 6 (Phase 1+)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED QT6_DIR)
    message(STATUS "QT6_DIR not set — searching common locations")
    file(GLOB _qt_candidates "C:/Qt/6*/msvc*" "$ENV{LOCALAPPDATA}/Qt/6*/msvc*")
    foreach(_dir ${_qt_candidates})
        if(EXISTS "${_dir}/lib/Qt6Core.lib")
            set(QT6_DIR "${_dir}")
            break()
        endif()
    endforeach()
endif()

if(QT6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${QT6_DIR}")
    message(STATUS "Qt 6: ${QT6_DIR}")
else()
    message(STATUS "Qt 6 not auto-detected — if Phase 1+ UI is needed, set QT6_DIR")
endif()

# nlohmann/json (header-only, MIT)
find_package(nlohmann_json 3.11.3 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found - fetching v3.11.3")
    include(FetchContent)
    FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

# ── Dependencies ──────────────────────────────────────────────

# FFmpeg (dev build with include/ and lib/ directories)
# Set FFMPEG_DIR to the root of a Gyan "full_build-shared" or equivalent install
if(NOT DEFINED FFMPEG_DIR)
    message(STATUS "FFMPEG_DIR not set — searching common locations")
    file(GLOB _candidates "$ENV{ProgramFiles}/ffmpeg*" "C:/ffmpeg*" "$ENV{LOCALAPPDATA}/ffmpeg*")
    foreach(_dir ${_candidates})
        if(EXISTS "${_dir}/include/libavcodec/avcodec.h")
            set(FFMPEG_DIR "${_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT FFMPEG_DIR)
    message(FATAL_ERROR "FFMPEG_DIR not set and not auto-detected. "
        "Download a shared FFmpeg dev build from https://www.gyan.dev/ffmpeg/builds/ "
        "and set FFMPEG_DIR to its root.")
endif()

set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/include")
file(GLOB FFMPEG_LIB_DIRS "${FFMPEG_DIR}/lib" "${FFMPEG_DIR}/lib/x64")

set(FFMPEG_COMPONENTS avcodec avformat avutil swresample swscale)
foreach(_comp ${FFMPEG_COMPONENTS})
    find_library(FFMPEG_${_comp}_LIBRARY
        NAMES ${_comp}
        PATHS ${FFMPEG_LIB_DIRS}
        REQUIRED)
    list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${_comp}_LIBRARY})
endforeach()

message(STATUS "FFmpeg: ${FFMPEG_DIR}")

# Windows SDK — D3D11, DXGI
# Using direct .lib names for MSVC; these are in the default linker search path.
if(MSVC)
    set(D3D11_LIB d3d11.lib)
    set(DXGI_LIB  dxgi.lib)
    set(DXGUID_LIB dxguid.lib)
    set(SHELL32_LIB shell32.lib)
    set(DWMAPI_LIB dwmapi.lib)
else()
    find_library(D3D11_LIB d3d11 REQUIRED)
    find_library(DXGI_LIB dxgi REQUIRED)
    find_library(DXGUID_LIB dxguid REQUIRED)
    if(WIN32)
        find_library(SHELL32_LIB shell32 REQUIRED)
        find_library(DWMAPI_LIB dwmapi REQUIRED)
    endif()
endif()

# Qt 6 (optional for Phase 0, required for Phase 1+)
find_package(Qt6 QUIET COMPONENTS Core Widgets Quick QuickControls2 QuickDialogs2 Multimedia)
if(Qt6_FOUND)
    message(STATUS "Qt 6 found — UI features enabled")
else()
    message(WARNING "Qt 6 not found — building without UI (Phase 0 mode)")
endif()

# ── Sources ───────────────────────────────────────────────────

set(SOURCES
    src/main.cpp

    src/capture/CaptureManager.cpp
    src/capture/DXGICapture.cpp
    src/capture/WGCCapture.cpp

    src/encode/EncoderSelector.cpp
    src/encode/FFmpegVideoEncoder.cpp

    src/audio/AudioCapture.cpp
    src/audio/AudioEncoder.cpp

    src/buffer/RingBuffer.cpp
    src/buffer/KeyframeIndex.cpp

    src/hotkey/HotkeyManager.cpp

    src/export/ClipExporter.cpp
    src/export/TranscodeJob.cpp

    src/game/GameDetector.cpp

    src/util/Logger.cpp
    src/util/Config.cpp
    src/util/Thumbnail.cpp
    src/util/ClipProbe.cpp
)

# Qt UI sources (compiled only when HAS_QT6 is defined)
set(QT_UI_SOURCES
    src/ui/GRApplication.cpp
    src/ui/SettingsWindow.cpp
    src/ui/Backend.cpp
    src/ui/WindowChromeController.cpp
    src/ui/ThumbnailProvider.cpp
    src/ui/ClipPreviewController.cpp
    src/util/FileActions.cpp
    src/util/ReleaseHealth.cpp
)

set(QT_UI_HEADERS
    src/ui/GRApplication.h
    src/ui/SettingsWindow.h
    src/ui/Backend.h
    src/ui/WindowChromeController.h
    src/ui/ThumbnailProvider.h
    src/ui/ClipPreviewController.h
    src/util/FileActions.h
    src/util/ReleaseHealth.h
)

set(HEADERS
    src/capture/ICapture.h
    src/capture/CaptureManager.h
    src/capture/CaptureTarget.h
    src/capture/WGCCapture.h

    src/encode/IEncoder.h
    src/encode/EncoderSelector.h
    src/encode/FFmpegVideoEncoder.h
    src/encode/EncoderConfig.h

    src/audio/AudioCapture.h
    src/audio/AudioEncoder.h

    src/buffer/RingBuffer.h
    src/buffer/KeyframeIndex.h

    src/hotkey/HotkeyManager.h

    src/export/ClipExporter.h
    src/export/TranscodeJob.h

    src/game/GameDetector.h

    src/util/Logger.h
    src/util/Config.h
    src/util/Thumbnail.h
    src/util/ClipProbe.h
)

add_executable(ghostreplay ${SOURCES} ${HEADERS})

if(WIN32)
    target_sources(ghostreplay PRIVATE resources/windows/ghostreplay.rc)
endif()

# ── Include directories ───────────────────────────────────────

target_include_directories(ghostreplay PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(ghostreplay PRIVATE
    ${FFMPEG_LIBRARIES}
    ${D3D11_LIB}
    ${DXGI_LIB}
    ${DXGUID_LIB}
    ${SHELL32_LIB}
    ${DWMAPI_LIB}
    runtimeobject.lib
    ole32.lib
    psapi.lib
    nlohmann_json::nlohmann_json
)

target_compile_definitions(ghostreplay PRIVATE GHOST_REPLAY_VERSION="${PROJECT_VERSION}")

if(Qt6_FOUND)
    target_sources(ghostreplay PRIVATE
        ${QT_UI_SOURCES}
        ${QT_UI_HEADERS}
        resources/resources.qrc
        resources/qml.qrc
    )
    target_link_libraries(ghostreplay PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickDialogs2
        Qt6::Multimedia
    )
    target_compile_definitions(ghostreplay PRIVATE HAS_QT6)

    if(WIN32)
        set_target_properties(ghostreplay PROPERTIES WIN32_EXECUTABLE TRUE)
        if(MSVC)
            target_link_options(ghostreplay PRIVATE /SUBSYSTEM:WINDOWS)
        endif()
    endif()

    # Run windeployqt to copy Qt DLLs to the output directory
    find_program(WINDEPLOYQT_EXE windeployqt
        HINTS "${QT6_DIR}/bin" "${QT6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET ghostreplay POST_BUILD
            COMMAND ${WINDEPLOYQT_EXE} $<TARGET_FILE:ghostreplay>
                --no-translations --no-system-d3d-compiler --no-opengl-sw
                --qmldir "${CMAKE_SOURCE_DIR}/resources/qml"
            COMMENT "Deploying Qt DLLs via windeployqt")
    else()
        message(WARNING "windeployqt not found — Qt DLLs must be deployed manually")
    endif()
endif()

# ── Compiler flags ────────────────────────────────────────────

if(MSVC)
    target_compile_options(ghostreplay PRIVATE /W4 /utf-8)
    target_compile_definitions(ghostreplay PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# ── Copy FFmpeg DLLs to output ─────────────────────────────────

foreach(_comp ${FFMPEG_COMPONENTS})
    get_filename_component(_lib_dir "${FFMPEG_${_comp}_LIBRARY}" DIRECTORY)
    file(GLOB _dll
        "${FFMPEG_DIR}/bin/${_comp}*.dll"
        "${_lib_dir}/${_comp}*.dll"
        "${FFMPEG_DIR}/${_comp}*.dll")
    if(_dll)
        add_custom_command(TARGET ghostreplay POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_dll} $<TARGET_FILE_DIR:ghostreplay>
            COMMENT "Copying ${_comp} DLL")
    else()
        message(WARNING "FFmpeg runtime DLL for ${_comp} was not found under FFMPEG_DIR; packaging requires shared runtime DLLs")
    endif()
endforeach()

find_program(FFMPEG_EXE
    NAMES ffmpeg.exe ffmpeg
    PATHS "${FFMPEG_DIR}/bin" "${FFMPEG_DIR}"
    NO_DEFAULT_PATH)
find_program(FFPROBE_EXE
    NAMES ffprobe.exe ffprobe
    PATHS "${FFMPEG_DIR}/bin" "${FFMPEG_DIR}"
    NO_DEFAULT_PATH)

if(FFMPEG_EXE)
    add_custom_command(TARGET ghostreplay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFMPEG_EXE} $<TARGET_FILE_DIR:ghostreplay>
        COMMENT "Copying ffmpeg executable")
endif()

if(FFPROBE_EXE)
    add_custom_command(TARGET ghostreplay POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FFPROBE_EXE} $<TARGET_FILE_DIR:ghostreplay>
        COMMENT "Copying ffprobe executable")
endif()

# ── Tests (optional) ──────────────────────────────────────────

add_custom_target(package_portable
    COMMAND ${CMAKE_COMMAND}
        -DRUNTIME_DIR=$<TARGET_FILE_DIR:ghostreplay>
        -DDIST_DIR=${CMAKE_SOURCE_DIR}/dist
        -DVERSION=${PROJECT_VERSION}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/cmake/PackagePortable.cmake
    DEPENDS ghostreplay
    COMMENT "Staging Ghost Replay portable beta package"
    VERBATIM)

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 not found - fetching v3.6.0")
        include(FetchContent)
        set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
        set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.6.0
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
    add_executable(ghostreplay_tests ${TEST_SOURCES})
    target_sources(ghostreplay_tests PRIVATE src/encode/EncoderSelector.cpp)
    target_sources(ghostreplay_tests PRIVATE src/game/GameDetector.cpp)
    target_sources(ghostreplay_tests PRIVATE src/util/Config.cpp src/util/Logger.cpp)
    target_link_libraries(ghostreplay_tests PRIVATE Catch2::Catch2WithMain)
    target_link_libraries(ghostreplay_tests PRIVATE nlohmann_json::nlohmann_json)
    target_include_directories(ghostreplay_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_definitions(ghostreplay_tests PRIVATE
        GHOST_REPLAY_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    if(WIN32)
        target_link_libraries(ghostreplay_tests PRIVATE psapi.lib ${DWMAPI_LIB})
    endif()
    if(Qt6_FOUND)
        target_sources(ghostreplay_tests PRIVATE src/util/FileActions.cpp)
        target_sources(ghostreplay_tests PRIVATE src/util/ReleaseHealth.cpp src/util/ClipProbe.cpp)
        target_link_libraries(ghostreplay_tests PRIVATE Qt6::Core Qt6::Gui ${DXGI_LIB} ${SHELL32_LIB})
        target_compile_definitions(ghostreplay_tests PRIVATE HAS_QT6 GHOST_REPLAY_VERSION="${PROJECT_VERSION}")
    endif()
    include(Catch)
    catch_discover_tests(ghostreplay_tests)
endif()
