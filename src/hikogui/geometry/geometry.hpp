// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "alignment.hpp" // export
#include "axis.hpp" // export
#include "aarectangle.hpp" // export
#include "circle.hpp" // export
#include "corner_radii.hpp" // export
#include "extent2.hpp" // export
#include "extent3.hpp" // export
#include "line_end_cap.hpp" // export
#include "line_join_style.hpp" // export
#include "line_segment.hpp" // export
#include "lookat.hpp" // export
#include "margins.hpp" // export
#include "matrix2.hpp" // export
#include "matrix3.hpp" // export
#include "object_fit.hpp" // export
#include "perspective.hpp" // export
#include "point2.hpp" // export
#include "point3.hpp" // export
#include "quad.hpp" // export
#include "rectangle.hpp" // export
#include "rotate2.hpp" // export
#include "rotate3.hpp" // export
#include "scale2.hpp" // export
#include "scale3.hpp" // export
#include "transform.hpp" // export
#include "translate2.hpp" // export
#include "translate3.hpp" // export
#include "vector2.hpp" // export
#include "vector3.hpp" // export

hi_export_module(hikogui.geometry);

hi_export namespace hi {
inline namespace v1 {
/**
\defgroup geometry Geometry

Low level geometry types
------------------------

### numeric\_array<T,N>

The `simd` is an array of numbers, with many mathematical operations
on the array of numbers. The `simd` is designed to be useable in
constexpr and easily to vectorize by the optimizer.

### f32x4

The `f32x4` is an `simd<float,4>`.

Many of the operations on a `f32x4` are hand optimized using the intel intrinsics on SSE registers.

High level geometry type
------------------------

### geo::vector<D>

A vector is a direction and distance.

When transforming a vector, only scale, rotation and shear have effect.

Both `vector2` and `vector3` are implemented as a `f32x4` homogeneous 4D coordinate with w = 0.0.

### geo::point<D>

A point is a location in space.

A point can be transformed in the same way as a vector and also be translated.

Both `point2` and `point3` are implemented as a `f32x4` homogeneous 4D coordinate with w = 1.0.

### geo::extent<D>

An extent is a width, height and depth.

An extent can be transformed like a vector.

Both `extent2` and `extent3` are implemented as a `f32x4` homogeneous 4D coordinate with w = 0.0.

### corner\_shapes

Corner shapes are 4 floating point numbers, one for the corner in the left-bottom, right-bottom,
left-top and right-top corner. Each number has the following meaning:

 - `0.0` Sharp corner,
 - `>0.0` Radius of a rounded corner,
 - `<0.0` Radius of a cut corner.

### color

A 4D red, green, blue and alpha value. A `color` can be transformed like a `vector3`.
In this case, the alpha value is ignored and copied into the result.

### rectangle

A `rectangle` is a closed plane in three dimensions.

A `rectangle` can be transformed like a `point3`.

It should be implemented as a `point3` in the left-bottom corner and two `vector3`s to the upper and right corners.
However, it is currently implemented as 4 points, one for each corner.

### aarectangle

The `aarectangle` class is a 2D axis-aligned rectangle.

When transforming an axis aligned rectangle in 3D or with rotation, the result will be a normal `rectangle`.
A rectangle can be converted back to an `aarectangle`, as a bounding rectangle around the transformed rectangle.

An `aarectangle` is implemented as a `f32x4` where:

 - `x`: left-bottom point x
 - `y`: left-bottom point y
 - `z`: right-top point x
 - `w`: right-top point y

Transformation types
--------------------

### geo::translate<D>

### geo::scale<D>

### geo::rotate<D>

### geo::matrix<D>

The `mat` class is a 4x4 homogeneous transformation matrix in column-major
order. Internally, this a `vec` for each column.

Vector * matrix multiplications are performed as if the vector is a column.

### geo::transform

Coordinates
-----------

All origins (0, 0, 0) are at the bottom-left-far corner. With the x-axis pointing
right, y-axis pointing up, z-axis pointing toward the camera. This is the same coordinate
system as OpenGL; a right-handed coordinate system.

```
           +y   
           |   -z (away from camera)
           |  /
           | /
           |/
    -x ----+---- +x
          /|
         / |
        /  |
      +z   |
           -y
```


This is true for:

 - Window
 - Images
 - Paths
 - Font-glyphs

### Window and Widget-surface coordinates

The coordinates in a window and widgets  are in pixels.

With the center of the left bottom pixel having the coordinates (0.5, 0.5).

The alignment of borders to pixels is the responsibility of the
widget that draws itself. The widget has access to
the `draw_context::draw_box_including_border()` function to position the border's
edge to the edge of the given rectangle, if the rectangle is rounded to
integer coordinates; the rectangle, border and pixel will share the same edge.

### Window depth

Z-coordinate for a window is between `0.0` (far) to `100.0` (near).

For better precision we use a reverse-z method, to combine
1/z together with float with linear precision.

 - The window-widget is set to depth `0.0`.
 - Each nested widget, which needs to draw itself, is `1.0` nearer.
 - Widgets which extends across other widgets, such as a combo-box-widget
   will be `25.0` nearer.

### Image coordinates

The coordinates in an image are in pixels. With the center of the left bottom
pixel having the coordinates (0.5, 0.5).

### Path coordinates

The coordinates in a path are free from scale and origin, when rendering a
path to an image, the path should be first be transformed to image coordinates.

### Glyph/Font coordinates

Glyph coordinates are in the number of Em units.
X-axis pointing right, Y-axis pointing up.
The origin (0,0) is on the crossing of the base line and left side bearing.
Vertices of front faces are specified in counter clockwise order.

Corners
-------

Corners are enumerated as follows:

 - `0`: far bottom left
 - `1`: far bottom right
 - `2`: far top left
 - `3`: far top right
 - `4`: near bottom left
 - `5`: near bottom right
 - `6`: near top left
 - `7`: near top right

When corners are packed in a 4D vector:

 - `x`: bottom-left
 - `y`: bottom-right
 - `z`: top-left
 - `w`: top-right

Edges
-----

When edges are packed in a 4D vector:

 - `x`: bottom-edge
 - `y`: left-edge
 - `z`: top-edge
 - `w`: right-edge

Triangles
---------

A front facing triangle has the vertex ordered in counter-clockwise direction.

Quads
-----

Quads are defined as two front facing triangles.

The vertex index order is: `0, 1, 2, 2, 1, 3`.

As illustrated below:

```text
2 <--- 3
| \    ^
|  \ B |
| A \  |
v    \ |
0 ---> 1
```

*/
}}
