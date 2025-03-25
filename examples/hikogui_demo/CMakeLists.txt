# Copyright Take Vos 2022.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#-------------------------------------------------------------------
# Build Target: hikogui_demo                             (executable)
#-------------------------------------------------------------------

add_executable(hikogui_demo WIN32)
add_dependencies(examples hikogui_demo)
target_link_libraries(hikogui_demo PRIVATE hikogui)
target_link_resources(hikogui_demo hikogui)
set_target_properties(hikogui_demo PROPERTIES MACOSX_BUNDLE TRUE)
set_target_properties(hikogui_demo PROPERTIES MACOSX_FRAMEWORK_IDENTIFIER org.hikogui.hikogui.hikogui_demo)
set_target_properties(hikogui_demo PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(hikogui_demo PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

target_sources(hikogui_demo PRIVATE
    src/main_impl.cpp
    PUBLIC ${hikogui_demo_resource_files})

target_resources(hikogui_demo
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/en.po
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/nl.po
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/hikogui_demo.png)

#-------------------------------------------------------------------
# Installation Rules: hikogui_demo
#-------------------------------------------------------------------

install(
    TARGETS hikogui_demo DESTINATION examples/hikogui_demo
    RESOURCE DESTINATION examples/hikogui_demo/resources)

