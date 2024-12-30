Widget Layout
=============

The first rule of layout, a widget will never be asked to draw itself on a size
smaller than the minimum-size constraint. To guarantee this the widget should
not change its internal state between constrain, layout and drawing; if that
state could change the constrain size of the widget.

The layout of widgets is ordered: vertically first, horizontally second.

The height of widgets in a single row is determined as follows:
 - The row's minimum-height is the maximum of each widget's minimum-height.
 - The row's preferred-height is the maximum of each widget's preferred-height.

For a label the minimum-height is the natural height of the text without any
folding. The preferred-height is the height of the text when it is folded based
on the width given in the theme. For many other widgets the minimum-height is
the smallest it can be drawn, and the preferred-height is set to the height in
the theme.

The width of widgets in a single column is determined as follows:
 - The column's minimum-width is the maximum of each widget's minimum-width;
   the minimum-width of a widget is calculated using a lambda function with as
   the argument the row's minimum-height.
 - The column's preferred-width is the maximum of each widget's preferred-width.

For a label widget the minimum-width lambda will fold the text for the specific
height and return the width of that text. For most other widget the
minimum-width lambda will return the minimum draw width without regard of the
row's minimum-height. The preferred width is set to the width in the theme.

For nested widgets the minimum-width lambda will need to be nested to include
padding and margins.

widgets also have a height-weight and width-weight:
 - If set to zero, the widget will not receive extra-size in those dimensions
   when the window is made larger.
 - If set to a positive value, then the value is used as a weight to distribute
   the extra-size between the widgets along those axis.
 - If all widgets along the axis have a zero weight then the window can not be
   made larger along those axis.

Since there is no maximum width or height of widgets, this means when the window
is allowed to resize, the window is only limited to the size of the virtual
screen.

A widget also has a baseline which is used to align the widget's drawing. The
baseline for widgets in a row is determined by the priority of each baseline.
For baselines with equal priority the first most widget's baseline is chosen.

Because depending the alignment of the (winning) widget the baseline is
calculated from the top, bottom or middle of the widget; the baseline is a
lambda with the height of the widget as the argument. When nesting widgets
the baseline lambda is nested, to include padding and margins.

When laying out the widget, the container will execute the lambda with the row's
height and determine the location of the baseline and pass it to the child
widgets of that row.

Widgets should use the baseline to align the drawing or text. The widget may use
different strategies of aligning other parts of the drawing or text to this
baseline, or completely ignoring the baseline, if by natural aligning the
drawing would be clipped outside the widget.

To determine which baseline is used in a row when a col-span is involved,
the vertical alignment of the grid widget is used.
