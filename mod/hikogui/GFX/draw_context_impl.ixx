// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"


export module hikogui_GFX : draw_context_impl;
import : draw_context_intf;
import : gfx_device_intf;
import : gfx_pipeline_SDF_impl;
import : gfx_pipeline_box_impl;
import : gfx_pipeline_image_impl;
import : gfx_pipeline_override_impl;
import hikogui_text;

export namespace hi { inline namespace v1 {

draw_context::draw_context(
    gfx_device& device,
    vector_span<gfx_pipeline_box::vertex>& box_vertices,
    vector_span<gfx_pipeline_image::vertex>& image_vertices,
    vector_span<gfx_pipeline_SDF::vertex>& sdf_vertices,
    vector_span<gfx_pipeline_override::vertex>& override_vertices) noexcept :
    device(std::addressof(device)),
    frame_buffer_index(std::numeric_limits<size_t>::max()),
    scissor_rectangle(),
    _box_vertices(&box_vertices),
    _image_vertices(&image_vertices),
    _sdf_vertices(&sdf_vertices),
    _override_vertices(&override_vertices)
{
    _box_vertices->clear();
    _image_vertices->clear();
    _sdf_vertices->clear();
    _override_vertices->clear();
}

void
draw_context::_draw_override(aarectangle const& clipping_rectangle, quad box, draw_attributes const& attributes) const noexcept
{
    if (_override_vertices->full()) {
        // Too many boxes where added, just don't draw them anymore.
        ++global_counter<"override::overflow">;
        return;
    }

    gfx_pipeline_override::device_shared::place_vertices(*_override_vertices, clipping_rectangle, box, attributes.fill_color, attributes.line_color);
}

void
draw_context::_draw_box(aarectangle const& clipping_rectangle, quad box, draw_attributes const& attributes) const noexcept
{
    // clang-format off
    hilet border_radius = attributes.line_width * 0.5f;
    hilet box_ =
        attributes.border_side == hi::border_side::inside ? box - border_radius :
        attributes.border_side == hi::border_side::outside ? box + border_radius :
        box;

    hilet corner_radius =
        attributes.border_side == hi::border_side::inside ? attributes.corner_radius - border_radius :
        attributes.border_side == hi::border_side::outside ? attributes.corner_radius + border_radius :
        attributes.corner_radius;
    // clang-format on

    if (_box_vertices->full()) {
        // Too many boxes where added, just don't draw them anymore.
        ++global_counter<"draw_box::overflow">;
        return;
    }

    gfx_pipeline_box::device_shared::place_vertices(
        *_box_vertices,
        clipping_rectangle,
        box_,
        attributes.fill_color,
        attributes.line_color,
        attributes.line_width,
        corner_radius);
}

[[nodiscard]] bool draw_context::_draw_image(
    aarectangle const& clipping_rectangle,
    quad const& box,
    gfx_pipeline_image::paged_image const& image) const noexcept
{
    hi_assert_not_null(_image_vertices);

    if (image.state != gfx_pipeline_image::paged_image::state_type::uploaded) {
        return false;
    }

    device->image_pipeline->place_vertices(*_image_vertices, clipping_rectangle, box, image);
    return true;
}

void draw_context::_draw_glyph(
    aarectangle const& clipping_rectangle,
    quad const& box,
    font const& font,
    glyph_id glyph,
    draw_attributes const& attributes) const noexcept
{
    hi_assert_not_null(_sdf_vertices);

    if (_sdf_vertices->full()) {
        auto box_attributes = attributes;
        box_attributes.fill_color = hi::color{1.0f, 0.0f, 1.0f}; // Magenta.
        _draw_box(clipping_rectangle, box, box_attributes);
        ++global_counter<"draw_glyph::overflow">;
        return;
    }

    hilet atlas_was_updated =
        device->SDF_pipeline->place_vertices(*_sdf_vertices, clipping_rectangle, box, font, glyph, attributes.fill_color);

    if (atlas_was_updated) {
        device->SDF_pipeline->prepare_atlas_for_rendering();
    }
}

void draw_context::_draw_text(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper const& text,
    draw_attributes const& attributes) const noexcept
{
    hi_assert_not_null(_sdf_vertices);

    auto atlas_was_updated = false;
    for (hilet& c : text) {
        hilet box = translate2{c.position} * c.metrics.bounding_rectangle;
        hilet color = attributes.num_colors > 0 ? attributes.fill_color : quad_color{c.style->color};

        if (not is_visible(c.general_category)) {
            continue;

        } else if (_sdf_vertices->full()) {
            auto box_attributes = attributes;
            box_attributes.fill_color = hi::color{1.0f, 0.0f, 1.0f}; // Magenta.
            _draw_box(clipping_rectangle, box, box_attributes);
            ++global_counter<"draw_glyph::overflow">;
            break;
        }

        atlas_was_updated |= device->SDF_pipeline->place_vertices(
            *_sdf_vertices, clipping_rectangle, transform * box, *c.glyphs.font, c.glyphs.ids.front(), color);
    }

    if (atlas_was_updated) {
        device->SDF_pipeline->prepare_atlas_for_rendering();
    }
}

void draw_context::_draw_text_selection(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper const& text,
    text_selection const& selection,
    draw_attributes const& attributes) const noexcept
{
    hilet[first, last] = selection.selection_indices();
    hilet first_ = text.begin() + first;
    hilet last_ = text.begin() + last;
    hi_axiom(first_ <= text.end());
    hi_axiom(last_ <= text.end());
    hi_axiom(first_ <= last_);

    for (auto it = first_; it != last_; ++it) {
        _draw_box(clipping_rectangle, transform * it->rectangle, attributes);
    }
}

void draw_context::_draw_text_insertion_cursor_empty(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper const& text,
    draw_attributes const& attributes) const noexcept
{
    hilet maximum_left = std::round(text.rectangle().left() - 0.5f);
    hilet maximum_right = std::round(text.rectangle().right() - 0.5f);
    hilet& only_line = text.lines()[0];

    hilet bottom = std::floor(only_line.rectangle.bottom());
    hilet top = std::ceil(only_line.rectangle.top());
    hilet left = only_line.paragraph_direction == unicode_bidi_class::L ? maximum_left : maximum_right;

    hilet shape_I = aarectangle{point2{left, bottom}, point2{left + 1.0f, top}};
    _draw_box(clipping_rectangle, transform * shape_I, attributes);
}

void draw_context::_draw_text_insertion_cursor(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper const& text,
    text_cursor cursor,
    bool show_flag,
    draw_attributes const& attributes) const noexcept
{
    hilet maximum_left = std::round(text.rectangle().left() - 0.5f);
    hilet maximum_right = std::round(text.rectangle().right() - 0.5f);

    hilet it = text.get_it(cursor);
    hilet& line = text.lines()[it->line_nr];
    hilet ltr = it->direction == unicode_bidi_class::L;
    hilet on_right = ltr == cursor.after();

    // The initial position of the cursor.
    auto bottom = std::floor(line.rectangle.bottom());
    auto top = std::ceil(line.rectangle.top());
    auto left = std::round((on_right ? it->rectangle.right() : it->rectangle.left()) - 0.5f);

    hilet next_line_nr = it->line_nr + 1;
    hilet line_ltr = line.paragraph_direction == unicode_bidi_class::L;
    hilet end_of_line = line_ltr ? it->column_nr == line.columns.size() - 1 : it->column_nr == 0;
    if (cursor.after() and end_of_line and next_line_nr < text.lines().size()) {
        // The cursor is after the last character on the line,
        // the cursor should appear at the start of the next line.
        hilet& next_line = text.lines()[next_line_nr];

        bottom = std::floor(next_line.rectangle.bottom());
        top = std::ceil(next_line.rectangle.top());
        left = it->direction == unicode_bidi_class::L ? maximum_left : maximum_right;
    }

    // Clamp the cursor position between the left and right side of the laid out text.
    left = std::clamp(left, maximum_left - 1.0f, maximum_right + 1.0f);

    // Draw the vertical line cursor.
    hilet shape_I = aarectangle{point2{left, bottom}, point2{left + 1.0f, top}};
    _draw_box(clipping_rectangle, transform * shape_I, attributes);

    if (show_flag) {
        // Draw the LTR/RTL flag at the top of the line cursor.
        hilet shape_flag = ltr ? aarectangle{point2{left + 1.0f, top - 1.0f}, point2{left + 3.0f, top}} :
                                 aarectangle{point2{left - 2.0f, top - 1.0f}, point2{left, top}};

        _draw_box(clipping_rectangle, transform * shape_flag, attributes);
    }
}

void draw_context::_draw_text_overwrite_cursor(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper::char_const_iterator it,
    draw_attributes const& attributes) const noexcept
{
    hilet box = ceil(it->rectangle) + 0.5f;
    _draw_box(clipping_rectangle, transform * box, attributes);
}

void draw_context::_draw_text_cursors(
    aarectangle const& clipping_rectangle,
    matrix3 const& transform,
    text_shaper const& text,
    text_cursor primary_cursor,
    bool overwrite_mode,
    bool dead_character_mode,
    draw_attributes const& attributes) const noexcept
{
    hi_axiom(attributes.line_width == 0.0f);

    if (text.empty()) {
        // When text is empty, draw a cursor directly.
        return _draw_text_insertion_cursor_empty(clipping_rectangle, transform, text, attributes);
    }

    auto draw_flags = false;

    hi_assert_bounds(primary_cursor.index(), text);

    if (dead_character_mode) {
        hi_assert(primary_cursor.before());
        auto cursor_attributes = attributes;
        cursor_attributes.fill_color = attributes.line_color;
        cursor_attributes.line_color = {};
        return _draw_text_overwrite_cursor(
            clipping_rectangle, transform, text.begin() + primary_cursor.index(), cursor_attributes);
    }

    if (overwrite_mode and primary_cursor.before()) {
        auto cursor_attributes = attributes;
        cursor_attributes.fill_color = {};
        cursor_attributes.line_color = attributes.fill_color;
        cursor_attributes.line_width = 1.0f;
        return _draw_text_overwrite_cursor(
            clipping_rectangle, transform, text.begin() + primary_cursor.index(), cursor_attributes);
    }

    // calculate the position of the primary cursor.
    hilet primary_it = text.begin() + primary_cursor.index();
    hilet primary_ltr = primary_it->direction == unicode_bidi_class::L;
    hilet primary_is_on_right = primary_ltr == primary_cursor.after();
    hilet primary_is_on_left = not primary_is_on_right;

    do {
        if (primary_cursor.start_of_text() or primary_cursor.end_of_text(text.size())) {
            // Don't draw secondary cursor which would be on the other edge of the text-field.
            break;
        }

        hilet secondary_cursor = primary_cursor.neighbor(text.size());
        hilet secondary_it = text.begin() + secondary_cursor.index();
        hilet secondary_ltr = secondary_it->direction == unicode_bidi_class::L;
        hilet secondary_is_on_right = secondary_ltr == secondary_cursor.after();
        hilet secondary_is_on_left = not secondary_is_on_right;

        if (primary_is_on_right and secondary_is_on_left and text.move_right_char(primary_it) == secondary_it) {
            // The secondary character is right of primary character, and the cursors are touching.
            break;
        } else if (primary_is_on_left and secondary_is_on_right and text.move_left_char(primary_it) == secondary_it) {
            // The secondary character is left of primary character, and the cursors are touching.
            break;
        }

        draw_flags = true;
        auto cursor_attributes = attributes;
        cursor_attributes.fill_color = attributes.line_color;
        cursor_attributes.line_color = {};
        _draw_text_insertion_cursor(clipping_rectangle, transform, text, secondary_cursor, draw_flags, cursor_attributes);
    } while (false);

    _draw_text_insertion_cursor(clipping_rectangle, transform, text, primary_cursor, draw_flags, attributes);
}

}} // namespace hi::v1
