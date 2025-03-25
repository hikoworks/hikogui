// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "audio/audio.hpp" // export
#include "algorithm/algorithm.hpp" // export
#include "char_maps/char_maps.hpp" // export
#include "codec/codec.hpp" // export
#include "color/color.hpp" // export
#include "concurrency/concurrency.hpp" // export
#include "container/container.hpp" // export
#include "crt/crt.hpp" // export
#include "file/file.hpp" // export
#include "font/font.hpp" // export
#include "geometry/geometry.hpp" // export
#include "GFX/GFX.hpp" // export
#include "graphic_path/graphic_path.hpp" // export
#include "GUI/GUI.hpp" // export
#include "i18n/i18n.hpp" // export
#include "image/image.hpp" // export
#include "l10n/l10n.hpp" // export
#include "dispatch/dispatch.hpp" // export
#include "layout/layout.hpp" // export
#include "memory/memory.hpp" // export
#include "net/net.hpp" // export
#include "numeric/numeric.hpp" // export
#include "observer/observer.hpp" // export
#include "parser/parser.hpp" // export
#include "path/path.hpp" // export
#include "random/random.hpp" // export
#include "security/security.hpp" // export
#include "settings/settings.hpp" // export
#include "telemetry/telemetry.hpp" // export
#include "text/text.hpp" // export
#include "time/time.hpp" // export
#include "unicode/unicode.hpp" // export
#include "units/units.hpp" // export
#include "utility/utility.hpp" // export
#include "widgets/widgets.hpp" // export
#include "win32/win32.hpp" // export

hi_export_module(hikogui);

//#include "command_line.hpp"
// Never include "crt.hpp"

/** @file module.hpp
* 
* 
* Dependencies:
*  - utility: -
*  - SIMD: utility
*  - geometry: SIMD, utility, concurrency
*  - color: geometry, SIMD, utility
*  - image: geometry, SIMD, color, utility
*  - char_maps: utility
* 
*/

/** @namespace hi The HikoGUI namespace.
 */
hi_export namespace hi {

/** @namespace hi::v1 The HikoGUI API version 1.
 */
inline namespace v1 {

} // namespace v1
} // namespace hi

/** @namespace v1 DOXYGEN BUG
 * @brief Doxygen can't handle `namespace hi::inline v1 {}` syntax.
 * All files should be changed to use old-style inline namespace syntax.
 */
namespace v1{}
