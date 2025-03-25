// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/audio_device_widget.hpp Defines audio_device_widget.
 * @ingroup widgets
 */

#pragma once

#include "selection_widget.hpp"
#include "grid_widget.hpp"
#include "../audio/audio.hpp"
#include "../l10n/l10n.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <coroutine>

hi_export_module(hikogui.widgets.audio_device_widget);

hi_export namespace hi {
inline namespace v1 {

/** Audio device configuration widget.
 * @ingroup widgets
 */
class audio_device_widget : public widget {
public:
    using super = widget;

    /** The audio device this widget has selected and is configuring.
     */
    observer<std::string> device_id;

    /** The audio direction (input or output) of devices is should show.
     */
    observer<audio_direction> direction = audio_direction::bidirectional;

    virtual ~audio_device_widget() {}

    audio_device_widget() noexcept : super()
    {
        _grid_widget = std::make_unique<grid_widget>();
        _grid_widget->set_parent(this);

        _device_selection_widget = &_grid_widget->emplace<selection_widget>("A1", device_id, _device_list);

        _sync_device_list_task = sync_device_list();

        style.set_name("audio-device");
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf&> children(bool include_invisible) const noexcept override
    {
        co_yield *_grid_widget;
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _grid_constraints = _grid_widget->update_constraints();
        return _grid_constraints;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _grid_shape = box_shape{context.rectangle()};
        _grid_widget->set_layout(context.transform(_grid_shape, transform_command::level));
    }

    hitbox hitbox_test(point2 position) const noexcept override
    {
        if (enabled()) {
            auto r = hitbox{};
            r = _grid_widget->hitbox_test_from_parent(position, r);
            return r;
        } else {
            return hitbox{};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        if (enabled()) {
            return _grid_widget->accepts_keyboard_focus(group);
        } else {
            return false;
        }
    }
    /// @endprivatesection
private:
    /** The grid widget contains all the child widgets.
     */
    std::unique_ptr<grid_widget> _grid_widget;
    box_constraints _grid_constraints;
    box_shape _grid_shape;

    /** The widget used to select the audio device.
     */
    selection_widget* _device_selection_widget = nullptr;

    observer<std::vector<std::pair<std::string, label>>> _device_list;

    hi::scoped_task<> _sync_device_list_task;

    [[nodiscard]] hi::scoped_task<> sync_device_list() noexcept
    {
        while (true) {
            {
                auto proxy = _device_list.get();
                proxy->clear();
                for (auto& device : audio_devices(hi::audio_device_state::active, *direction)) {
                    proxy->emplace_back(device.id(), device.label());
                }
            }

            co_await when_any(audio_system::global(), direction);
        }
    }
};

} // namespace v1
} // namespace hi::v1
