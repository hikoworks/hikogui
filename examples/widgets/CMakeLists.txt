# Copyright Take Vos 2021-2022.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#-------------------------------------------------------------------
# Build Target: hikogui_demo                             (executable)
#-------------------------------------------------------------------

add_executable(checkbox_example WIN32 MACOSX_BUNDLE)
target_sources(checkbox_example PRIVATE checkbox_example_impl.cpp)
target_link_libraries(checkbox_example PRIVATE hikogui)
target_link_resources(checkbox_example hikogui)
set_target_properties(checkbox_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(checkbox_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.checkbox_example)
set_target_properties(checkbox_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(checkbox_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(text_example WIN32 MACOSX_BUNDLE)
target_sources(text_example PRIVATE text_example_impl.cpp)
target_link_libraries(text_example PRIVATE hikogui)
target_link_resources(text_example hikogui)
set_target_properties(text_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(text_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.text_example)
set_target_properties(text_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(text_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(toggle_example WIN32 MACOSX_BUNDLE)
target_sources(toggle_example PRIVATE toggle_example_impl.cpp)
target_link_libraries(toggle_example PRIVATE hikogui)
target_link_resources(toggle_example hikogui)
set_target_properties(toggle_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(toggle_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.toggle_example)
set_target_properties(toggle_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(toggle_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(radio_button_example WIN32 MACOSX_BUNDLE)
target_sources(radio_button_example PRIVATE radio_button_example_impl.cpp)
target_link_libraries(radio_button_example PRIVATE hikogui)
target_link_resources(radio_button_example hikogui)
set_target_properties(radio_button_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(radio_button_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.radio_button_example)
set_target_properties(radio_button_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(radio_button_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(tab_example WIN32 MACOSX_BUNDLE)
target_sources(tab_example PRIVATE tab_example_impl.cpp)
target_link_libraries(tab_example PRIVATE hikogui)
target_link_resources(tab_example hikogui)
set_target_properties(tab_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(tab_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.tab_example)
set_target_properties(tab_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(tab_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_executable(selection_example WIN32 MACOSX_BUNDLE)
target_sources(selection_example PRIVATE selection_example_impl.cpp)
target_link_libraries(selection_example PRIVATE hikogui)
target_link_resources(selection_example hikogui)
set_target_properties(selection_example PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(selection_example PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.selection_example)
set_target_properties(selection_example PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(selection_example PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_dependencies(examples
    checkbox_example
    text_example
    toggle_example
    radio_button_example
    tab_example
    selection_example)

#-------------------------------------------------------------------
# Installation Rules: hikogui_demo
#-------------------------------------------------------------------
install(
    TARGETS
        checkbox_example
        text_example
        toggle_example
        radio_button_example
        tab_example
        selection_example
    DESTINATION examples/widgets
    RESOURCE DESTINATION examples/widgets/resources)
