// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <unordered_map>
#include <format>
#include <string_view>
#include <string>
#include <ostream>

export module hikogui_GUI : keyboard_virtual_key_intf;
import : keyboard_modifiers;
import hikogui_utility;

export namespace hi::inline v1 {

enum class keyboard_virtual_key : uint8_t {
    nul = 0,

    A = 'a',
    B = 'b',
    C = 'c',
    D = 'd',
    E = 'e',
    F = 'f',
    G = 'g',
    H = 'h',
    I = 'i',
    J = 'j',
    K = 'k',
    L = 'l',
    M = 'm',
    N = 'n',
    O = 'o',
    P = 'p',
    Q = 'q',
    R = 'r',
    S = 's',
    T = 't',
    U = 'u',
    V = 'v',
    W = 'w',
    X = 'x',
    Y = 'y',
    Z = 'z',

    _0 = '0',
    _1 = '1',
    _2 = '2',
    _3 = '3',
    _4 = '4',
    _5 = '5',
    _6 = '6',
    _7 = '7',
    _8 = '8',
    _9 = '9',

    plus = '+',
    minus = '-',
    star = '*',
    slash = '/',
    percent = '%',
    tilde = '~',
    ampersand = '&',
    pipe = '|',
    caret = '^',
    less = '<',
    equal = '=',
    greater = '>',
    open_parentheses = '(',
    close_parentheses = ')',
    open_bracket = '[',
    close_bracket = ']',
    open_brace = '{',
    close_brace = '}',
    period = '.',
    comma = ',',
    colon = ':',
    semi_colon = ';',
    bang = '!',
    question = '?',
    space = ' ',
    tab = '\t',
    enter = '\n',
    backtick = '`',
    quote = '\'',
    double_quote = '"',
    at = '@',
    hash = '#',
    dollar = '$',
    underscore = '_',
    backslash = '\\',

    F1 = 'A',
    F2 = 'B',
    F3 = 'C',
    F4 = 'D',
    F5 = 'E',
    F6 = 'F',
    F7 = 'G',
    F8 = 'H',
    F9 = 'I',
    F10 = 'J',
    F11 = 'K',
    F12 = 'L',
    F13 = 'M',
    F14 = 'N',
    F15 = 'O',
    F16 = 'P',
    F17 = 'Q',
    F18 = 'R',
    F19 = 'S',
    F20 = 'T',
    F21 = 'U',
    F22 = 'V',
    F23 = 'W',
    F24 = 'X',

    home = 0x02, // ASCII start-of-text
    end = 0x03, // ASCII end-of-text
    backspace = 0x08, // ASCII backspace
    clear = 0x0c, // ASCII form-feed
    insert = 0x1a, // ASCII substitute
    escape = 0x1b, // ASCII escape
    _delete = 0x7f, // ASCII delete

    left = 0x80,
    right,
    up,
    down,
    page_up,
    page_down,

    menu,
    print_screen,
    pause_break,
    sysmenu,

    media_next_track,
    media_prev_track,
    media_stop,
    media_play_pause,

    volume_mute,
    volume_up,
    volume_down,

    browser_back,
    browser_forward,
    browser_home,
    browser_refresh,
    browser_stop,
    browser_search,
    browser_favorites,
};

// clang-format off
constexpr auto keyboard_virtual_key_metadata = enum_metadata{
    keyboard_virtual_key::nul, "nul",
    keyboard_virtual_key::A, "a",
    keyboard_virtual_key::B, "b",
    keyboard_virtual_key::C, "c",
    keyboard_virtual_key::D, "d",
    keyboard_virtual_key::E, "e",
    keyboard_virtual_key::F, "f",
    keyboard_virtual_key::G, "g",
    keyboard_virtual_key::H, "h",
    keyboard_virtual_key::I, "i",
    keyboard_virtual_key::J, "j",
    keyboard_virtual_key::K, "k",
    keyboard_virtual_key::L, "l",
    keyboard_virtual_key::M, "m",
    keyboard_virtual_key::N, "n",
    keyboard_virtual_key::O, "o",
    keyboard_virtual_key::P, "p",
    keyboard_virtual_key::Q, "q",
    keyboard_virtual_key::R, "r",
    keyboard_virtual_key::S, "s",
    keyboard_virtual_key::T, "t",
    keyboard_virtual_key::U, "u",
    keyboard_virtual_key::V, "v",
    keyboard_virtual_key::W, "w",
    keyboard_virtual_key::X, "x",
    keyboard_virtual_key::Y, "y",
    keyboard_virtual_key::Z, "z",
    keyboard_virtual_key::_0, "0",
    keyboard_virtual_key::_1, "1",
    keyboard_virtual_key::_2, "2",
    keyboard_virtual_key::_3, "3",
    keyboard_virtual_key::_4, "4",
    keyboard_virtual_key::_5, "5",
    keyboard_virtual_key::_6, "6",
    keyboard_virtual_key::_7, "7",
    keyboard_virtual_key::_8, "8",
    keyboard_virtual_key::_9, "9",
    keyboard_virtual_key::plus, "plus",
    keyboard_virtual_key::minus, "-",
    keyboard_virtual_key::star, "*",
    keyboard_virtual_key::slash, "/",
    keyboard_virtual_key::percent, "%",
    keyboard_virtual_key::tilde, "~",
    keyboard_virtual_key::ampersand, "&",
    keyboard_virtual_key::pipe, "|",
    keyboard_virtual_key::caret, "^",
    keyboard_virtual_key::less, "<",
    keyboard_virtual_key::equal, "=",
    keyboard_virtual_key::greater, ">",
    keyboard_virtual_key::open_parentheses, "(",
    keyboard_virtual_key::close_parentheses, ")",
    keyboard_virtual_key::open_bracket, "[",
    keyboard_virtual_key::close_bracket, "]",
    keyboard_virtual_key::open_brace, "{",
    keyboard_virtual_key::close_brace, "}",
    keyboard_virtual_key::period, ".",
    keyboard_virtual_key::comma, ",",
    keyboard_virtual_key::colon, ":",
    keyboard_virtual_key::semi_colon, ";",
    keyboard_virtual_key::bang, "!",
    keyboard_virtual_key::question, "?",
    keyboard_virtual_key::space, "space",
    keyboard_virtual_key::tab, "tab",
    keyboard_virtual_key::enter, "enter",
    keyboard_virtual_key::backtick, "`",
    keyboard_virtual_key::quote, "quote",
    keyboard_virtual_key::double_quote, "dquote",
    keyboard_virtual_key::at, "@",
    keyboard_virtual_key::hash, "#",
    keyboard_virtual_key::dollar, "$",
    keyboard_virtual_key::underscore, "_",
    keyboard_virtual_key::backslash, "backslash",
    keyboard_virtual_key::F1, "f1",
    keyboard_virtual_key::F2, "f2",
    keyboard_virtual_key::F3, "f3",
    keyboard_virtual_key::F4, "f4",
    keyboard_virtual_key::F5, "f5",
    keyboard_virtual_key::F6, "f6",
    keyboard_virtual_key::F7, "f7",
    keyboard_virtual_key::F8, "f8",
    keyboard_virtual_key::F9, "f9",
    keyboard_virtual_key::F10, "f10",
    keyboard_virtual_key::F11, "f11",
    keyboard_virtual_key::F12, "f12",
    keyboard_virtual_key::F13, "f13",
    keyboard_virtual_key::F14, "f14",
    keyboard_virtual_key::F15, "f15",
    keyboard_virtual_key::F16, "f16",
    keyboard_virtual_key::F17, "f17",
    keyboard_virtual_key::F18, "f18",
    keyboard_virtual_key::F19, "f19",
    keyboard_virtual_key::F20, "f20",
    keyboard_virtual_key::F21, "f21",
    keyboard_virtual_key::F22, "f22",
    keyboard_virtual_key::F23, "f23",
    keyboard_virtual_key::F24, "f24",
    keyboard_virtual_key::home, "home",
    keyboard_virtual_key::end, "end",
    keyboard_virtual_key::backspace, "backspace",
    keyboard_virtual_key::clear, "clear",
    keyboard_virtual_key::insert, "insert",
    keyboard_virtual_key::escape, "escape",
    keyboard_virtual_key::_delete, "delete",
    keyboard_virtual_key::left, "left",
    keyboard_virtual_key::right, "right",
    keyboard_virtual_key::up, "up",
    keyboard_virtual_key::down, "down",
    keyboard_virtual_key::page_up, "page-up",
    keyboard_virtual_key::page_down, "page-down",
    keyboard_virtual_key::menu, "menu",
    keyboard_virtual_key::sysmenu, "sysmenu",
    keyboard_virtual_key::print_screen, "print-screen",
    keyboard_virtual_key::pause_break, "pause-break",
    keyboard_virtual_key::media_next_track, "media-next",
    keyboard_virtual_key::media_prev_track, "media-prev",
    keyboard_virtual_key::media_stop, "media-stop",
    keyboard_virtual_key::media_play_pause, "media-play",
    keyboard_virtual_key::volume_mute, "volume-mute",
    keyboard_virtual_key::volume_up, "volume-up",
    keyboard_virtual_key::volume_down, "volume-down",
    keyboard_virtual_key::browser_back, "browser-back",
    keyboard_virtual_key::browser_forward, "browser-forward",
    keyboard_virtual_key::browser_home, "browser-home",
    keyboard_virtual_key::browser_refresh, "browser-refresh",
    keyboard_virtual_key::browser_stop, "browser-stop",
    keyboard_virtual_key::browser_search, "browser-search",
    keyboard_virtual_key::browser_favorites, "browser-favorites"
};
// clang-format on

constexpr keyboard_virtual_key to_keyboard_virtual_key(std::string_view s)
{
    return keyboard_virtual_key_metadata[s];
}

[[nodiscard]] constexpr keyboard_virtual_key to_keyboard_virtual_key(int key_code, bool extended, keyboard_modifiers modifiers) noexcept;

constexpr std::string_view to_string_view(keyboard_virtual_key key) noexcept
{
    return keyboard_virtual_key_metadata[key];
}

std::string to_string(keyboard_virtual_key key) noexcept
{
    return std::string{to_string_view(key)};
}

std::ostream& operator<<(std::ostream& lhs, keyboard_virtual_key const& rhs)
{
    return lhs << to_string_view(rhs);
}

} // namespace hi::inline v1

template<>
struct std::hash<hi::keyboard_virtual_key> {
    [[nodiscard]] std::size_t operator()(hi::keyboard_virtual_key rhs) const noexcept
    {
        return std::hash<uint8_t>{}(static_cast<uint8_t>(rhs));
    }
};

// XXX #617 MSVC bug does not handle partial specialization in modules.
export template<>
struct std::formatter<hi::keyboard_virtual_key, char> : std::formatter<std::string_view, char> {
    auto format(hi::keyboard_virtual_key const& t, auto& fc) const
    {
        return std::formatter<std::string_view, char>{}.format(hi::to_string_view(t), fc);
    }
};
