# Copyright Take Vos 2021.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#-------------------------------------------------------------------
# Build Target: hikogui_demo                             (executable)
#-------------------------------------------------------------------

add_executable(json_to_bon8 WIN32 MACOSX_BUNDLE)
target_sources(json_to_bon8 PRIVATE json_to_bon8_impl.cpp)
target_link_libraries(json_to_bon8 PRIVATE hikogui)
target_include_directories(json_to_bon8 PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/src)
set_target_properties(json_to_bon8 PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(json_to_bon8 PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")

add_dependencies(examples json_to_bon8)

#-------------------------------------------------------------------
# Installation Rules: hikogui_demo
#-------------------------------------------------------------------

install(TARGETS json_to_bon8 DESTINATION examples/codec COMPONENT examples EXCLUDE_FROM_ALL)
