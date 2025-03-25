
#pragma once

#include "bookmark.hpp" // export
#include "cmake_install.hpp" // export
#include "glob.hpp" // export
#include "path_location.hpp" // export
#include "URI.hpp" // export
#include "URL.hpp" // export

hi_export_module(hikogui.path);

hi_export namespace hi {
inline namespace v1 {

/**
\defgroup path Path handling utilities.

This module contains file handling utilities:
 - `*_dir()`, `*_dirs()`, `*_file()` for getting the configured paths of the
   system.
 - `find_path()` to find files in a set of directories.
 - `glob_pattern`, `glob()` to find files and directories based on a glob pattern.
 - `URI` and `URL` for managing URLs for local and remote resources.

Path locations
--------------
The path-locations functions are used to find files and directories based
on the context of the operating system, the user account and application.

For example fonts are located in multiple places:
 - the font-directory where a user can install their own fonts,
 - the font-directory that is part of the application's resources, and
 - the font-directory which is shared between all users of the operating system.

To iterate over all the font directories:

```
for (auto const &path : font_dirs()) {
    std::cout << path.string() << std::endl;
}
```

To find the first matching file in one of the font directories:

```
if (auto const &path : hi::find_path(font_dirs(), "arial.ttf")) {
    std::cout << path->string() << std::endl;
}
```

glob
----
To find files based on a pattern using wild cards like `*` you can use the glob
utilities.

The constructor of the `glob_pattern` object parses a string or `std::filesystem::path`
which contain one or more of the following tokens:

  Token          | Description
 :-------------- |:--------------------------------
  foo            | Matches the text "foo".
  ?              | Matches any single character except '/'.
  [abcd]         | Matches a single character that is 'a', 'b', 'c' or 'd'.
  [a-d]          | Matches a single character that is 'a', 'b', 'c' or 'd'.
  [-a-d]         | Matches a single character that is '-', 'a', 'b', 'c' or 'd'.
  {foo,bar,baz}  | Matches the text "foo", "bar" or "baz".
  \*             | Matches zero or more character except '/'.
  /&zwj;**&zwj;/ | Matches one or more directories. A single slash or zero or more characters between two slashes.

Then the `glob()` function will search for files and directories matching this pattern.
`glob()` is also overloaded to directly parse a pattern and combine it with `path_location`.

For example to find all the files in the font directories:

```
for (auto const &path : hi::glob(font_dirs(), "**&zwj;/&zwj;
    *.ttf ")) {
    std::cout
    << path.string() << std::endl;
}
```

URL / URI
---------
The URI, URL and URN terms can be confusing, here is a short explanation.
 - A URI (Unique Resource Identifier) is a specification for parsing and writing identifiers.
 - A URL (Unique Resource Locator) is a type of URI which is used to address things.
 - A URN (Unique Resource Name) is a type of URI which is used to name things.

HikoGUI currently implements dereferencing of the following types of URLs:
 - **relative path**: A relative path does not start with a scheme and can be
   joined with a base URL to get an absolute URL.
 - **file-URL**: A file accessible via the operating system. This scheme
   supports the following features:
   + relative paths (can not be joined using URL joining),
   + absolute path (relative to the current drive or file-share),
   + relative path on a specific drive,
   + absolute path on a specific drive and file-share,
   + conversion of drive to a file-share name,
   + the server name may be part of the double-slash "//" path or the authority.
 - **resource-URL**: A relative path that converts into a file-path by searching
   for the first file in one of the directories of `resource_dirs()`.

Both file: and resource: URLs may be implicitly converted to a `std::filesystem::path`.

*/

}}