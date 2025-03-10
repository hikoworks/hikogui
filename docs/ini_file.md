


```
test_string = "bar"
test_integer = 1234
test_float = 1234.23
test_list = [ 123, 456, 566]

[test_group]
; This is a comment by the user.
;; This is a help description automatically generated.
;; test_group_integer = 1
test_group_integer = 1234
test_group_float = $test_float
test_group_dict = {
    "hello": 123,
    "world": 456
}


```


```

document := root group*

root := block

group := '[' group_name ']' block
group_name := ( name '.' )* name

block := ( assignment | comment | description | deprecated | suppress )*

comment := [;#] nolf_text '\n'

description := description_text* description_default
description_text = ';;' nolf_text '\n'
description_default = ';;' assignment

deprecated := deprecated* deprecated_default
deprecated_text = ';!' nolf_text '\n'
deprecated_default = ';!' assignment

suppress := ';-' name* '\n'

assignment := name [=:] expression ( [;#] nolf_text )? '\n'

nolf_text := [^\n]*

name := id

id := [a-zA-Z_][a-zA-Z0-9_]

expression :=

```

