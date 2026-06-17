foreach(_required_var IN ITEMS RUNTIME_DIR DIST_DIR VERSION)
    if(NOT DEFINED ${_required_var})
        message(FATAL_ERROR "${_required_var} is required")
    endif()
    if("${${_required_var}}" STREQUAL "")
        message(FATAL_ERROR "${_required_var} cannot be empty")
    endif()
endforeach()

if(NOT DEFINED SOURCE_DIR OR "${SOURCE_DIR}" STREQUAL "")
    set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
endif()

get_filename_component(RUNTIME_DIR "${RUNTIME_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(DIST_DIR "${DIST_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(SOURCE_DIR "${SOURCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

set(PACKAGE_DIR "${DIST_DIR}/GhostReplay-${VERSION}-win64-portable")
set(RELEASE_CONFIG "${SOURCE_DIR}/config/ghostreplay.example.json")
set(RELEASE_README "${SOURCE_DIR}/docs/README_EARLY_ACCESS.md")
set(RELEASE_NOTICES "${SOURCE_DIR}/docs/THIRD_PARTY_NOTICES.md")
set(RELEASE_NOTES "${SOURCE_DIR}/docs/RELEASE_NOTES.md")
set(LICENSE_FILE "${SOURCE_DIR}/LICENSE")
set(SECURITY_FILE "${SOURCE_DIR}/SECURITY.md")

foreach(_required_file IN ITEMS
        "${RELEASE_CONFIG}"
        "${RELEASE_README}"
        "${RELEASE_NOTICES}"
        "${RELEASE_NOTES}"
        "${LICENSE_FILE}"
        "${SECURITY_FILE}")
    if(NOT EXISTS "${_required_file}")
        message(FATAL_ERROR "Required release file is missing: ${_required_file}")
    endif()
endforeach()

function(assert_public_ffmpeg_safe FFMPEG_EXE_PATH)
    if(NOT EXISTS "${FFMPEG_EXE_PATH}")
        return()
    endif()

    execute_process(
        COMMAND "${FFMPEG_EXE_PATH}" -hide_banner -buildconf
        RESULT_VARIABLE _ffmpeg_buildconf_result
        OUTPUT_VARIABLE _ffmpeg_buildconf
        ERROR_VARIABLE _ffmpeg_buildconf_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)
    if(_ffmpeg_buildconf_result EQUAL 0)
        string(FIND "${_ffmpeg_buildconf}" "--enable-gpl" _has_gpl)
        string(FIND "${_ffmpeg_buildconf}" "--enable-libx264" _has_x264)
        string(FIND "${_ffmpeg_buildconf}" "--enable-libx265" _has_x265)
        if(NOT _has_gpl EQUAL -1 OR NOT _has_x264 EQUAL -1 OR NOT _has_x265 EQUAL -1)
            message(FATAL_ERROR
                "Packaged ffmpeg.exe is GPL/libx264/libx265-enabled. "
                "Use a non-GPL FFmpeg build before public release.")
        endif()
    else()
        message(WARNING "Could not inspect ffmpeg.exe build configuration: ${_ffmpeg_buildconf_error}")
    endif()
endfunction()

function(assert_ffmpeg_runtime_dlls RUNTIME_PATH)
    foreach(_component IN ITEMS avcodec avformat avutil swresample swscale)
        file(GLOB _component_dlls "${RUNTIME_PATH}/${_component}-*.dll")
        list(LENGTH _component_dlls _component_dll_count)
        if(_component_dll_count EQUAL 0)
            message(FATAL_ERROR
                "Packaged runtime is missing ${_component}.dll. "
                "Use a shared FFmpeg build with runtime DLLs and rebuild from a clean output directory.")
        endif()
        if(_component_dll_count GREATER 1)
            message(FATAL_ERROR
                "Packaged runtime has multiple ${_component} DLL versions: ${_component_dlls}. "
                "Clean the build output before creating a release package.")
        endif()
    endforeach()
endfunction()

file(REMOVE_RECURSE "${PACKAGE_DIR}")
assert_public_ffmpeg_safe("${RUNTIME_DIR}/ffmpeg.exe")
assert_ffmpeg_runtime_dlls("${RUNTIME_DIR}")

file(MAKE_DIRECTORY "${PACKAGE_DIR}")
file(COPY "${RUNTIME_DIR}/" DESTINATION "${PACKAGE_DIR}")

file(REMOVE "${PACKAGE_DIR}/ghostreplay.log")
file(REMOVE "${PACKAGE_DIR}/ghostreplay_tests.exe")
file(REMOVE "${PACKAGE_DIR}/AGENTS.md")
file(REMOVE_RECURSE "${PACKAGE_DIR}/clips")

file(COPY_FILE "${RELEASE_CONFIG}" "${PACKAGE_DIR}/ghostreplay.json")
file(COPY "${RELEASE_README}" DESTINATION "${PACKAGE_DIR}")
file(COPY "${RELEASE_NOTICES}" DESTINATION "${PACKAGE_DIR}")
file(COPY "${RELEASE_NOTES}" DESTINATION "${PACKAGE_DIR}")
file(COPY "${LICENSE_FILE}" DESTINATION "${PACKAGE_DIR}")
file(COPY "${SECURITY_FILE}" DESTINATION "${PACKAGE_DIR}")

assert_public_ffmpeg_safe("${PACKAGE_DIR}/ffmpeg.exe")
assert_ffmpeg_runtime_dlls("${PACKAGE_DIR}")

message(STATUS "Portable package staged at ${PACKAGE_DIR}")
