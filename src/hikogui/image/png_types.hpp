

#pragma once

#include "../macros.hpp"

hi_export_module(hikogui.image : png_types);

hi_export namespace hi::inline v1 {

/** How is the png-image interlaces.
 */
enum class png_interlace_type { none, adam7 };

/** The color type of the png-image.
 */
enum class png_color_type { gray, gray_alpha, palette, rgb, rgb_alpha };

/** The standard color profiles which requires special handling.
 */
enum class png_iccp_profile {
    /** Unknown color profile.
     * 
     * This profile is treated as follows:
     *  - A RGB color space without unknow primaries. The color primaries are
     *    expected to be encoded in the png file, or by default to sRGB.
     *  - A unknown transfer function, the png decoder is expected to use the
     *    transfer function encoded in the png file, or by default to sRGB-like
     *    gamma curve.
     *  - The peak luminance is 80 cd/m2.
     */
    unknown,

    /** The ITUR 2100 PQ FULL color profile.
     * 
     * This profile is treated as follows:
     *  - The RGB color space is encoded in the ITU-R BT.2020 color space.
     *  - The transfer function is the ITU-R BT.2100 PQ transfer function.
     *  - The peak luminance is 1000 cd/m2.
     */
    ITUR_2100_PQ_FULL
};

}
