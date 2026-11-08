cmake_minimum_required(VERSION 3.24)

project(GrindstoneSwordFix VERSION 1.0 LANGUAGES C CXX)

# The version is set only on the project() line above. C++ reads it from the
# generated Version.h; the Python tools read it from this file.
foreach(part MAJOR MINOR PATCH)
    set(GSF_VERSION_${part} "${PROJECT_VERSION_${part}}")
    if(GSF_VERSION_${part} STREQUAL "")
        set(GSF_VERSION_${part} 0)
    endif()
endforeach()
configure_file(
    include/GrindstoneSwordFix/Version.h.in
    "${CMAKE_CURRENT_BINARY_DIR}/generated/GrindstoneSwordFix/Version.h"
    @ONLY
)

option(GSF_BUILD_TESTS "Build engine-independent tests" ON)

include(FetchContent)
set(BUILD_TESTS OFF CACHE BOOL "Build dependency tests" FORCE)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "Support Skyrim SE" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "Support Skyrim AE" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "Support Skyrim VR" FORCE)

# Same pinned CommonLibSSE-NG as Artisans of Skyrim (one DLL for SE and AE).
FetchContent_Declare(
    openvr
    URL https://codeload.github.com/ValveSoftware/openvr/zip/60eb187801956ad277f1cae6680e3a410ee0873b
    URL_HASH SHA256=51b4deef78c10f52e3e41d85bab51a87a55cd4d87b4d6c65c8952c2cbfafb9ae
    SOURCE_SUBDIR headers
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    CommonLibSSE
    URL https://codeload.github.com/alandtse/CommonLibSSE-NG/zip/refs/tags/v7.2.0
    URL_HASH SHA256=377b447c4923c6f7c615494a793ace8d7b443fb899d4e15dbf95ec2712155ce8
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(openvr CommonLibSSE)
target_include_directories(CommonLibSSE SYSTEM PUBLIC
    "$<BUILD_INTERFACE:${openvr_SOURCE_DIR}/headers>"
)
if(MSVC)
    # /pathmap only takes effect together with /experimental:deterministic.
    target_compile_options(CommonLibSSE PRIVATE
        /experimental:deterministic
        "/pathmap:${commonlibsse_SOURCE_DIR}=CommonLibSSE"
        "/pathmap:${CMAKE_BINARY_DIR}=Build"
        "/pathmap:${VCPKG_INSTALLED_DIR}=Dependencies"
    )
endif()

add_library(
    GrindstoneSwordFix
    SHARED
        include/GrindstoneSwordFix/PCH.h
        include/GrindstoneSwordFix/RuntimeCompatibility.h
        include/GrindstoneSwordFix/AnimObjectFix.h
        src/AnimObjectFix.cpp
        src/Plugin.cpp
)

target_compile_features(GrindstoneSwordFix PRIVATE cxx_std_23)
target_compile_definitions(GrindstoneSwordFix PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(GrindstoneSwordFix PRIVATE include "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(GrindstoneSwordFix PRIVATE CommonLibSSE::CommonLibSSE)
target_precompile_headers(GrindstoneSwordFix PRIVATE include/GrindstoneSwordFix/PCH.h)

if(MSVC)
    target_compile_options(
        GrindstoneSwordFix
        PRIVATE
            /permissive-
            /Zc:preprocessor
            /utf-8
            /W4
            /EHsc
            /MP
            /experimental:deterministic
            "/pathmap:${CMAKE_SOURCE_DIR}=."
            "/pathmap:${commonlibsse_SOURCE_DIR}=CommonLibSSE"
            "/pathmap:${CMAKE_BINARY_DIR}=Build"
            "/pathmap:${VCPKG_INSTALLED_DIR}=Dependencies"
    )
    target_link_options(GrindstoneSwordFix PRIVATE /INCREMENTAL:NO /DEBUG:NONE)
endif()

# Stage the built DLL in dist/Data, the input of tools/MakeInstallableZip.ps1.
add_custom_command(
    TARGET GrindstoneSwordFix POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:GrindstoneSwordFix>"
        "${CMAKE_CURRENT_SOURCE_DIR}/dist/Data/SKSE/Plugins/$<TARGET_FILE_NAME:GrindstoneSwordFix>"
)

if(GSF_BUILD_TESTS)
    enable_testing()
    add_executable(GrindstoneSwordFixRuntimeCompatibilityTests tests/RuntimeCompatibilityTests.cpp)
    target_compile_features(GrindstoneSwordFixRuntimeCompatibilityTests PRIVATE cxx_std_20)
    target_include_directories(GrindstoneSwordFixRuntimeCompatibilityTests PRIVATE include)
    target_compile_options(GrindstoneSwordFixRuntimeCompatibilityTests PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>"
    )
    add_test(NAME GrindstoneSwordFix.RuntimeCompatibility COMMAND GrindstoneSwordFixRuntimeCompatibilityTests)
endif()
