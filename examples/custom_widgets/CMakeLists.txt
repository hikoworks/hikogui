# Copyright Take Vos 2021.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#-------------------------------------------------------------------
# Build Target: hikogui_demo                             (executable)
#-------------------------------------------------------------------

add_executable(minimum_custom_widget_example WIN32 MACOSX_BUNDLE)
target_sources(minimum_custom_widget_example PRIVATE minimum_custom_widget_example_impl.cpp)
target_link_libraries(minimum_custom_widget_example PRIVATE hikogui)
target_link_resources(minimum_custom_widget_example hikogui)
set_target_properties(minimum_custom_widget_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(minimum_custom_widget_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.minimum_custom_widget_example)
set_target_properties(minimum_custom_widget_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(minimum_custom_widget_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(custom_widget_with_child_example WIN32 MACOSX_BUNDLE)
target_sources(custom_widget_with_child_example PRIVATE custom_widget_with_child_example_impl.cpp)
target_link_libraries(custom_widget_with_child_example PRIVATE hikogui)
target_link_resources(custom_widget_with_child_example hikogui)
set_target_properties(custom_widget_with_child_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(custom_widget_with_child_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.custom_widget_with_child_example)
set_target_properties(custom_widget_with_child_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(custom_widget_with_child_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(custom_widget_command_example WIN32 MACOSX_BUNDLE)
target_sources(custom_widget_command_example PRIVATE custom_widget_command_example_impl.cpp)
target_link_libraries(custom_widget_command_example PRIVATE hikogui)
target_link_resources(custom_widget_command_example hikogui)
set_target_properties(custom_widget_command_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(custom_widget_command_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.custom_widget_command_example)
set_target_properties(custom_widget_command_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(custom_widget_command_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(custom_widget_drawing_example WIN32 MACOSX_BUNDLE)
target_sources(custom_widget_drawing_example PRIVATE custom_widget_drawing_example_impl.cpp)
target_resources(custom_widget_drawing_example "${CMAKE_CURRENT_SOURCE_DIR}/resources/mars3.png")
target_link_libraries(custom_widget_drawing_example PRIVATE hikogui)
target_link_resources(custom_widget_drawing_example hikogui)
set_target_properties(custom_widget_drawing_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(custom_widget_drawing_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.custom_widget_drawing_example)
set_target_properties(custom_widget_drawing_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(custom_widget_drawing_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_dependencies(examples
    minimum_custom_widget_example
    custom_widget_with_child_example
    custom_widget_command_example
    custom_widget_drawing_example)

#-------------------------------------------------------------------
# Installation Rules: hikogui_demo
#-------------------------------------------------------------------

install(
    TARGETS
        minimum_custom_widget_example
        custom_widget_with_child_example
        custom_widget_command_example
        custom_widget_drawing_example
    DESTINATION examples/custom_widgets
    RESOURCE DESTINATION examples/custom_widgets/resources)
