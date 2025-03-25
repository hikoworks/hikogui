# This file was generated with tools/generate_cmakelists.sh

add_executable(hikogui_htests)
target_link_libraries(hikogui_htests PRIVATE hikotest hikogui)
target_include_directories(hikogui_htests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(hikogui_htests PROPERTIES DEBUG_POSTFIX "-dbg")
set_target_properties(hikogui_htests PROPERTIES RELEASE_POSTFIX "-rel")
set_target_properties(hikogui_htests PROPERTIES RELWITHDEBINFO_POSTFIX "-rdi")
add_test(NAME hikogui_htests COMMAND hikogui_htests)

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(ASAN_DLL "${CMAKE_CXX_COMPILER}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        cmake_path(REPLACE_FILENAME ASAN_DLL "clang_rt.asan_dbg_dynamic-x86_64.dll")
    else()
        cmake_path(REPLACE_FILENAME ASAN_DLL "clang_rt.asan_dynamic-x86_64.dll")
    endif()

    add_custom_command(TARGET hikogui_htests PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${ASAN_DLL}" $<TARGET_FILE_DIR:hikogui_htests>)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(hikogui_htests PRIVATE "_DISABLE_VECTOR_ANNOTATION")
        target_compile_definitions(hikogui_htests PRIVATE "_DISABLE_STRING_ANNOTATION")
        target_compile_options(hikogui_htests PRIVATE -fsanitize=address)
    endif()

elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(ASAN_DLL "${CMAKE_CXX_COMPILER}")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            cmake_path(REPLACE_FILENAME ASAN_DLL "clang_rt.asan_dbg_dynamic-x86_64.dll")
        else()
            cmake_path(REPLACE_FILENAME ASAN_DLL "clang_rt.asan_dynamic-x86_64.dll")
        endif()

        add_custom_command(TARGET hikogui_htests PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${ASAN_DLL}" $<TARGET_FILE_DIR:hikogui_htests>)

        target_compile_options(hikogui_htests PRIVATE -fsanitize=address)
        target_link_options(hikogui_htests PRIVATE -fsanitize=address)

    endif()
endif()

target_sources(hikogui_htests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/float_to_half_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/half_to_float_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/simd_f16x4_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/simd_f32x2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/simd_f32x4_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/simd_f64x2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikocpu/simd_f64x4_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/GUI/widget_state_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/algorithm/algorithm_misc_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/algorithm/strings_tests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/audio/audio_sample_packer_tests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/audio/audio_sample_unpacker_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/ascii_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/char_converter_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/cp_1252_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/iso_8859_1_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/utf_16_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/utf_32_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/char_maps/utf_8_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/BON8_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/JSON_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/SHA2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/base_n_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/datum_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/codec/jsonpath_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/color/color_space_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/concurrency/callback_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/concurrency/unfair_mutex_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/container/expected_optional_tests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/container/lean_vector_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/container/polymorphic_optional_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/dispatch/awaitable_future_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/dispatch/function_predicate_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/dispatch/notifier_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/file/file_view_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/font/font_char_map_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/font/font_weight_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/matrix3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/point2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/point3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/scale2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/scale3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/transform_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/translate2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/translate3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/vector2_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/geometry/vector3_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/graphic_path/bezier_curve_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/graphic_path/graphic_path_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/i18n/iso_15924_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/i18n/iso_3166_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/i18n/iso_639_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/i18n/language_tag_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/image/pixmap_span_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/image/pixmap_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/layout/spreadsheet_address_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/numeric/bigint_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/numeric/int_carry_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/numeric/polynomial_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/observer/group_ptr_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/observer/shared_state_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/parser/lexer_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/parser/lookahead_iterator_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/path/URI_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/path/URL_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/path/glob_tests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/random/dither_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/random/seed_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/random/xorshift128p_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/security/sip_hash_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/settings/user_settings_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/telemetry/counters_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/telemetry/format_check_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/theme/style_parser_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/theme/style_selector_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/grapheme_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/gstring_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/markup_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/ucd_scripts_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/unicode_bidi_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/unicode_break_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/unicode/unicode_normalization_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/units/em_squares_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/units/pixel_density_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/units/pixels_per_inch_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/cast_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/defer_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/enum_metadata_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/fixed_string_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/forward_value_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/generator_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/reflection_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/utility/type_traits_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/widgets/button_delegate_tests.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/src/hikogui/widgets/text_widget_tests.cpp
)

show_build_target_properties(hikogui_htests)
