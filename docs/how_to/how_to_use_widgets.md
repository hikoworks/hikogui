How to use widgets
==================

Creating Windows
----------------

In the following example, a new window is created. First we need to create
a `std::unique_ptr` to a `hi::window_widget`, it accepts a label or txt
as an argument for the title of the window. The label may contain both a
translatable text and an icon.

Then we create a new window, this may be directly creates on the stack or
on the heap, the window will take ownership of the `hi::window_widget`.

```cpp
auto widget = std::make_unique<hi::window_widget>(hi::txt("The window title"));
auto window = hi::gui_window(std::move(widget));
```

Unlike many other GUI libraries, a window is not given a size; its initial,
minimum and maximum size is determined by the constraints and the layout
algorithm of the widgets being displayed in the window.

From the first construction of a `hi::gui_window`, any usage of the GUI system,
its windows and their widgets must be done from the same thread,
called the `main` thread.

After creating at least one window, you should call `hi::loop::main().resume()`
member function. This function will enter the system's GUI-loop, monitor
keyboard & mouse events and render all the windows. Once all windows are
closed the `resume()` function will return with a return code, which may be
returned from the `main()` function.

```cpp
int hi_main(int argc, char *argv[])
{
    hi::set_application_name("Hello World");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    auto widget = std::make_unique<hi::window_widget>(hi::txt("The window title"));
    widget->content().emplace<hi::momentary_button_widget>("A1", hi::txt("Does nothing"));

    auto window = std::make_unique<hi::gui_window>(std::move(widget));

    auto close_cb = window->closing.subscribe(
        [&] {
            // When the close button on the window is clicked this lambda
            // will destroy the actual window, which will in turn cause the
            // resume() function to exit.
            window.reset();
        },
        hi::callback_flags::main);

    return hi::loop::main().resume();
}
```

How to add widgets
------------------

Widget are graphical elements which are shown inside a window and often can be
interacted with using the mouse and keyboard, such as various kinds of buttons,
text fields and selection boxes.

Many widgets such as the grid layout widget, column layout widget or the scroll
view widget are containers for other widgets. The top level widget of a window
for example contains two containers a `hi::grid_widget` for the content area and
a `hi::toolbar_widget`.

In the example below we are adding 4 widgets to the content area of the window.

The `hi::window::content()` function returns a reference to the `hi::grid_widget`,
and we use its `emplace<>()` function to add new widgets. The template
argument is the type of widget to instantiate and the first argument is the
position within the grid widget. The rest of the arguments are passed to the
constructor of the new widget.

```cpp
int hi_main(int argc, char *argv[])
{
    observer<int> value = 0;

    set_application_name("Radio button example");
    set_application_vendor("HikoGUI");
    set_application_version({1, 0, 0});

    auto widget = std::make_unique<window_widget>(txt("Radio button example"));
    widget->content().emplace<label_widget>("A1", txt("radio buttons:"));
    widget->content().emplace<radio_button_widget>("B1", value, 1, txt("one"));
    widget->content().emplace<radio_button_widget>("B2", value, 2, txt("two"));
    widget->content().emplace<radio_button_widget>("B3", value, 3, txt("three"));

    auto window = std::make_unique<gui_window>(std::move(widget));

    auto close_cbt = window->closing.subscribe(
        [&] {
            window.reset();
        },
        hi::callback_flags::main);
    return loop::main().resume();
}
```

There are often two different ways to construct a widget: with a delegate or
with an observer. In the example above we use an `hi::observer<int>` for the
radio buttons to monitor and update. Sharing the same observer allows the
radio buttons to act as a set.

### Layout using the grid widget

The `hi::grid_widget` is a powerful layout widget which allows adding of new widgets
using the `hi::grid_widget::emplace<>()` member function.

The template parameter for `emplace()` specifies the widget class to allocate and construct.
The first argument to `emplace()` is a string specifying the location where the
new widget should be positioned. The rest of the arguments are passed to the constructor of
the new widget.

The location is specified using a spreadsheet-like address.

There are two forms:

 - single cell, examples: "A1", "C4", "AB45"
 - cell range, examples: "A1:E1", "C4:C7", "Z3:AA4"

A widget may span multiple rows and columns. Widget that spans multiple column or rows
are often widgets that are resizable; therefor the `grid_widget` will override the
preferred- and maximum constraint of other widgets in those rows or columns.


### Observer

An observer is a type that observers a value, it will use callbacks to notify listeners when the
observed value changes. Unlike other parts of the GUI system, observers are thread-save and
may be read and written from any thread.

In the example below a checkbox monitors the observer `my_value`:

- when the value is `bar` the box is checked,
- when the value is `foo` the box is unchecked,
- when the value is anything else the box shows a dash.

```cpp
enum class my_type {foo, bar, baz};

observer<my_type> my_value;
widget->content().emplace<checkbox_widget>("A1", my_value, my_type::bar, my_type::foo);
```

As you can see, the `checkbox_widget` will work with custom types. For the checkbox
the type needs to be equality comparable and assignable.

It is also possible to chain observers to each other. Chaining is done by
assigning an observer to another observer. You can also get a chained sub-observer,
selecting a member variable or the result of the index-operator from the observed value.

In the example below, we make another checkbox, but now it will listen to
the `my_chain` observer. When `my_value` gets assigned to `my_chain`,
`my_chain` will start observing `my_value`. Any modification of `my_value`
will be observed by the checkbox through the chain of observers.

```cpp
enum class my_type {foo, bar, baz};

observer<my_type> my_value;
observer<my_type> my_chain;
widget->content().emplace<checkbox_widget>("A1", my_chain, my_type::bar, my_type::foo);

my_chain = my_value;
my_value = my_type::bar;
```

Observers are used for many member variables of a widget, including the
`hi::widget::enabled`, `hi::widget::visible` members and various labels.

### Delegates

A widget may also be controlled through a delegate object. The widget queries a
delegate for the data to display and sends messages to the delegate when a user
interacts with the widget.

Delegates are actually the primary way for controlling a widget, the
`hi::observer` examples above are implemented by templated default-delegates.

In the example below, a user defined instance of `my_delegate` is passed to the
constructor of the `hi::checkbox_button`. `my_delegate` must inherit from
`hi::button_delegate`.

```cpp
auto delegate = std::make_shared<my_delegate>();
auto button = widget->emplace<checkbox_button>("A1", delegate));
```

A list of widgets
-----------------

- **Simple widgets**
  - `hi::text_widget`: Displays, select and edit text.
  - `hi::icon_widget`: Displays a small image.
  - `hi::label_widget`: Lays out and displays text and a icon together.

- **Container widgets**
  - `hi::grid_widget`: Lays out children in a grid of variable sized cells.
  - `hi::tab_widget`: Shows one child at a time.
  - `hi::scroll_widget`: Allows a larger child to be shown in less space.
  - `hi::overlay_widget`: Shows a child anywhere on the window, overlaying above
    any other widget.
  - `hi::toolbar_widget`: Lays out children in a toolbar.

- **Buttons**
  - `hi::momentary_button_widget`: A push button designed to be used with a
    callback function.
  - `hi::toolbar_button_widget`: A push button designed to be used with a
    callback function. Specifically for use inside a `hi::toolbar_widget`.
  - `hi::toggle_widget`: A button representing a binary choice and indicating
    an immediate effect.
  - `hi::checkbox_widget`: A button representing a binary choice, but also being
    able to show a third 'other' state.
  - `hi::radio_button_widget`: A button representing one out of mutually
    exclusive choices.
  - `hi::toolbar_tab_button_widget`: A button representing one out of mutually
    exclusive tabs. Designed to control the `hi::tab_widget`. For use inside a
    `hi::toolbar_widget`.
  - `hi::menu_button_widget`: A button used inside menus.
- **Misc**
  - `hi::selection_widget`: A selection widget allows selecting one out of a set
    of choices.
  - `hi::text_field_widget`: A text field widgets allows a user to type in
    a value of different types: std::string, int, float, etc.
  - `hi::window_widget`: A window widget is directly owned by a window.
  - `hi::window_traffic_light_widget`: This widget displays the minimize,
    maximize and close button of a window.
  - `hi::scroll_bar_widget`: This widget shows a scroll-bar and is part of a
    `scroll_widget`.
  - `hi::system_menu_widget`: The system menu is a logo to show for the window
    and also is the mouse target for the system-menu that every window has in
    Windows 10.
