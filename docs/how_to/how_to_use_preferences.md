How to use preferences
======================

Preferences
-----------

Opens the preferences file of the application.
The location of the preferences file is operating system depended. On
windows this file is located in:
`\%USERPROFILE%\AppData\Local\<vendor>\<application name>\preferences.json`

```cpp
auto preferences = hi::preferences(hi::URL::urlFromApplicationPreferencesFile());
```

Observer values are linked to values in the preferences located using a json-path.
In the example below the json-path is "bar.foo" which results in the preferences file as
`{"bar": {"foo": 1}}`:

```cpp
hi::observer<int> foo;
preferences.add("bar.foo", foo);

// Every time the program loads the value in the preference file is incremented.
++foo;
```

Complex types
-------------

The example above will work with observers with types that can be natively
stored in a json file: integers, floating point, booleans, strings, vectors and maps.

For more complex types, you will need to add a template specialization for `hi::pickle`.
This specialization adds an encoder and decoder between the complex type and a `hi::datum`.
The `hi::preferences` knows how to read and write a json file using `hi::datum` values.

The example below shows how to specialize `hi::pickle` for the complex `foo_type` type:

```cpp
struct foo_type {
    int one;
    int two;

    friend bool operator==(foo_type const &, foo_type const &) = default;
};

template<>
struct hi::pickle<foo_type> {
    [[nodiscard]] datum encode(foo_type const& x) const noexcept {
        return datum::make_map("one", x.one, "two", x.two);
    }

    [[nodiscard]] foo_type decode(datum const& x) const {
        return foo_type{ static_cast<int>(x["one"]), static_cast<int>(x["two"]) };
    };
};

hi::observer<foo_type> foo;
preferences.add("foo", foo);

hi_log_info("old: {}", foo->one);
foo->one += 1;
hi_log_info("new: {}", foo->one);
```
