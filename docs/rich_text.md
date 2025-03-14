HikoGUI Rich Text
=================

Grapheme Cluster
----------------

Text shaping and editing is done on a per-grapheme-cluster basis,
graphemes clusters are defined in "UAX #29 Unicode Text Segmentation".

Inside Hikogui a `grapheme` can be used in a `std::basic_string`. A grapheme
is 21 bits, where grapheme clusters larger than one code-point get a temporary and unique-id
assigned to the invalid unicode planes 17-31.

Text Style
----------

A text-style is actually a set of styles for different text-categories, languages
and scripts. The text-category is a mask, the language may be empty meaning a wildcard;
and script may be 'zzzz' meaning a wildcard. This allows for at least one of the sub-styles
to be a fallback.

 - filter:
   + language code
   + script code
   + phrasing mask
 - font family id
 - font variant (weight/italic)
 - font size
 - color
 - lines (under / over / strike through).

Attributed Grapheme Cluster
---------------------------


 - [63:48] Language code.
 - [47:36] Text style id.
 - [35:32] Text category.
 - [31:0] Grapheme cluster.

### Phrasing

 --------------- | --- | ------------------
  Regular        | \\r | The default, neutral phrasing
  Emphasis       | \\e | Text with stressed emphasis. Spoken with clear articulation. Often shown in italic.
  Strong         | \\s | Text with more importance, warning, urgent. Spoken louder. Often shown in bold.
  Code           | \\c | A (sub-)expression  of computer-language-code, Often shown in a non-proportional font.
  Abbreviation   | \\a | An abbreviation, shown in a different style to show there is a definition somewhere.
  Italic         | \\i | Show text in italic, without the text begin "emphasis"
  Bold           | \\b | Show text in bold, without the text being "strong"
  Citation       | \\q | A citation of a title, or a quote.
  Keyboard       | \\k | Used in help messages to show which key to press.
  Mark           | \\h | Used to highlight text, like using a physical yellow marker.
  Math           | \\m | A mathematical (sub-)expression. Often shown in a special italic math font.
  Unarticulated  | \\u | Unarticulated text. Often shown as regular underlined.


Rich Text format
----------------

 ----------------------- | ------------------------
  \\l{language}          | Select ISO-639 language.
  \\f{font-family-name}



