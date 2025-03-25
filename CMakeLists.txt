# Copyright Jens A. Koch 2021-2023.
# Copyright Take Vos 2019-2022.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.25)

#-------------------------------------------------------------------
# Configure early CMAKE paths
#-------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

#-------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------

option(BUILD_SHARED_LIBS    "Build shared libraries"                             OFF)
option(HI_ENABLE_MODULES    "Compile using C++20 modules"                        OFF)
option(BUILD_EXAMPLES       "Compile the example executables"                    ON)
set(HI_ARCHITECTURE "" CACHE STRING "An architecture name used to set the CXXFLAGS")

if(HI_ENABLE_MODULES)
    include(SetupCxxModules)
endif()
include(SetupVcpkg)
include(SetupCompiler)

#-------------------------------------------------------------------
# Project
#-------------------------------------------------------------------

if(APPLE)
    set(LANGUAGES CXX OBJCXX)
else()
    set(LANGUAGES CXX)
endif()

# vcpkg.json is the primary source for version data
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json VCPKG_JSON_STRING)
string(JSON HI_LIB_NAME     GET ${VCPKG_JSON_STRING} "name")
string(JSON HI_LIB_VERSION  GET ${VCPKG_JSON_STRING} "version")
string(JSON HI_LIB_LICENSE  GET ${VCPKG_JSON_STRING} "license")
string(JSON HI_LIB_DESC     GET ${VCPKG_JSON_STRING} "description")
string(JSON HI_LIB_HOMEPAGE GET ${VCPKG_JSON_STRING} "homepage")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMake/packaging/package.json.in" "package.json" @ONLY)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/package.json PACKAGE_JSON_STRING)
string(JSON HI_LIB_DISPLAY_NAME GET ${PACKAGE_JSON_STRING} "display-name")
string(JSON HI_LIB_VENDOR GET ${PACKAGE_JSON_STRING} "vendor")

project(${HI_LIB_NAME} VERSION ${HI_LIB_VERSION} LANGUAGES ${LANGUAGES})

#-------------------------------------------------------------------
# Setup CMake Includes
#-------------------------------------------------------------------

include(FeatureSummary)
include(ShowBuildTargetProperties)
include(FetchContent)
include(SetupDoxygen)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(TargetResources)

# disable adding CTest build targets like "NightlyStart" (before ctest include)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ctest/CTestCustom.cmake.in" "CTestCustom.cmake" @ONLY)
include(CTest)

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

if(BUILD_TESTING)
    FetchContent_Declare(hikotest GIT_REPOSITORY https://github.com/hikogui/hikotest.git GIT_TAG main)
    FetchContent_MakeAvailable(hikotest)
endif()

if(VCPKG_TOOLCHAIN)
    find_package(ms-gsl)
else()
    FetchContent_Declare(GSL
        GIT_REPOSITORY https://github.com/microsoft/GSL
        GIT_TAG v4.0.0)
    FetchContent_MakeAvailable(GSL)
endif()

if(VCPKG_TOOLCHAIN)
    find_package(ZLIB)
else()
    FetchContent_Declare(ZLIB
        GIT_REPOSITORY https://github.com/madler/zlib
        GIT_TAG v1.3.1
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/patches/CMakeLists_zlib.cmake" "CMakeLists.txt"
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(ZLIB)

    # The ZLIB_ROOT and ZLIB_LIBRARY are set by find_package(zlib) when using
    # vcpkg.
    set(ZLIB_ROOT "${zlib_SOURCE_DIR}")
    set(ZLIB_LIBRARY "zlib")
endif()

if(VCPKG_TOOLCHAIN)
    find_package(PNG)
else()
    FetchContent_Declare(PNG
        URL https://sourceforge.net/projects/libpng/files/libpng-1.6.44.tar.gz
        URL_HASH SHA256=8c25a7792099a0089fa1cc76c94260d0bb3f1ec52b93671b572f8bb61577b732
#        PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/patches/CMakeLists_png.cmake" "CMakeLists.txt"
        EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(PNG)
    add_library(PNG::PNG ALIAS png_static)
endif()

#
# Vulkan SDK Headers
#
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR
        "Please set the VULKAN_SDK environment variable to the directory in which the SDK is installed.\n"
        "It looks like C:\\VulkanSDK\\1.0.0.0, but with the version number being the version that was installed.\n"
        "set VULKAN_SDK=\"C:\\VulkanSDK\\1.0.0.0\"\n")
endif()
find_package(Vulkan REQUIRED)

#
# Vulkan Memory Allocator - installed as part of Vulkan SDK headers
#
if(NOT EXISTS "${Vulkan_INCLUDE_DIRS}/vma")
    message(FATAL_ERROR "Please make sure to include VMA (Vulkan Memory Allocator) with the installation of VulkanSDK.\n")
endif()

#-------------------------------------------------------------------
# Handle architecture.
#-------------------------------------------------------------------
if(NOT HI_ARCHITECTURE STREQUAL "")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        if(HI_ARCHITECTURE STREQUAL "none")
            set(ARCHITECTURE_ID "none")
            add_compile_definitions(HI_GENERIC)
        elseif(HI_ARCHITECTURE STREQUAL "x86-64")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-arch:AVX")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v2")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-arch:AVX")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v3")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-arch:AVX2")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v4")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-arch:AVX512")
        else()
            message(FATAL_ERROR "Unknown HI_ARCHITECTURE=${HI_ARCHITECTURE} for MSVC frontend.")
        endif()

    elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        if(HI_ARCHITECTURE STREQUAL "none")
            set(ARCHITECTURE_ID "none")
            add_compile_definitions(HI_GENERIC)
        elseif(HI_ARCHITECTURE STREQUAL "x86-64")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-march=x86-64")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v2")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-march=x86-64-v2")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v3")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-march=x86-64-v3")
        elseif(HI_ARCHITECTURE STREQUAL "x86-64-v4")
            set(ARCHITECTURE_ID "x86")
            add_compile_options("-march=x86-64-v4")
        else()
            message(FATAL_ERROR "Unknown HI_ARCHITECTURE=${HI_ARCHITECTURE} for MSVC frontend.")
        endif()

    else()
        message(FATAL_ERROR "Unknown compiler frontend ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}.")
    endif()
endif()

#-------------------------------------------------------------------
# Build Target: hikogui                                     (library)
#-------------------------------------------------------------------
if(HI_ENABLE_MODULES)
    set(HI_SCOPE "PUBLIC")
    add_library(hikogui)
else()
    set(HI_SCOPE "INTERFACE")
    add_library(hikogui INTERFACE)
endif()
add_library(hikogui::hikogui ALIAS hikogui)

set(HIKOGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui")
if(HI_ENABLE_MODULES)
    include(CMakeLists_modules.cmake)
else()
    include(CMakeLists_includes.cmake)
endif()
include(CMakeLists_resources.cmake)
include(CMakeLists_shaders.cmake)

target_compile_features(hikogui ${HI_SCOPE} cxx_std_23)
set_target_properties(hikogui PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(hikogui PROPERTIES VERSION ${HIKOGUI_PROJECT_VERSION_SHORT} SOVERSION ${HIKOGUI_MAJOR_VERSION})
set_target_properties(hikogui PROPERTIES FRAMEWORK TRUE)
set_target_properties(hikogui PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui)   

if(APPLE)
    target_link_libraries(hikogui ${HI_SCOPE} "-framework Foundation -framework AppKit")
endif()
if(WIN32)
	target_link_libraries(hikogui ${HI_SCOPE} "ws2_32")
	target_link_libraries(hikogui ${HI_SCOPE} "DXGI")
	target_link_libraries(hikogui ${HI_SCOPE} "bcrypt")
	target_link_libraries(hikogui ${HI_SCOPE} "winmm")
	target_link_libraries(hikogui ${HI_SCOPE} "dwmapi")
endif()

# Add the Vulkan libraries.
target_link_libraries(hikogui ${HI_SCOPE} ${Vulkan_LIBRARIES})
target_include_directories(hikogui ${HI_SCOPE} ${Vulkan_INCLUDE_DIRS})

target_link_libraries(hikogui ${HI_SCOPE} Microsoft.GSL::GSL)
target_link_libraries(hikogui ${HI_SCOPE} ZLIB::ZLIB)
target_link_libraries(hikogui ${HI_SCOPE} PNG::PNG)

# HikoGUI only uses Vulkan-1.2; we can limit VMA to that version as well.
# This solves a common issue with: "Cant find entrypoint for vkGetDeviceBufferMemoryRequirements procedure"
target_compile_definitions(hikogui ${HI_SCOPE} "VMA_VULKAN_VERSION=1002000")
if (WIN32)
    # Add vulkan win32 surface support.
    target_compile_definitions(hikogui ${HI_SCOPE} "VK_USE_PLATFORM_WIN32_KHR")

    # vulkan.h will include the windows headers, so we must define all sorts of stuff on the command line.
    target_compile_definitions(hikogui ${HI_SCOPE} "UNICODE")
    target_compile_definitions(hikogui ${HI_SCOPE} "_UNICODE")
    target_compile_definitions(hikogui ${HI_SCOPE} "_CRT_SECURE_NO_WARNINGS")
    target_compile_definitions(hikogui ${HI_SCOPE} "NOMINMAX")
    target_compile_definitions(hikogui ${HI_SCOPE} "WIN32_LEAN_AND_MEAN")
    target_compile_definitions(hikogui ${HI_SCOPE} "WIN32_NO_STATUS")

    # Minimum Windows 10.
    target_compile_definitions(hikogui ${HI_SCOPE} "WINVER=0x0a00")
    target_compile_definitions(hikogui ${HI_SCOPE} "_WIN32_WINNT=0x0a00")

elseif (APPLE)
    # Add vulkan apple's Metal surface support.
    target_compile_definitions(hikogui ${HI_SCOPE} "VK_USE_PLATFORM_METAL_EXT")
endif()

if(HI_ENABLE_MODULES)
    target_include_directories(hikogui ${HI_SCOPE}
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mod>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/mod/hikogui/metadata>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mod/hikogui/metadata>"
        "$<INSTALL_INTERFACE:include>")
else()
    target_include_directories(hikogui ${HI_SCOPE}
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/hikogui/metadata>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/metadata>"
        "$<INSTALL_INTERFACE:include>")
endif()

#
# We will support the following CPUs:
#   * Intel Ivy Bridge from 2012, still used by Mac Pro sold in 2019.
#   * AMD Steamroller from 2014
#
if (${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    # Set the execution character encoding to UTF-8
    if (MSVC)
        target_compile_options(hikogui ${HI_SCOPE} -utf-8)
    else()
        target_compile_options(hikogui ${HI_SCOPE} -fexec-charset=UTF-8)
        target_compile_options(hikogui ${HI_SCOPE} -finput-charset=UTF-8)
    endif()

    # Clang will complain about __builtin_assume even on constexpr functions and operators,
    # this warning is absolutely useless.
    target_compile_options(hikogui ${HI_SCOPE} -Wno-assume)

    # Clang knows [[no_unique_address]] but since win32-ABI does not
    # support [[no_unique_address] clang pretends it does not know the attribute.
    target_compile_options(hikogui ${HI_SCOPE} -Wno-unknown-attributes)

    # Clang will complain about calling hidden-friend-template-functions.
    target_compile_options(hikogui ${HI_SCOPE} -Wno-unknown-warning-option -Wno-c++20-extensions -Wno-c++2a-extensions)

    # vk_mem_alloc.h seems to turn on nullability completeness checks on clang.
    # It seems this check should only be used for interopability with swift
    target_compile_options(hikogui ${HI_SCOPE} -Wno-nullability-completeness)

    # warning: template argument for template type parameter must be a type; omitted 'typename' is a Microsoft extension
    # Very likely this is because clang does not support C++20 fully "down with typename".
    target_compile_options(hikogui ${HI_SCOPE} -Wno-microsoft-template)

    if (MSVC)
        # Options specific for clang-cl.

        # The hikogui generic tokenizer uses large constexpr tables.
        target_compile_options(hikogui ${HI_SCOPE} "SHELL:-Xclang -fconstexpr-steps -Xclang 100000000")

    else()
        # Normal clang.

        # The hikogui generic tokenizer uses large constexpr tables.
        target_compile_options(hikogui ${HI_SCOPE} -fconstexpr-steps=100000000)

        # Tune according to a more modern CPU.
        target_compile_options(hikogui ${HI_SCOPE} -mtune=skylake)
    endif()

elseif (MSVC)
    # Turn on a lot of warnings by default.
    target_compile_options(hikogui ${HI_SCOPE} -W4)

    # Set the compiler to C++20 standard conforming as much as possible.
    target_compile_options(hikogui ${HI_SCOPE} -permissive-)

    # suppress msbuild startup banner
    target_compile_options(hikogui ${HI_SCOPE} -nologo)

    # By default MSVC reports a incorrect version in __cplusplus
    # This option will make it work correctly.
    target_compile_options(hikogui ${HI_SCOPE} -Zc:__cplusplus)

    # Use the C++20 compliant preprocessor, which include __VA_OPT__()
    # This causes warning "C5105: macro expansion producing 'defined' has undefined behavior" in winbase.h
    # which is supposed to be fixed in a newer version of the SDK.
    target_compile_options(hikogui ${HI_SCOPE} -Zc:preprocessor)
    #target_compile_options(hikogui ${HI_SCOPE} -wd5105)

    # Set the execution and source character encoding to UTF-8
    target_compile_options(hikogui ${HI_SCOPE} -utf-8)

    # The hikogui generic tokenizer uses large constexpr tables.
    target_compile_options(hikogui ${HI_SCOPE} "-constexpr:steps100000000")

    # The library is too big.
    target_compile_options(hikogui ${HI_SCOPE} -bigobj)

    # Configure the headers of external libraries and only set warning level to W3 for those.
    target_compile_options(hikogui ${HI_SCOPE} -external:W0)
    target_compile_options(hikogui ${HI_SCOPE} "-external:IC:\\Program Files\\Microsoft Visual Studio")
    target_compile_options(hikogui ${HI_SCOPE} "-external:IC:\\Program Files (x86)\\Windows Kits")
    target_compile_options(hikogui ${HI_SCOPE} "-external:I${Vulkan_INCLUDE_DIRS}")

    # C4061: enumerator '...' in a switch of enum '...' is not explicitly handled by a case label.
    # This warning happens even if you added a default label.
    target_compile_options(hikogui ${HI_SCOPE} -wd4061)

    # C4068: unknown pragma.
    # Ignore unknown pragmas, needed for other compilers.
    target_compile_options(hikogui ${HI_SCOPE} -wd4068)

    # C4324: structure was padded due to alignment specifier.
    # Of course it is, that is what the alignment specifier is used for??????
    target_compile_options(hikogui ${HI_SCOPE} -wd4324)

    # C4100: unreferenced formal parameter.
    # This warning is in the way when you want to document a parameter of a virtual function
    # with an implementation that does not use the parameter.
    target_compile_options(hikogui ${HI_SCOPE} -wd4100)

    # C4127: conditional expression is constant.
    # False positive with expressions that include template parameters.
    target_compile_options(hikogui ${HI_SCOPE} -wd4127)

    # C6326: Potential comparison of a constant with another constant.
    # False positive in constexpr context.
    target_compile_options(hikogui ${HI_SCOPE} -wd6326)

    # C6239: (<non-zero constant> && <expression>) always evaluates to the result of <expression>. Did you intend to use the bitwise-and operator?"
    # False positive when using logical operators in constexpr context.
    target_compile_options(hikogui ${HI_SCOPE} -wd6239)

    # C6262: Function uses 'x' bytes of stack: exceeds /analyze:stacksize 'x'. Consider moving some data to heap.
    # False positives when returning data from a function in constexpr context.
    target_compile_options(hikogui ${HI_SCOPE} -wd6262)

    # C4505: unreferenced local function has been removed.
    # False positive when calling a local function from within a "if constexpr" block.
    target_compile_options(hikogui ${HI_SCOPE} -wd4505)

    # C4648: standard attribute 'no_unique_address' is ignored
    target_compile_options(hikogui ${HI_SCOPE} -wd4648)

    # C4744: 'x' has different type in 'foo_impl.cpp' and 'bar_impl.cpp':
    #        'struct (8 bytes)' and '__declspec(align(8)) struct (8 bytes)' [foobar.vcxproj]
    # Bug in linker where atomic variables defined in a header file and used in two _impl.cpp files be
    # interpreted as different types. Microsoft knows about the bug for many years.
    target_link_options(hikogui ${HI_SCOPE} "/ignore:4744")

    if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
        # Enable Security Development Lifecycle checks.
        # This includes run-time checks, don't include this in the Release type
        # Do include it with ReleaseWithDebug.

        # -sdl forces deprecated warnings as errors so also not useful during development.
        #target_compile_options(hikogui ${HI_SCOPE} -sdl)

        # Just my code enables the VS debugger to step over system, framework, library, and other non-user calls.
        target_compile_options(hikogui ${HI_SCOPE} -JMC)
    endif()

    if("$ENV{CXXFLAGS}" MATCHES "[/-]analyze")
        #target_compile_options(hikogui ${HI_SCOPE} -analyze:ruleset ${CMAKE_CURRENT_SOURCE_DIR}/AllRules.ruleset)

        # The Core Guidelines checker, not so useful for real programs.
        message(NOTICE "The following environment variables need to be set for the core guidelines checker to work:")
        message(NOTICE "    esp.extensions=cppcorecheck.dll")
        message(NOTICE "")

        # Exclude external headers from analysis.
        target_compile_options(hikogui ${HI_SCOPE} -analyze:external-)

        target_compile_options(hikogui ${HI_SCOPE} -analyze:pluginEspxEngine.dll)

        # C26440: Function '...' can be declared 'noexcept' (f.6).
        # It requires lambda function to also be declared 'noexcept' that makes the code messy.
        target_compile_options(hikogui ${HI_SCOPE} -wd26440)

        # C26446: Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
        # This rule makes applications slow.
        target_compile_options(hikogui ${HI_SCOPE} -wd26446)

        # C26447: The function is declared 'noexcept' but calls function '...' which may throw exceptions (f.6).
        # Since c++20 std::terminate will be called on throw.
        target_compile_options(hikogui ${HI_SCOPE} -wd26447)

        # C26481: Don't use pointer arithmetic. Use span instead (bounds.1).
        # False positive everywhere std::format() is used.
        target_compile_options(hikogui ${HI_SCOPE} -wd26481)

        # C26482: Only index into arrays using constant expressions (bounds.2).
        # This rule makes it so you can not use a variable to index into an array.
        target_compile_options(hikogui ${HI_SCOPE} -wd26482)

        # C26821: For '...', consider using gsl::span instead of std::span to guarantee runtime bounds safety (gsl.view).
        # This rule makes applications slow.
        target_compile_options(hikogui ${HI_SCOPE} -wd26821)

        # C28020: The expression '0<=_Param_(1)&&_Param_(1)<=256-1' is not true at this call.
        # False positives in base_n, where the index is static_cast<uint8_t>().
        # The bug (since 2020) in the compiler seem to have been triggered in unrelated code.
        # https://developercommunity.visualstudio.com/t/c28020-false-positives/923103
        target_compile_options(hikogui ${HI_SCOPE} -wd28020)

        # C26409: Avoid calling new and delete explicitly, use std::make_unique<T> instead (r.11)
        # Can't exclude unit-test and google-test uses new and delete in the TEST() macro.
        target_compile_options(hikogui ${HI_SCOPE} -wd26409)

        # C26426: Global initializer calls a non-constexpr function (i.22)
        # Can't exclude unit-test and google-test does this a lot.
        target_compile_options(hikogui ${HI_SCOPE} -wd26426)

        # C26455: Default constructor should not throw. Declare it 'noexcept' (f.6)
        # Can't exclude unit-test and google-test uses throwing default constructors in the TEST() macro.
        target_compile_options(hikogui ${HI_SCOPE} -wd26455)

        # Turn off every warning fix one at a time:
        #target_compile_options(hikogui ${HI_SCOPE} -wd26472)
        #target_compile_options(hikogui ${HI_SCOPE} -wd26473)
        #target_compile_options(hikogui ${HI_SCOPE} -wd26474)
        target_compile_options(hikogui ${HI_SCOPE} -wd26475)
        target_compile_options(hikogui ${HI_SCOPE} -wd26476)
        target_compile_options(hikogui ${HI_SCOPE} -wd26478)
        target_compile_options(hikogui ${HI_SCOPE} -wd26485)
        target_compile_options(hikogui ${HI_SCOPE} -wd26490)
        target_compile_options(hikogui ${HI_SCOPE} -wd26491)
        target_compile_options(hikogui ${HI_SCOPE} -wd26492)
        target_compile_options(hikogui ${HI_SCOPE} -wd26493)
        target_compile_options(hikogui ${HI_SCOPE} -wd26494)
        target_compile_options(hikogui ${HI_SCOPE} -wd26495)
        target_compile_options(hikogui ${HI_SCOPE} -wd26496)
        target_compile_options(hikogui ${HI_SCOPE} -wd26497)
        target_compile_options(hikogui ${HI_SCOPE} -wd26498)
        target_compile_options(hikogui ${HI_SCOPE} -wd26800)
        target_compile_options(hikogui ${HI_SCOPE} -wd26813)
        target_compile_options(hikogui ${HI_SCOPE} -wd26814)
        target_compile_options(hikogui ${HI_SCOPE} -wd26817)
        target_compile_options(hikogui ${HI_SCOPE} -wd26818)
        target_compile_options(hikogui ${HI_SCOPE} -wd26820)
        target_compile_options(hikogui ${HI_SCOPE} -wd26830)
        target_compile_options(hikogui ${HI_SCOPE} -wd26432)
        target_compile_options(hikogui ${HI_SCOPE} -wd26451)

    endif()
endif()

#-------------------------------------------------------------------
# Build Target: 
#-------------------------------------------------------------------
# An INTERFACE library does not have a build target which means it
# can not be installed. Therefor we create a dummy target which
# will make sure the INTERFACE library can be installed.
add_custom_target(hikogui_dummy ALL)
add_dependencies(hikogui_dummy hikogui)

#-------------------------------------------------------------------
# Build Target: hikogui_tests
#-------------------------------------------------------------------

if(BUILD_TESTING)
    include(CMakeLists_tests.cmake)
endif()

#-------------------------------------------------------------------
# Build examples
#-------------------------------------------------------------------
if(BUILD_EXAMPLES)
    add_custom_target(examples)
    add_subdirectory(examples/codec)
    add_subdirectory(examples/custom_widgets)
    add_subdirectory(examples/hikogui_demo)
    add_subdirectory(examples/vulkan/triangle)
    add_subdirectory(examples/widgets)
endif()

#-------------------------------------------------------------------
# Display Compiler and Linker properties of Build Targets
#-------------------------------------------------------------------

show_build_target_properties(hikogui)

feature_summary(WHAT PACKAGES_FOUND PACKAGES_NOT_FOUND ENABLED_FEATURES DISABLED_FEATURES)


#-------------------------------------------------------------------
# Installation Rules: hikogui                               (library)
#-------------------------------------------------------------------
set(INSTALL_RESOURCEDIR "${CMAKE_INSTALL_DATADIR}/hikogui/resources")
set(INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/hikogui/CMake")
set(INSTALL_MODULEDIR "module")


install(TARGETS hikogui GSL ZLIB png_static EXPORT hikogui
    FILE_SET zlib_public_include_files
    FILE_SET zlib_generated_public_include_files
    FILE_SET hikogui_include_files
    FILE_SET hikogui_generated_include_files
    FILE_SET CXX_MODULES DESTINATION "${INSTALL_MODULEDIR}"
    RUNTIME
    ARCHIVE
    LIBRARY
    RESOURCE DESTINATION "${INSTALL_RESOURCEDIR}"
)

install(
    EXPORT hikogui
    DESTINATION "${INSTALL_CMAKEDIR}"
    NAMESPACE hikogui::
    FILE "hikoguiTargets.cmake"
)

get_target_resources(hikogui RESOURCE_FILES)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/packaging/hikoguiResources.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiResources.cmake"
    @ONLY
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/packaging/hikoguiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiConfig.cmake"
    INSTALL_DESTINATION "${INSTALL_CMAKEDIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hikoguiResources.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/TargetResources.cmake"
    DESTINATION "${INSTALL_CMAKEDIR}"
)
