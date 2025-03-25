// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "color_intf.hpp" // export
#include "color_impl.hpp" // export
#include "color_space.hpp" // export
#include "quad_color.hpp" // export
#include "Rec2020.hpp" // export
#include "Rec2100.hpp" // export
#include "sRGB.hpp" // export
#include "../macros.hpp"

hi_export_module(hikogui.color);

hi_export namespace hi { inline namespace v1 {

/**
\defgroup color Color

Color, color-spaces and transfer functions.

Usage of color spaces
---------------------

 | Usage            | Space | depth   | alpha          |
 |:-----------------|:------|:--------|:---------------|
 | draw API         | scRGB | half | straight       |
 | vertices         | scRGB | half | straight       |
 | images           | scRGB | half | pre-multiplied |
 | fragment shaders | scRGB | float32 | pre-multiplied |
 | frame buffers    | scRGB | half | pre-multiplied |
 | swap chain       | sRGB\*| uint8\* | pre-multiplied |

### PNG decoding

HikoGUI's PNG decoder will create an image of half RGBA values
in the scRGB color space. The RGB values in the resulting image are
pre-multiplied by the alpha.

The PNG decoder will use the color space and transfer
function information from the PNG data to do the conversion of
pixel values into the scRGB color space.

sRGB color space and transfer function is implied if color space
information is not available in the PNG data.

### Theme files

The colors in hikogui's theme file may be edited by users, there
are three different ways of specifying colors.

 | Format                        | Color space |
 |:------------------------------|:------------|
 | '#' 6 or 8 hex digits         | sRGB        |
 | 3 or 4 integers               | sRGB        |
 | 3 or 4 floating point numbers | scRGB       |

### Swap chain

The swap chain images are the ones that are presented to the
desktop compositor.

On windows 10 this will be 8-bit sRGB. On Windows 10 float-16 scRGB is
possible but when a single application request a scRGB swap chain the whole desktop
will switch to HDR/high gamut mode. This mode switch will also cause the graphics card and
monitor to switch to this mode and likely they have different calibrations associated with it;
very likely the user has never calibrated this and the experience will be extremely jarring.
For this reason on Windows 10 HikoGUI will not by default request a float-16 scRGB swap chain.

On MacOS the swap chain will be in scRGB half. Since the desktop is always running HDR/high
gamut mode, there is no switching effect like on Windows 10.

scRGB color space
-----------------

scRGB is a HDR / high gamut color space. It is normally specified as
16 bit biased scaled integers with a range between -0.5 and 7.5 with
a linear transfer function. But this color space is also often used
with float 16; for GPU frame buffers and swap chains.

The scRGB color space has the same color primaries and white-point as the
sRGB/BT.709 color spaces.

 |           | x      | y      | Luminosity contribution |
 |:----------|-------:|-------:|------------------------:|
 | Red       | 0.6400 | 0.3300 |                  0.2126 |
 | Green     | 0.3000 | 0.6000 |                  0.7152 |
 | Blue      | 0.1500 | 0.0600 |                  0.0722 |
 | White D65 | 0.3127 | 0.3290 |                         |

The scRGB values have a linearly transfer function where scRGB(0.0, 0.0, 0.0)
is black and scRGB(1.0, 1.0, 1.0) is white with a luminance of 80 cd/m2.
When all scRGB component values are within 0.0 and 1.0 the scRGB color space
is identical to linear sRGB.

Color component values above 1.0 will produce higher luminance values available on HDR
monitors.

Negative color component values will allow colors outside of the sRGB gamut
triangle, which may be rendered on high gamut monitors. However the luminosity
of the color must be greater or equal to zero.

The color types
---------------

### color

The `color` type is a float-16 3D RGB vector + alpha scalar.

The RGB values are linear and may comprise the complete floating point range,
including values above 1.0 and value below zero.

The alpha value is a linear value between 0.0 (transparent) and 1.0 (opaque).

Depending on where the colors are used the RGB values are pre-multiplied by
the alpha value, for example: inside fragment shaders, images and frame buffers.

### matrix3

A color conversion matrix can be stored in the `matrix3` type. A `color` value can be transformed by a `matrix3`.

The transformation ignores and preserves the alpha value.

 */

}} // namespace hi::v1
