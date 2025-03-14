
#pragma once

#include "baseline.hpp" // export
#include "box_constraints.hpp" // export
#include "box_shape.hpp" // export
#include "constraints.hpp" // export
#include "grid_layout.hpp" // export
#include "grid.hpp" // export
#include "row_column_layout.hpp" // export
#include "spreadsheet_address.hpp" // export

hi_export_module(hikogui.layout);

hi_export namespace hi {
inline namespace v1 {
/**
\defgroup layout 2D layout algorithms

Layout algorithms
-----------------

 * `hi::grid_layout`: An algorithm that lays out boxes in rows and columns.
 * `hi::row_layout`: An algorithm that lays out boxes in a single row.
 * `hi::column_layout`: An algorithm that lays out boxes in a single column.
 * `hi::flex_layout`: An algorithm that lays out boxes next to each other, possibly flowing to a next line.

*/
}}
