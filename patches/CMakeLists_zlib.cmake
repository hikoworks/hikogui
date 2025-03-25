# This is a replacement for the CMakeLists.txt file that is used to build zlib.
# FetchContent will patch/replace the CMakeLists.txt file with this one.

cmake_minimum_required(VERSION 2.4.4...3.15.0)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

set(VERSION "1.3.1.1")

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
#include(CheckCSourceCompiles)
enable_testing()

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)
check_type_size(off64_t HAVE_OFF64_T)
check_function_exists(fseeko HAVE_FSEEKO)
check_include_file(unistd.h HAVE_UNISTD_H)

# vcpkg uses capital letters for targets like ZLIB and PNG.
add_library(ZLIB)
add_library(ZLIB::ZLIB ALIAS ZLIB)
# The library name of course still uses lowercase.
set_target_properties(ZLIB PROPERTIES OUTPUT_NAME "zlib")
set_target_properties(ZLIB PROPERTIES DEBUG_POSTFIX "d")

#
# Check to see if we have large file support
#
if(HAVE_OFF64_T)
    target_compile_definitions(ZLIB PUBLIC "_LARGEFILE64_SOURCE=1")
endif()

#
# Check for fseeko
#
if(NOT HAVE_FSEEKO)
    target_compile_definitions(ZLIB PUBLIC "NO_FSEEKO")
endif()

#
# Check for unistd.h
#
if(HAVE_UNISTD_H)
    target_compile_definitions(ZLIB PUBLIC "HAVE_UNISTD_H")
endif()

if(WIN32)
    target_compile_definitions(ZLIB PUBLIC "_CRT_SECURE_NO_DEPRECATE")
    target_compile_definitions(ZLIB PUBLIC "_CRT_NONSTDC_NO_DEPRECATE")
endif()

option(RENAME_ZCONF "Rename the zconf when building out of source" ON)
if(NOT "${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}" AND "${RENAME_ZCONF}")
    # If we're doing an out of source build and the user has a zconf.h
    # in their source tree...
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
        message(STATUS "Renaming")
        message(STATUS "    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h")
        message(STATUS "to 'zconf.h.included' because this file is included with zlib")
        message(STATUS "but CMake generates it automatically in the build directory.")
        file(RENAME "${CMAKE_CURRENT_SOURCE_DIR}/zconf.h" "${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.included")
  endif()
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein"
	"${CMAKE_CURRENT_BINARY_DIR}/zconf.h" @ONLY)

#============================================================================
# zlib
#============================================================================

target_sources(ZLIB PUBLIC FILE_SET zlib_generated_public_include_files TYPE HEADERS BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}" FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
)

target_sources(ZLIB PUBLIC FILE_SET zlib_public_include_files TYPE HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h
)

target_sources(ZLIB PRIVATE FILE_SET zlib_private_include_files TYPE HEADERS BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/crc32.h
    ${CMAKE_CURRENT_SOURCE_DIR}/deflate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gzguts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inffast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inffixed.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inflate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inftrees.h
    ${CMAKE_CURRENT_SOURCE_DIR}/trees.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zutil.h
)

target_sources(ZLIB PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/adler32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crc32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deflate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzclose.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzread.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzwrite.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inflate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/infback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inffast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inftrees.c
    ${CMAKE_CURRENT_SOURCE_DIR}/trees.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uncompr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/zutil.c
)


# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})


#target_include_directories(zlib PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
#set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
#set_target_properties(zlib PROPERTIES SOVERSION 1)

if(NOT CYGWIN)
    # This property causes shared libraries on Linux to have the full version
    # encoded into their final filename.  We disable this on Cygwin because
    # it causes cygz-${ZLIB_FULL_VERSION}.dll to be created when cygz.dll
    # seems to be the default.
    #
    # This has no effect with MSVC, on that platform the version info for
    # the DLL comes from the resource file win32/zlib1.rc
    set_target_properties(ZLIB PROPERTIES VERSION ${ZLIB_FULL_VERSION})
endif()

if(UNIX)
    # On unix-like platforms the library is almost always called libz
    set_target_properties(ZLIB PROPERTIES OUTPUT_NAME z)
    if(NOT APPLE AND NOT(CMAKE_SYSTEM_NAME STREQUAL AIX))
        set_target_properties(ZLIB PROPERTIES LINK_FLAGS "-Wl,--version-script,\"${CMAKE_CURRENT_SOURCE_DIR}/zlib.map\"")
    endif()
elseif(BUILD_SHARED_LIBS AND WIN32)
    # Creates zlib1.dll when building shared library version
    set_target_properties(ZLIB PROPERTIES SUFFIX "1.dll")
endif()

set(INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/zlib/CMake")

install(TARGETS ZLIB EXPORT ZLIB
    FILE_SET zlib_public_include_files
    FILE_SET zlib_generated_public_include_files
    FILE_SET zlib_private_include_files
    RUNTIME
    ARCHIVE
    LIBRARY
)

#install(
#    EXPORT ZLIB
#    DESTINATION "${INSTALL_CMAKEDIR}"
#    NAMESPACE ZLIB::
#    FILE "zlibTargets.cmake"
#)
