// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "iso_15924_intf.hpp"
#include "../utility/utility.hpp"
#include "../algorithm/algorithm.hpp"
#include "../macros.hpp"
#include <array>
#include <algorithm>
#include <format>
#include <compare>

hi_export_module(hikogui.i18n.iso_15924 : impl);

hi_export namespace hi { inline namespace v1 {
namespace detail {

struct iso_15924_info {
    fixed_string<4> code4;
    fixed_string<4> code4_open_type;
    uint16_t number;

    constexpr iso_15924_info(char const (&code4)[5], char const (&code4_open_type)[5], uint16_t number) noexcept :
        code4{to_title(fixed_string(code4))}, code4_open_type{code4_open_type}, number(number)
    {
    }

    constexpr iso_15924_info() noexcept : iso_15924_info("zzzz", 999) {}

    constexpr iso_15924_info(char const (&code4)[5], uint16_t number) noexcept : iso_15924_info(code4, code4, number) {}
};

[[nodiscard]] consteval auto iso_15924_infos_init() noexcept
{
    // We are using a c-style array to std::array conversion because
    // compilers, tools and analysers do not handle large std::array constructors.

    // clang-format off
    constexpr iso_15924_info data[] = {
        {"adlm", 166},
        {"afak", 439},
        {"aghb", 239},
        {"ahom", 338},
        {"arab", 160},
        {"aran", 161},
        {"armi", 124},
        {"armn", 230},
        {"avst", 134},
        {"bali", 360},
        {"bamu", 435},
        {"bass", 259},
        {"batk", 365},
        {"beng", "bng2", 325},
        {"bhks", 334},
        {"blis", 550},
        {"bopo", 285},
        {"brah", 300},
        {"brai", 570},
        {"bugi", 367},
        {"buhd", 372},
        {"cakm", 349},
        {"cans", 440},
        {"cari", 201},
        {"cham", 358},
        {"cher", 445},
        {"chrs", 109},
        {"cirt", 291},
        {"copt", 204},
        {"cpmn", 402},
        {"cprt", 403},
        {"cyrl", 220},
        {"cyrs", 221},
        {"deva", "dev2", 315},
        {"diak", 342},
        {"dogr", 328},
        {"dsrt", 250},
        {"dupl", 755},
        {"egyd", 70},
        {"egyh", 60},
        {"egyp", 50},
        {"elba", 226},
        {"elym", 128},
        {"ethi", 430},
        {"geok", 241},
        {"geor", 240},
        {"glag", 225},
        {"gong", 312},
        {"gonm", 313},
        {"goth", 206},
        {"gran", 343},
        {"grek", 200},
        {"gujr", "gjr2", 320},
        {"guru", "gur2", 310},
        {"hanb", 503},
        {"hang", 286},
        {"hani", 500},
        {"hano", 371},
        {"hans", 501},
        {"hant", 502},
        {"hatr", 127},
        {"hebr", 125},
        {"hira", 410},
        {"hluw", 80},
        {"hmng", 450},
        {"hmnp", 451},
        {"hrkt", 412},
        {"hung", 176},
        {"inds", 610},
        {"ital", 210},
        {"jamo", 284},
        {"java", 361},
        {"jpan", 413},
        {"jurc", 510},
        {"kali", 357},
        {"kana", 411},
        {"khar", 305},
        {"khmr", 355},
        {"khoj", 322},
        {"kitl", 505},
        {"kits", 288},
        {"knda", "knd2", 345},
        {"kore", 287},
        {"kpel", 436},
        {"kthi", 317},
        {"lana", 351},
        {"laoo", "lao ", 356},
        {"latf", 217},
        {"latg", 216},
        {"latn", 215},
        {"leke", 364},
        {"lepc", 335},
        {"limb", 336},
        {"lina", 400},
        {"linb", 401},
        {"lisu", 399},
        {"loma", 437},
        {"lyci", 202},
        {"lydi", 116},
        {"mahj", 314},
        {"maka", 366},
        {"mand", 140},
        {"mani", 139},
        {"marc", 332},
        {"maya", 90},
        {"medf", 265},
        {"mend", 438},
        {"merc", 101},
        {"mero", 100},
        {"mlym", "mlm2", 347},
        {"modi", 324},
        {"mong", 145},
        {"moon", 218},
        {"mroo", 264},
        {"mtei", 337},
        {"mult", 323},
        {"mymr", "mym2", 350},
        {"nand", 311},
        {"narb", 106},
        {"nbat", 159},
        {"newa", 333},
        {"nkdb", 85},
        {"nkgb", 420},
        {"nkoo", "nko ", 165},
        {"nshu", 499},
        {"ogam", 212},
        {"olck", 261},
        {"orkh", 175},
        {"orya", "ory2", 327},
        {"osge", 219},
        {"osma", 260},
        {"ougr", 143},
        {"palm", 126},
        {"pauc", 263},
        {"pcun", 15},
        {"pelm", 16},
        {"perm", 227},
        {"phag", 331},
        {"phli", 131},
        {"phlp", 132},
        {"phlv", 133},
        {"phnx", 115},
        {"plrd", 282},
        {"piqd", 293},
        {"prti", 130},
        {"psin", 103},
        {"qaaa", 900},
        {"qabv", "byzm", 947}, // Open-type
        {"qabw", "musc", 948}, // Open-type
        {"qabx", 949},
        {"ranj", 303},
        {"rjng", 363},
        {"rohg", 167},
        {"roro", 620},
        {"runr", 211},
        {"samr", 123},
        {"sara", 292},
        {"sarb", 105},
        {"saur", 344},
        {"sgnw", 95},
        {"shaw", 281},
        {"shrd", 319},
        {"shui", 530},
        {"sidd", 302},
        {"sind", 318},
        {"sinh", 348},
        {"sogd", 141},
        {"sogo", 142},
        {"sora", 398},
        {"soyo", 329},
        {"sund", 362},
        {"sylo", 316},
        {"syrc", 135},
        {"syre", 138},
        {"syrj", 137},
        {"syrn", 136},
        {"tagb", 373},
        {"takr", 321},
        {"tale", 353},
        {"talu", 354},
        {"taml", "tml2", 346},
        {"tang", 520},
        {"tavt", 359},
        {"telu", "tel2", 340},
        {"teng", 290},
        {"tfng", 120},
        {"tglg", 370},
        {"thaa", 170},
        {"thai", 352},
        {"tibt", 330},
        {"tirh", 326},
        {"tnsa", 275},
        {"toto", 294},
        {"ugar", 40},
        {"vaii", "vai ", 470},
        {"visp", 280},
        {"vith", 228},
        {"wara", 262},
        {"wcho", 283},
        {"wole", 480},
        {"xpeo", 30},
        {"xsux", 20},
        {"yezi", 192},
        {"yiii", "yi  ", 460},
        {"zanb", 339},
        {"zinh", 994},
        {"zmth", "math", 995},
        {"zsye", 993},
        {"zsym", 996},
        {"zxxx", 997},
        {"zyyy", "DFLT", 998},
        {"zzzz", 999}};
    // clang-format on

    constexpr auto data_size = sizeof(data) / sizeof(data[0]);

    auto r = std::array<iso_15924_info, data_size>{};

    for (auto i = 0_uz; i != data_size; ++i) {
        r[i] = data[i];
    }

    return r;
}

constexpr auto iso_15924_infos = iso_15924_infos_init();

[[nodiscard]] consteval auto iso_15924_code4_by_number_init() noexcept
{
    auto r = std::array<fixed_string<4>, 1000>{};

    for (auto const& info : iso_15924_infos) {
        r[info.number] = info.code4;
    }

    return r;
}

[[nodiscard]] consteval auto iso_15924_code4_open_type_by_number_init() noexcept
{
    auto r = std::array<fixed_string<4>, 1000>{};

    for (auto const& info : iso_15924_infos) {
        r[info.number] = info.code4_open_type;
    }

    return r;
}

[[nodiscard]] consteval auto iso_15924_number_by_code4_init() noexcept
{
    constexpr auto array_size = std::tuple_size_v<decltype(iso_15924_infos)>;
    using record_type = std::pair<fixed_string<4>, uint16_t>;

    auto r = std::array<record_type, array_size>{};
    for (auto i = 0_uz; i != iso_15924_infos.size(); ++i) {
        r[i] = {iso_15924_infos[i].code4, iso_15924_infos[i].number};
    }
    std::sort(r.begin(), r.end(), [](auto const& a, auto const& b) {
        return a.first < b.first;
    });

    return r;
}

constexpr auto iso_15924_code4_by_number = iso_15924_code4_by_number_init();
constexpr auto iso_15924_code4_open_type_by_number = iso_15924_code4_open_type_by_number_init();
constexpr auto iso_15924_number_by_code4 = iso_15924_number_by_code4_init();

} // namespace detail

constexpr iso_15924::iso_15924(std::string_view code4)
{
    if (code4.size() != 4) {
        throw parse_error(std::format("Invalid script '{}'", code4));
    }

    auto const code4_ = to_title(code4);

    auto const it = std::lower_bound(
        detail::iso_15924_number_by_code4.begin(),
        detail::iso_15924_number_by_code4.end(),
        code4_,
        [](auto const& item, auto const& value) {
            return item.first < value;
        });

    if (it == detail::iso_15924_number_by_code4.end() or it->first != code4_) {
        throw parse_error(std::format("Unknown script '{}'", code4));
    }

    _v = it->second;
}

[[nodiscard]] constexpr std::string iso_15924::code4() const noexcept
{
    hi_assert(_v < 1000);
    return detail::iso_15924_code4_by_number[_v];
}

[[nodiscard]] constexpr uint32_t iso_15924::code4_open_type() const noexcept
{
    hi_assert(_v < 1000);
    return fourcc(detail::iso_15924_code4_open_type_by_number[_v]);
}

[[nodiscard]] constexpr bool iso_15924::left_to_right() const noexcept
{
    switch (_v) {
    case 50: // Hyro
    case 105: // Sarb
    case 106: // Narb
    case 115: // Phnx
    case 116: // Lydi
    case 123: // Samr
    case 124: // Armi
    case 125: // Hebr
    case 126: // Palm
    case 127: // Hatr
    case 130: // Prti
    case 131: // Phli
    case 132: // Phlp
    case 133: // Phlv
    case 134: // Evst
    case 135: // Syrc
    case 136: // Syrn
    case 137: // Syrj
    case 138: // Syre
    case 140: // Mand
    case 141: // Sogd
    case 142: // Sogo
    case 159: // Nbat
    case 160: // Arab
    case 161: // Aran
    case 165: // Nkoo
    case 166: // Adlm
    case 167: // Rohg
    case 170: // Thaa
    case 175: // Orkh
    case 176: // Hung
    case 192: // Yezi
    case 210: // Ital
    case 305: // Khar
    case 403: // Cprt
    case 438: // Mend
    case 495: // Ethi
    case 610: // Inds
        return false;
    default:
        return true;
    }
}
}} // namespace hi::v1
