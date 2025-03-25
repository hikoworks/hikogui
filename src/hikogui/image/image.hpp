// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "image_loader.hpp" // export
#include "pixmap_span.hpp" // export
#include "pixmap.hpp" // export
#include "png_loader.hpp" // export
#include "sdf_r8.hpp" // export
#include "sfloat_rg32.hpp" // export
#include "sfloat_rgb32.hpp" // export
#include "sfloat_rgba16.hpp" // export
#include "sfloat_rgba32.hpp" // export
#include "sfloat_rgba32x4.hpp" // export
#include "sint_abgr8_pack.hpp" // export
#include "snorm_r8.hpp" // export
#include "srgb_abgr8_pack.hpp" // export
#include "uint_abgr8_pack.hpp" // export
#include "unorm_a2bgr10_pack.hpp" // export

hi_export_module(hikogui.image);

hi_export namespace hi {
inline namespace v1 {
/**
\defgroup image Image

Image types and pixel formats.

Pixel formats
-------------

 | HikoGUI type         | Vulkan format                        | Comment                          |
 |:-------------------- |:------------------------------------ |:-------------------------------- |
 | `sfloat_rg32`        | `VK_FORMAT_R32G32_SFLOAT`            |                                  |
 | `sfloat_rgb32`       | `VK_FORMAT_R32G32B32_SFLOAT`         |                                  |
 | `sfloat_rgba16`      | `VK_FORMAT_R16G16B16A16_SFLOAT`      | The default HikoGUI image format.|
 | `sfloat_rgba32`      | `VK_FORMAT_R32G32B32A32_SFLOAT`      |                                  |
 | `sfloat_rgba32x4`    | 4 x `VK_FORMAT_R32G32B32A32_SFLOAT`  | Used for 4x4 matrix              |
 | `snorm_r8`           | `VK_FORMAT_R8_SNORM`                 |                                  |
 | `int_abgr8_pack`     | `VK_FORMAT_A8B8G8R8_SINT_PACK32`     |                                  |
 | `uint_abgr8_pack`    | `VK_FORMAT_A8B8G8R8_UINT_PACK32`     |                                  |
 | `srgb_abgr8_pack`    | `VK_FORMAT_A8B8G8R8_SRGB_PACK32`     |                                  |
 | `unorm_a2bgr10_pack` | `VK_FORMAT_A2R10G10B10_UNORM_PACK32` |                                  |
 | `sdf_r8`             | `VK_FORMAT_R8_SNORM`                 | To store signed-distance-field.  |

Naming
------

Color format types are used in vertex arrays and images when communicating with the GPU.

The types have the following syntax: `numeric-type` \_ `color-components` [ \_pack ]

#### Numeric type

 | Type  | Description                                                              |
 |:------|:-------------------------------------------------------------------------|
 | snorm | Signed integer mapped to the floating point range between -1.0 and 1.0.  |
 | unorm | Unsigned integer mapped to the floating point range between 0.0 and 1.0. |
 | sint  | Signed integer.                                                          |
 | uint  | Unsigned integer.                                                        |
 | float | Floating point number.                                                   |
 | srgb  | Non-linear sRGB format for the RGB component, the alpha remains linear.  |

#### Color components

The color components are the lower case letters: r, g, b & a. The ordering of the letters describe
the order of the color components in memory. A number describes the number of bits of each component
before it.

Here are a few examples of components:

 | Combination | Description                                           |
 |:------------|:------------------------------------------------------|
 | rgba32      | 32 bits per component red, green, blue & alpha.       |
 | rgba16      | 16 bits per component red, green, blue & alpha.       |
 | rgba8       | 8 bits per component red, green, blue & alpha.        |
 | rgb8        | 8 bits per component red, green, blue.                |
 | rg8         | 8 bits per component red, green.                      |
 | r8          | 8 bits per component red.                             |
 | abgr8       | 8 bits per component alpha, blue, green & red.        |
 | a2bgr10     | 2 bit alpha, 10 bits per component blue, green & red  |

#### Packing

If the format is non-packed, then each color component must be 8, 16, 32 or 64 bits in size.
Each color component is stored in memory in native-byte-order, and the components are ordered
with in increasing memory addresses.

If the format is packed, then the color components are packed together in a single integer.
The color components are packed inside the integer from `msb` to `lsb`.
The integers are 8, 16, 32, 64 or 128 bits in size. The integer is stored in memory in native-byte-order.

*/
}}

