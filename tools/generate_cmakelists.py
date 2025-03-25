#!/usr/bin/python3

import sys
import os
import os.path

def get_header_files():
    header_files = []
    for dirpath, dirnames, filenames in os.walk("src"):
        for filename in filenames:
            path = os.path.join(dirpath, filename)
            if path.endswith(".hpp") or path.endswith(".h"):
                header_files.append(path)

    header_files.sort()
    return header_files

def get_test_files():
    test_files = []
    for dirpath, dirnames, filenames in os.walk("src"):
        for filename in filenames:
            path = os.path.join(dirpath, filename)
            if path.endswith("_tests.cpp"):
                test_files.append(path)

    test_files.sort()
    return test_files


def make_cmakelist_includes_text(header_files, suppressed_header_files):
    r  = "# This file was generated with tools/generate_cmakelists.sh\n"
    r += "\n"
    r += "if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)\n"
    r += "    # vcpkg does not allow absolute paths anywhere in the install directory.\n"
    r += "    # These directories are normally used to execute files in their build\n"
    r += "    # directory; which does not happen on a vcpkg install.\n"
    r += "    set(LIBRARY_SOURCE_DIR \"\")\n"
    r += "    set(LIBRARY_BUILD_DIR \"\")\n"
    r += "else()\n"
    r += "    set(LIBRARY_SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")\n"
    r += "    set(LIBRARY_BUILD_DIR \"${CMAKE_CURRENT_BINARY_DIR}\")\n"
    r += "endif()\n"
    r += "\n"
    r += "configure_file(\n"
    r += "    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/metadata/library_metadata.hpp.in\n"
    r += "    ${CMAKE_CURRENT_BINARY_DIR}/src/hikogui/metadata/library_metadata.hpp @ONLY)\n"
    r += "\n"
    r += "target_sources(hikogui INTERFACE FILE_SET hikogui_generated_include_files TYPE HEADERS BASE_DIRS \"${CMAKE_CURRENT_BINARY_DIR}/src/\" FILES\n"
    r += "    ${CMAKE_CURRENT_BINARY_DIR}/src/hikogui/metadata/library_metadata.hpp\n"
    r += ")\n"
    r += "\n"
    r += "target_sources(hikogui INTERFACE FILE_SET hikogui_include_files TYPE HEADERS BASE_DIRS \"${CMAKE_CURRENT_SOURCE_DIR}/src/\"  FILES\n"
    for header_file in header_files:
        if header_file.endswith("_win32.hpp") or header_file.endswith("_win32_impl.hpp") or header_file.endswith("_win32_intf.hpp"):
            r += "    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/%s>\n" % header_file.replace("\\", "/")

        if header_file.endswith("_posix.hpp") or header_file.endswith("_posix_impl.hpp") or header_file.endswith("_posix_intf.hpp"):
            r += "    $<$<PLATFORM_ID:Linux>:${CMAKE_CURRENT_SOURCE_DIR}/%s>\n" % header_file.replace("\\", "/")

        if header_file.endswith("_macos.hpp") or header_file.endswith("_macos_impl.hpp") or header_file.endswith("_macos_intf.hpp"):
            r += "    $<$<PLATFORM_ID:MacOS>:${CMAKE_CURRENT_SOURCE_DIR}/%s>\n" % header_file.replace("\\", "/")

        if header_file.endswith("_x86.hpp") or header_file.endswith("_x86_impl.hpp") or header_file.endswith("_x86_intf.hpp"):
            r += "    $<$<STREQUAL:${ARCHITECTURE_ID},x86>:${CMAKE_CURRENT_SOURCE_DIR}/%s>\n" % header_file.replace("\\", "/")

        if header_file.endswith("_generic.hpp") or header_file.endswith("_generic_impl.hpp") or header_file.endswith("_generic_iintf.hpp"):
            r += "    $<$<STREQUAL:${ARCHITECTURE_ID},none>:${CMAKE_CURRENT_SOURCE_DIR}/%s>\n" % header_file.replace("\\", "/")

        if header_file in suppressed_header_files:
            r += "    #%s\n" % header_file.replace("\\", "/")

        else:
            r += "    %s\n" % header_file.replace("\\", "/")

    r += ")\n"
    r += "\n"
    return r

def make_cmakelist_tests_text(test_files, suppressed_test_files):
    r  = "# This file was generated with tools/generate_cmakelists.sh\n"
    r += "\n"
    r += "add_executable(hikogui_htests)\n"
    r += "target_link_libraries(hikogui_htests PRIVATE hikotest hikogui)\n"
    r += "target_include_directories(hikogui_htests PRIVATE \"${CMAKE_CURRENT_BINARY_DIR}\")\n"
    r += "set_target_properties(hikogui_htests PROPERTIES DEBUG_POSTFIX \"-dbg\")\n"
    r += "set_target_properties(hikogui_htests PROPERTIES RELEASE_POSTFIX \"-rel\")\n"
    r += "set_target_properties(hikogui_htests PROPERTIES RELWITHDEBINFO_POSTFIX \"-rdi\")\n"
    r += "add_test(NAME hikogui_htests COMMAND hikogui_htests)\n"
    r += "\n"
    r += "if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL \"MSVC\")\n"
    r += "    set(ASAN_DLL \"${CMAKE_CXX_COMPILER}\")\n"
    r += "    if(CMAKE_BUILD_TYPE STREQUAL \"Debug\")\n"
    r += "        cmake_path(REPLACE_FILENAME ASAN_DLL \"clang_rt.asan_dbg_dynamic-x86_64.dll\")\n"
    r += "    else()\n"
    r += "        cmake_path(REPLACE_FILENAME ASAN_DLL \"clang_rt.asan_dynamic-x86_64.dll\")\n"
    r += "    endif()\n"
    r += "\n"
    r += "    add_custom_command(TARGET hikogui_htests PRE_BUILD\n"
    r += "    COMMAND ${CMAKE_COMMAND} -E copy \"${ASAN_DLL}\" $<TARGET_FILE_DIR:hikogui_htests>)\n"
    r += "\n"
    r += "    if(CMAKE_BUILD_TYPE STREQUAL \"Debug\")\n"
    r += "        target_compile_definitions(hikogui_htests PRIVATE \"_DISABLE_VECTOR_ANNOTATION\")\n"
    r += "        target_compile_definitions(hikogui_htests PRIVATE \"_DISABLE_STRING_ANNOTATION\")\n"
    r += "        target_compile_options(hikogui_htests PRIVATE -fsanitize=address)\n"
    r += "    endif()\n"
    r += "\n"
    r += "elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL \"GNU\")\n"
    r += "    if(CMAKE_BUILD_TYPE STREQUAL \"Release\" OR CMAKE_BUILD_TYPE STREQUAL \"RelWithDebInfo\")\n"
    r += "        set(ASAN_DLL \"${CMAKE_CXX_COMPILER}\")\n"
    r += "        if(CMAKE_BUILD_TYPE STREQUAL \"Debug\")\n"
    r += "            cmake_path(REPLACE_FILENAME ASAN_DLL \"clang_rt.asan_dbg_dynamic-x86_64.dll\")\n"
    r += "        else()\n"
    r += "            cmake_path(REPLACE_FILENAME ASAN_DLL \"clang_rt.asan_dynamic-x86_64.dll\")\n"
    r += "        endif()\n"
    r += "\n"
    r += "        add_custom_command(TARGET hikogui_htests PRE_BUILD\n"
    r += "        COMMAND ${CMAKE_COMMAND} -E copy \"${ASAN_DLL}\" $<TARGET_FILE_DIR:hikogui_htests>)\n"
    r += "\n"
    r += "        target_compile_options(hikogui_htests PRIVATE -fsanitize=address)\n"
    r += "        target_link_options(hikogui_htests PRIVATE -fsanitize=address)\n"
    r += "\n"
    r += "    endif()\n"
    r += "endif()\n"
    r += "\n"
    r += "target_sources(hikogui_htests PRIVATE\n"
    for test_file in test_files:
        if test_file in suppressed_test_files:
            r += "    #${CMAKE_CURRENT_SOURCE_DIR}/%s\n" % test_file.replace("\\", "/")
        else:
            r += "    ${CMAKE_CURRENT_SOURCE_DIR}/%s\n" % test_file.replace("\\", "/")
            
    r += ")\n"
    r += "\n"
    r += "show_build_target_properties(hikogui_htests)\n"
    return r


def generate_cmakelists_includes():
    header_files = get_header_files()
    suppressed_header_files = [
        "src/hikogui/net/packet_buffer.hpp",
        "src/hikogui/net/stream.hpp"
    ]

    suppressed_header_files = set(os.path.normcase(x) for x in suppressed_header_files)

    with open("CMakeLists_includes.cmake", "w") as fd:
        fd.write(make_cmakelist_includes_text(header_files, suppressed_header_files))

def generate_cmakelists_tests():
    test_files = get_test_files()
    suppressed_test_files = [
        "src/hikogui/audio/audio_sample_packer_tests.cpp",
        "src/hikogui/audio/audio_sample_unpacker_tests.cpp",
        "src/hikogui/random/dither_tests.cpp",
        "src/hikogui/widgets/text_widget_tests.cpp",
        # Disabled until C++23
        "src/hikogui/container/lean_vector_tests.cpp"
    ]

    suppressed_test_files = set(os.path.normcase(x) for x in suppressed_test_files)

    with open("CMakeLists_tests.cmake", "w") as fd:
        fd.write(make_cmakelist_tests_text(test_files, suppressed_test_files))

def main():
    generate_cmakelists_includes()
    generate_cmakelists_tests()
    
    

if __name__ == "__main__":
    main()
