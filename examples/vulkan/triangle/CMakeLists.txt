# Copyright Take Vos 2022.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#-------------------------------------------------------------------
# Build Target: hikogui_demo                             (executable)
#-------------------------------------------------------------------

add_executable(vulkan_triangle WIN32)
add_dependencies(examples vulkan_triangle)

target_link_libraries(vulkan_triangle PRIVATE hikogui)
target_link_resources(vulkan_triangle hikogui)
set_target_properties(vulkan_triangle PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(vulkan_triangle PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.vulkan_triangle)
set_target_properties(vulkan_triangle PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(vulkan_triangle PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

target_sources(vulkan_triangle PRIVATE main_impl.cpp triangle_impl.cpp triangle.hpp)

target_resources(vulkan_triangle "${CMAKE_CURRENT_SOURCE_DIR}/resources/vulkan_triangle.png")

set(vulkan_triangle_shader_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/triangle.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/triangle.vert)

set(vulkan_triangle_compiled_shader_files "")
foreach(shader_file ${vulkan_triangle_shader_sources})
    set(input_file "${shader_file}")
    cmake_path(RELATIVE_PATH shader_file BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${shader_file}.spv")

    add_custom_command(
        OUTPUT "${output_file}"
        COMMAND Vulkan::glslc -o "${output_file}" "${input_file}"
        DEPENDS "${input_file}"
        VERBATIM)

    set(vulkan_triangle_compiled_shader_files ${vulkan_triangle_compiled_shader_files} "${output_file}")
endforeach()

add_custom_target(vulkan_triangle_shaders ALL DEPENDS ${vulkan_triangle_compiled_shader_files})
add_dependencies(vulkan_triangle vulkan_triangle_shaders)

target_resources(vulkan_triangle ${vulkan_triangle_compiled_shader_files})

#-------------------------------------------------------------------
# Installation Rules: hikogui_demo
#-------------------------------------------------------------------
install(
    TARGETS vulkan_triangle DESTINATION examples/vulkan/triangle
    RESOURCE DESTINATION examples/vulkan/triangle/resources)
