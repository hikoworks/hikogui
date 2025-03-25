// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"
#include <memory>
#include <stacktrace>

// import hikogui;

class my_preferences : public hi::preferences {
public:
    hi::observer<std::string> audio_output_device_id;
    hi::observer<bool> audio_output_exclusive;
    hi::observer<double> audio_output_sample_rate;
    hi::observer<hi::speaker_mapping> audio_output_speaker_mapping;

    hi::observer<std::string> audio_input_device_id;
    hi::observer<bool> audio_input_exclusive;
    hi::observer<double> audio_input_sample_rate;
    hi::observer<hi::speaker_mapping> audio_input_speaker_mapping;

    hi::observer<int> tab_index = 1;
    hi::observer<bool> toggle_value;
    hi::observer<int> radio_value = 0;
    hi::observer<std::vector<std::pair<std::string, hi::label>>> _audio_device_list;

    hi::observer<std::string> selected_theme;

    my_preferences(std::filesystem::path path) : hi::preferences(std::move(path))
    {
        add("audio_output_device_id", audio_output_device_id);
        add("audio_output_exclusive", audio_output_exclusive);
        add("audio_output_sample_rate", audio_output_sample_rate);
        add("audio_input_device_id", audio_input_device_id);
        add("audio_input_exclusive", audio_input_exclusive);
        add("audio_input_sample_rate", audio_input_sample_rate);
        add("tab_index", tab_index);
        add("toggle_value", toggle_value);
        add("radio_value", radio_value);
        add("selected_theme", selected_theme);
    }
};

hi::scoped_task<> init_audio_tab(hi::grid_widget& grid, my_preferences& preferences) noexcept
{
    using namespace hi;

    grid.emplace<label_widget>("A1", txt("Input audio device:")).style = ".right";

    auto& input_config = grid.emplace<audio_device_widget>("B1");
    input_config.direction = audio_direction::input;
    input_config.device_id = preferences.audio_input_device_id;

    grid.emplace<label_widget>("A2", txt("Output audio device:")).style = ".right";
    auto& output_config = grid.emplace<audio_device_widget>("B2");
    output_config.direction = audio_direction::output;
    output_config.device_id = preferences.audio_output_device_id;

    co_await std::suspend_always{};
}

hi::scoped_task<> init_theme_tab(hi::grid_widget& grid, my_preferences& preferences) noexcept
{
    using namespace hi;

    hi::observer<std::vector<std::pair<std::string, hi::label>>> theme_list;

    {
        auto proxy = theme_list.get();
        for (auto const& name : theme_names()) {
            proxy->emplace_back(name, txt(name));
        }
    }

    grid.emplace<label_widget>("A1", txt("Theme:")).style = ".right";
    grid.emplace<selection_widget>("B1", preferences.selected_theme, theme_list);

    co_await std::suspend_always{};
}

hi::scoped_task<> init_license_tab(hi::grid_widget& grid, my_preferences& preferences) noexcept
{
    using namespace hi;

    grid.emplace<label_widget>(
            "A1",
            txt("This is a [he-IL]\xd7\x9c\xd6\xb0\xd7\x9e\xd6\xb7\xd7\xaa\xd6\xb5\xd7\x92[.].\rAnd another sentence. One more:"))
        .style = ".right";
    auto& toggle = grid.emplace<toggle_with_label_widget>("B1", preferences.toggle_value);
    toggle.on_label = txt("true");
    toggle.off_label = txt("false");
    toggle.other_label = txt("other");

    grid.emplace<label_widget>("A2", txt("These is a disabled checkbox:")).style = ".right";
    auto& checkbox2 = grid.emplace<checkbox_with_label_widget>("B2", preferences.radio_value, 2);
    checkbox2.on_label = txt("Checkbox, with a pretty large label.");
    checkbox2.off_label = txt("off");
    checkbox2.other_label = txt("other");

    grid.emplace<label_widget>("A3", txt("These are radio buttons:")).style = ".right";
    auto& radio1 = grid.emplace<radio_with_label_widget>("B3", preferences.radio_value, 0);
    radio1.on_label = radio1.off_label = txt("Radio 1");
    auto& radio2 = grid.emplace<radio_with_label_widget>("B4", preferences.radio_value, 1);
    radio2.on_label = txt("Radio 2 (on)");
    radio2.off_label = txt("Radio 2 (off)");
    auto& radio3 = grid.emplace<radio_with_label_widget>("B5", preferences.radio_value, 2);
    radio3.on_label = radio3.off_label = txt("Radio 3");

    auto option_list = std::vector{
        std::pair{0, label{txt("first")}},
        std::pair{1, label{txt("second")}},
        std::pair{2, label{txt("third")}},
        std::pair{3, label{txt("four")}},
        std::pair{4, label{txt("five")}},
        std::pair{5, label{txt("six")}},
        std::pair{6, label{txt("seven")}}};

    grid.emplace<label_widget>("A6", txt("This is a selection box at the bottom:")).style = ".right";
    auto& selection3 = grid.emplace<selection_widget>("B6", preferences.radio_value, option_list);

    grid.emplace<label_widget>("A7", txt("Sample Rate:")).style = ".right";
    grid.emplace<text_field_widget>("B7", preferences.audio_output_sample_rate);

    auto toggle_value_cbt = preferences.toggle_value.subscribe(
        [&](auto...) {
            checkbox2.set_enabled(*preferences.toggle_value);
            selection3.set_enabled(*preferences.toggle_value);
        },
        callback_flags::main);

    grid.emplace<label_widget>("A8:B8", txt("This is large number locale formatted: {:L}", 1234.56));

    co_await std::suspend_always{};
}

hi::task<> preferences_window(std::stop_token stop_token, my_preferences& preferences)
{
    using namespace hi;

    auto window_label = label{hi::bookmark::resource("hikogui_demo.png"), txt("Preferences")};
    auto top = std::make_unique<window_widget>(window_label);

    auto &tab1 = top->toolbar().emplace<toolbar_tab_button_widget>(preferences.tab_index, 0);
    tab1.on_label = tab1.off_label = label{elusive_icon::Speaker, txt("Audio")};
    auto &tab2 = top->toolbar().emplace<toolbar_tab_button_widget>(preferences.tab_index, 1);
    tab2.on_label = tab2.off_label = label{elusive_icon::Key, txt("License")};
    auto &tab3 = top->toolbar().emplace<toolbar_tab_button_widget>(preferences.tab_index, 2);
    tab3.on_label = tab3.off_label = label{elusive_icon::Brush, txt("Theme")};

    auto& tabs = top->content().emplace<tab_widget>("A1", preferences.tab_index);
    auto& audio_tab_grid = tabs.emplace<grid_widget>(0);
    auto& license_tab_grid = tabs.emplace<scroll_widget<axis::both>>(1).emplace<grid_widget>();
    auto& theme_tab_grid = tabs.emplace<grid_widget>(2);

    auto audio_tab = init_audio_tab(audio_tab_grid, preferences);
    auto license_tab = init_license_tab(license_tab_grid, preferences);
    auto theme_tab = init_theme_tab(theme_tab_grid, preferences);

    auto window = gui_window{std::move(top)};

    co_await when_any(window.closing, stop_token);
}

inline std::shared_ptr<hi::button_delegate> preferences_window_button_delegate;

hi::task<> main_window(my_preferences& preferences)
{
    using namespace hi;

    auto window_label = label{hi::bookmark::resource("hikogui_demo.png"), txt("HikoGUI demo")};
    auto top = std::make_unique<window_widget>(window_label);

    auto& preferences_button = top->toolbar().emplace<button_widget>(preferences_window_button_delegate);
    preferences_button.label = label{elusive_icon::Wrench, txt("Preferences")};

    top->content().emplace_bottom<toggle_with_label_widget>(preferences.toggle_value);

    auto& vma_dump_button = top->content().emplace_bottom<button_widget>();
    vma_dump_button.label = txt("VMA\rcalculate stats");

    auto& abort_button = top->content().emplace_bottom<button_widget>(preferences_window_button_delegate);
    abort_button.label = txt("preferences");

    auto& break_button = top->content().emplace_bottom<button_widget>([]{
        using namespace std::chrono_literals;

        hi_log_info("Break button pressed");
        std::this_thread::sleep_for(1s);
        hi_log_info("Break finished.");
    });
    break_button.label = txt("async call");

    auto window = gui_window{std::move(top)};

    while (true) {
        auto const result = co_await when_any(
            preferences_button, vma_dump_button, abort_button, break_button, preferences.toggle_value, window.closing);

        switch (result.index()) {
        case 0:
            break;
        case 1:
            gfx_system::global().log_memory_usage();
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            hi_log_info("Toggle value {}", std::get<4>(result));
            break;
        case 5:
            co_return;
        default:
            hi_no_default();
        }
    }
}

int hi_main(int argc, char* argv[])
{
    using namespace hi;

    set_application_name("HikoGUI Demo");
    set_application_vendor("HikoGUI");
    set_application_version({1, 0, 0});

    // Start the logger system, so logging is done asynchronously.
    log::start_subsystem(global_state_type::log_level_info);
    start_render_doc();

    // XXX #707 If preferences.json file does not exist, this will fail.
    auto preferences = my_preferences(get_path(data_dirs(), "preferences.json"));
    preferences_window_button_delegate = make_shared_ctad<default_button_delegate>(preferences_window, preferences);

    theme_book::global().selected_theme = preferences.selected_theme;

    main_window(preferences);
    return loop::main().resume();
}

// extern "C" const char *__asan_default_options() {
//    return "help=1:log_path=asan.log";
//}
