// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "iso_3166_intf.hpp"
#include "../utility/utility.hpp"
#include "../algorithm/algorithm.hpp"
#include "../macros.hpp"
#include <array>
#include <algorithm>
#include <format>

hi_export_module(hikogui.i18n.iso_3166 : impl);

hi_export namespace hi::inline v1 {
namespace detail {

struct iso_3166_info {
    fixed_string<2> code2;
    fixed_string<3> code3;
    uint16_t number;
};

/** Information about language codes.
 *
 * This is a c-style array since compilers, tools and analyzers do not handle large std::array constructors.
 */
[[nodiscard]] consteval auto iso_3166_infos_init() noexcept
{
    // We are using a c-style array to std::array conversion because
    // compilers, tools and analysers do not handle large std::array constructors.

    // clang-format off
    constexpr iso_3166_info data[] = {
        {"AF", "AFG", 4},
        {"AL", "ALB", 8},
        {"AQ", "ATA", 10},
        {"DZ", "DZA", 12},
        {"AS", "ASM", 16},
        {"AD", "AND", 20},
        {"AO", "AGO", 24},
        {"AG", "ATG", 28},
        {"AZ", "AZE", 31},
        {"AR", "ARG", 32},
        {"AU", "AUS", 36},
        {"AT", "AUT", 40},
        {"BS", "BHS", 44},
        {"BH", "BHR", 48},
        {"BD", "BGD", 50},
        {"AM", "ARM", 51},
        {"BB", "BRB", 52},
        {"BE", "BEL", 56},
        {"BM", "BMU", 60},
        {"BT", "BTN", 64},
        {"BO", "BOL", 68},
        {"BA", "BIH", 70},
        {"BW", "BWA", 72},
        {"BV", "BVT", 74},
        {"BR", "BRA", 76},
        {"BZ", "BLZ", 84},
        {"IO", "IOT", 86},
        {"SB", "SLB", 90},
        {"VG", "VGB", 92},
        {"BN", "BRN", 96},
        {"BG", "BGR", 100},
        {"MM", "MMR", 104},
        {"BI", "BDI", 108},
        {"BY", "BLR", 112},
        {"KH", "KHM", 116},
        {"CM", "CMR", 120},
        {"CA", "CAN", 124},
        {"CV", "CPV", 132},
        {"KY", "CYM", 136},
        {"CF", "CAF", 140},
        {"LK", "LKA", 144},
        {"TD", "TCD", 148},
        {"CL", "CHL", 152},
        {"CN", "CHN", 156},
        {"TW", "TWN", 158},
        {"CX", "CXR", 162},
        {"CC", "CCK", 166},
        {"CO", "COL", 170},
        {"KM", "COM", 174},
        {"YT", "MYT", 175},
        {"CG", "COG", 178},
        {"CD", "COD", 180},
        {"CK", "COK", 184},
        {"CR", "CRI", 188},
        {"HR", "HRV", 191},
        {"CU", "CUB", 192},
        {"CY", "CYP", 196},
        {"CZ", "CZE", 203},
        {"BJ", "BEN", 204},
        {"DK", "DNK", 208},
        {"DM", "DMA", 212},
        {"DO", "DOM", 214},
        {"EC", "ECU", 218},
        {"SV", "SLV", 222},
        {"GQ", "GNQ", 226},
        {"ET", "ETH", 231},
        {"ER", "ERI", 232},
        {"EE", "EST", 233},
        {"FO", "FRO", 234},
        {"FK", "FLK", 238},
        {"GS", "SGS", 239},
        {"FJ", "FJI", 242},
        {"FI", "FIN", 246},
        {"AX", "ALA", 248},
        {"FR", "FRA", 250},
        {"GF", "GUF", 254},
        {"PF", "PYF", 258},
        {"TF", "ATF", 260},
        {"DJ", "DJI", 262},
        {"GA", "GAB", 266},
        {"GE", "GEO", 268},
        {"GM", "GMB", 270},
        {"PS", "PSE", 275},
        {"DE", "DEU", 276},
        {"GH", "GHA", 288},
        {"GI", "GIB", 292},
        {"KI", "KIR", 296},
        {"GR", "GRC", 300},
        {"GL", "GRL", 304},
        {"GD", "GRD", 308},
        {"GP", "GLP", 312},
        {"GU", "GUM", 316},
        {"GT", "GTM", 320},
        {"GN", "GIN", 324},
        {"GY", "GUY", 328},
        {"HT", "HTI", 332},
        {"HM", "HMD", 334},
        {"VA", "VAT", 336},
        {"HN", "HND", 340},
        {"HK", "HKG", 344},
        {"HU", "HUN", 348},
        {"IS", "ISL", 352},
        {"IN", "IND", 356},
        {"ID", "IDN", 360},
        {"IR", "IRN", 364},
        {"IQ", "IRQ", 368},
        {"IE", "IRL", 372},
        {"IL", "ISR", 376},
        {"IT", "ITA", 380},
        {"CI", "CIV", 384},
        {"JM", "JAM", 388},
        {"JP", "JPN", 392},
        {"KZ", "KAZ", 398},
        {"JO", "JOR", 400},
        {"KE", "KEN", 404},
        {"KP", "PRK", 408},
        {"KR", "KOR", 410},
        {"KW", "KWT", 414},
        {"KG", "KGZ", 417},
        {"LA", "LAO", 418},
        {"LB", "LBN", 422},
        {"LS", "LSO", 426},
        {"LV", "LVA", 428},
        {"LR", "LBR", 430},
        {"LY", "LBY", 434},
        {"LI", "LIE", 438},
        {"LT", "LTU", 440},
        {"LU", "LUX", 442},
        {"MO", "MAC", 446},
        {"MG", "MDG", 450},
        {"MW", "MWI", 454},
        {"MY", "MYS", 458},
        {"MV", "MDV", 462},
        {"ML", "MLI", 466},
        {"MT", "MLT", 470},
        {"MQ", "MTQ", 474},
        {"MR", "MRT", 478},
        {"MU", "MUS", 480},
        {"MX", "MEX", 484},
        {"MC", "MCO", 492},
        {"MN", "MNG", 496},
        {"MD", "MDA", 498},
        {"ME", "MNE", 499},
        {"MS", "MSR", 500},
        {"MA", "MAR", 504},
        {"MZ", "MOZ", 508},
        {"OM", "OMN", 512},
        {"NA", "NAM", 516},
        {"NR", "NRU", 520},
        {"NP", "NPL", 524},
        {"NL", "NLD", 528},
        {"CW", "CUW", 531},
        {"AW", "ABW", 533},
        {"SX", "SXM", 534},
        {"BQ", "BES", 535},
        {"NC", "NCL", 540},
        {"VU", "VUT", 548},
        {"NZ", "NZL", 554},
        {"NI", "NIC", 558},
        {"NE", "NER", 562},
        {"NG", "NGA", 566},
        {"NU", "NIU", 570},
        {"NF", "NFK", 574},
        {"NO", "NOR", 578},
        {"MP", "MNP", 580},
        {"UM", "UMI", 581},
        {"FM", "FSM", 583},
        {"MH", "MHL", 584},
        {"PW", "PLW", 585},
        {"PK", "PAK", 586},
        {"PA", "PAN", 591},
        {"PG", "PNG", 598},
        {"PY", "PRY", 600},
        {"PE", "PER", 604},
        {"PH", "PHL", 608},
        {"PN", "PCN", 612},
        {"PL", "POL", 616},
        {"PT", "PRT", 620},
        {"GW", "GNB", 624},
        {"TL", "TLS", 626},
        {"PR", "PRI", 630},
        {"QA", "QAT", 634},
        {"RE", "REU", 638},
        {"RO", "ROU", 642},
        {"RU", "RUS", 643},
        {"RW", "RWA", 646},
        {"BL", "BLM", 652},
        {"SH", "SHN", 654},
        {"KN", "KNA", 659},
        {"AI", "AIA", 660},
        {"LC", "LCA", 662},
        {"MF", "MAF", 663},
        {"PM", "SPM", 666},
        {"VC", "VCT", 670},
        {"SM", "SMR", 674},
        {"ST", "STP", 678},
        {"SA", "SAU", 682},
        {"SN", "SEN", 686},
        {"RS", "SRB", 688},
        {"SC", "SYC", 690},
        {"SL", "SLE", 694},
        {"SG", "SGP", 702},
        {"SK", "SVK", 703},
        {"VN", "VNM", 704},
        {"SI", "SVN", 705},
        {"SO", "SOM", 706},
        {"ZA", "ZAF", 710},
        {"ZW", "ZWE", 716},
        {"ES", "ESP", 724},
        {"SS", "SSD", 728},
        {"SD", "SDN", 729},
        {"EH", "ESH", 732},
        {"SR", "SUR", 740},
        {"SJ", "SJM", 744},
        {"SZ", "SWZ", 748},
        {"SE", "SWE", 752},
        {"CH", "CHE", 756},
        {"SY", "SYR", 760},
        {"TJ", "TJK", 762},
        {"TH", "THA", 764},
        {"TG", "TGO", 768},
        {"TK", "TKL", 772},
        {"TO", "TON", 776},
        {"TT", "TTO", 780},
        {"AE", "ARE", 784},
        {"TN", "TUN", 788},
        {"TR", "TUR", 792},
        {"TM", "TKM", 795},
        {"TC", "TCA", 796},
        {"TV", "TUV", 798},
        {"UG", "UGA", 800},
        {"UA", "UKR", 804},
        {"MK", "MKD", 807},
        {"EG", "EGY", 818},
        {"GB", "GBR", 826},
        {"GG", "GGY", 831},
        {"JE", "JEY", 832},
        {"IM", "IMN", 833},
        {"TZ", "TZA", 834},
        {"US", "USA", 840},
        {"VI", "VIR", 850},
        {"BF", "BFA", 854},
        {"UY", "URY", 858},
        {"UZ", "UZB", 860},
        {"VE", "VEN", 862},
        {"WF", "WLF", 876},
        {"WS", "WSM", 882},
        {"YE", "YEM", 887},
        {"ZM", "ZMB", 894}};
    // clang-format on
    constexpr auto data_size = sizeof(data) / sizeof(data[0]);

    auto r = std::array<iso_3166_info, data_size>{};

    for (auto i = 0_uz; i != data_size; ++i) {
        r[i] = data[i];
    }

    return r;
}

constexpr auto iso_3166_infos = iso_3166_infos_init();

[[nodiscard]] consteval auto iso_3166_code2_by_number_init() noexcept
{
    auto r = std::array<fixed_string<2>, 1000>{};
    for (auto i = 0_uz; i != r.size(); ++i) {
        r[i] = "ZZ";
    }

    for (auto const& info : iso_3166_infos) {
        r[info.number] = info.code2;
    }
    return r;
}

[[nodiscard]] consteval auto iso_3166_code3_by_number_init() noexcept
{
    auto r = std::array<fixed_string<3>, 1000>{};
    for (auto i = 0_uz; i != r.size(); ++i) {
        r[i] = "ZZZ";
    }

    for (auto const& info : iso_3166_infos) {
        r[info.number] = info.code3;
    }
    return r;
}

[[nodiscard]] consteval auto iso_3166_number_by_code2_init() noexcept
{
    constexpr auto size = std::tuple_size_v<decltype(iso_3166_infos)>;
    using type = std::pair<fixed_string<2>, uint16_t>;

    auto r = std::array<type, size>{};
    for (auto i = 0_uz; i != iso_3166_infos.size(); ++i) {
        auto const& info = iso_3166_infos[i];
        r[i] = {info.code2, info.number};
    }

    std::sort(r.begin(), r.end(), [](auto const& a, auto const& b) {
        return a.first < b.first;
    });
    return r;
}

[[nodiscard]] consteval auto iso_3166_number_by_code3_init() noexcept
{
    constexpr auto size = std::tuple_size_v<decltype(iso_3166_infos)>;
    using type = std::pair<fixed_string<3>, uint16_t>;

    auto r = std::array<type, size>{};
    for (auto i = 0_uz; i != iso_3166_infos.size(); ++i) {
        auto const& info = iso_3166_infos[i];
        r[i] = {info.code3, info.number};
    }

    std::sort(r.begin(), r.end(), [](auto const& a, auto const& b) {
        return a.first < b.first;
    });
    return r;
}

constexpr auto iso_3166_code2_by_number = iso_3166_code2_by_number_init();
constexpr auto iso_3166_code3_by_number = iso_3166_code3_by_number_init();
constexpr auto iso_3166_number_by_code2 = iso_3166_number_by_code2_init();
constexpr auto iso_3166_number_by_code3 = iso_3166_number_by_code3_init();

} // namespace detail

constexpr iso_3166::iso_3166(std::string_view str)
{
    if (is_digit(str)) {
        if (auto code = from_string<uint16_t>(str)) {
            _v = *code;
        } else {
            throw parse_error(std::format("Could not parse ISO-3166 code '{}'", str));
        }
        hi_check(_v < 1000, "ISO-3166 number must be between 000 and 999, got '{}'", _v);

    } else if (str.size() == 2) {
        auto str_up = to_upper(str);

        auto const it = std::lower_bound(
            detail::iso_3166_number_by_code2.begin(),
            detail::iso_3166_number_by_code2.end(),
            str_up,
            [](auto const& item, auto const& value) {
                return item.first < value;
            });

        hi_check(
            it != detail::iso_3166_number_by_code2.end() and it->first == str_up,
            "Could not find ISO-3166 2 letter language code '{}'",
            str);

        _v = it->second;

    } else if (str.size() == 3) {
        auto str_up = to_upper(str);

        auto const it = std::lower_bound(
            detail::iso_3166_number_by_code3.begin(),
            detail::iso_3166_number_by_code3.end(),
            str_up,
            [](auto const& item, auto const& value) {
                return item.first < value;
            });

        hi_check(
            it != detail::iso_3166_number_by_code3.end() and it->first == str_up,
            "Could not find ISO-3166 3 letter language code '{}'",
            str);

        _v = it->second;

    } else {
        throw parse_error(std::format("Could not parse ISO-3166 code '{}'", str));
    }
}

[[nodiscard]] constexpr std::string iso_3166::code2() const noexcept
{
    hi_assert(_v < 1000);
    return detail::iso_3166_code2_by_number[_v];
}

[[nodiscard]] constexpr std::string iso_3166::code3() const noexcept
{
    hi_assert(_v < 1000);
    return detail::iso_3166_code3_by_number[_v];
}

} // namespace hi::inline v1
