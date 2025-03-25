

#pragma once

#include "../utility/utility.hpp"
#include "../dispatch/dispatch.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets : widget_delegate);

hi_export namespace hi::inline v1 {

/** A delegate which bridges widgets with data.
 *
 * Delegates for compound widgets may require multiple inheritance
 * with a diamond pattern. Please always use "virtual inheritance"
 * when inheriting from any delegate class.
 *
 * All member functions must accept a `widget_intf const *` as the
 * first arguments. The widget calling these functions should
 * pass in the `this` pointer, or sometimes the pointer of a
 * child or parent widget of compound widgets. Unit-tests may
 * pass in a nullptr.
 *
 * `<name>_delegate` classes have the following optional member functions:
 *  - `[[nodiscard]] virtual bool empty_<name>(widget_intf const* sender) const`
 *  - `[[nodiscard]] virtual <value-type> get_<name>(widget_intf const* sender) const`
 *  - `[[nodiscard]] virtual bool mutable_<name>(widget_intf const* sender) const`
 *  - `virtual void set_<name>(widget_intf const* sender, <value-type> const &value)`
 *  - `virtual void toggle_<name>(widget_intf const* sender)`
 *
 * The name of the delegate is included in each member function, so
 * that it is possible to do multiple inheritance of different types
 * of delegates. Useful for compound widgets.
 *
 * @note Always use virtual inheritance with delegates.
 */
class widget_delegate {
public:
    virtual ~widget_delegate() = default;

    /** This function is called when a widget has been constructed.
     *
     * @param sender The instance of the widget that calls this function.
     *               This may be a nullptr is the sender is not a widget.
     */
    virtual void init(widget_intf const* sender) {}

    /** This function is called before a widget is destructed.
     *
     * @param sender The instance of the widget that calls this function.
     *               This may be a nullptr is the sender is not a widget.
     */
    virtual void deinit(widget_intf const* sender) {}

    /** Subscribe a callback for notifying the widget of a data change.
     *
     * @param sender The instance of the widget that calls this function.
     *               This may be a nullptr is the sender is not a widget.
     * @param func The function object to call when the state of the
     *             delegate changes.
     * @param flags The flags on how the function object is called.
     * @return A callback object which retains the callback. If the
     *         callback object is destroyed the callback is automatically
     *         unsubscribed.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe(widget_intf const* sender, Func&& func, callback_flags flags = callback_flags::synchronous)
    {
        return _notifier.subscribe(std::forward<Func>(func), flags);
    }

protected:
    notifier<void()> _notifier;
};

}

