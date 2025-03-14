Binary Object Notation (BON8)
=============================

Here are the reasons for creating the BON8 data format:

 - Exact translation between JSON and BON8
 - A canonical representation to allow signing of messages.
 - No extensibility, allows every parser to handle every BON8.
 - Low amount of overhead.
 - Quick encode / decode.
 - Self terminating, a valid BON8 message can be decoded without a buffer-length.

Encoding
--------

The idea of this encoding is that strings are encoded as UTF-8.

UTF-8 has a lot of codes that fall within the invalid range which
can be used to encode different kinds of data.

All of these invalid UTF-8 start-code-units have been assigned below
to reduce the ability to extent the format, and to improve compression
of the data.

```
message := value;

value := number | boolean | null | string | array | object;

number := integer | float;

integer := positive_integer | negative_integer | signed_integer;

float := binary32 | binary64;

boolean := true | false;

string := character character* | character* eos;

character := utf8_1 | utf8_2 | utf8_3 | utf8_4;

array := start_array_with_count value*
       | start_array value* eoc;

object := start_object_with_count (string value)*
        | start_object (string value)* eoc
```

This table gives an overview on the encoding:

  Code Sequence           | # | Type                        | Bits |      Min |       Max
 :----------------------- | -:|:-------------------------   | ----:| --------:| ----------:
  00-7f                   | 1 | UTF-8 One byte sequence     |    7 |   U+0000 |    U+007f
  c2-df 80-bf             | 2 | UTF-8 Two byte sequence     |   11 |   U+0080 |    U+07ff
  e0-ef 80-bf byte\*1     | 3 | UTF-8 Three byte sequence   |   16 |   U+0800 |    U+ffff
  f0-f7 80-bf byte\*2     | 4 | UTF-8 Four byte sequence    |   21 | U+100000 |  U+10ffff
  80-84                   |   | Start array with count      |      |        0 |         4
  85                      |   | Start array                 |      |          |
  86-8a                   |   | Start object with count     |      |        0 |         4
  8b                      |   | Start object                |      |          |
  8c byte\*4              | 5 | Signed integer              |   32 |    -2^31 |  2^31 - 1
  8d byte\*8              | 9 | Signed integer              |   64 |    -2^63 |  2^63 - 1
  8e byte\*4              | 5 | Floating point binary32     |   32 |          |
  8f byte\*8              | 9 | Floating point binary64     |   64 |          |
  90-b7                   | 1 | Positive integer            |      |        0 |        39
  b8-c1                   | 1 | Negative Integer            |      |       -1 |       -10
  c2-df 00-7f             | 2 | Positive Integer (30 * 128) |      |       40 |      3879
  e0-ef 00-7f byte\*1     | 3 | Positive Integer            |   19 |     3880 |    528167
  f0-f7 00-7f byte\*2     | 4 | Positive Integer            |   26 |   528168 |  67637031
  c2-df c0-ff             | 2 | Negative Integer (30 * 64)  |      |      -11 |     -1930
  e0-ef c0-ff byte\*1     | 3 | Negative Integer            |   18 |    -1931 |   -264074
  f0-f7 c0-ff byte\*2     | 4 | Negative Integer            |   25 |  -264075 | -33818506
  f8                      | 1 | False                       |      |          |
  f9                      | 1 | True                        |      |          |
  fa                      | 1 | null                        |      |          |
  fb                      | 1 | -1.0                        |      |          |
  fc                      | 1 | +0.0                        |      |          |
  fd                      | 1 | +1.0                        |      |          |
  fe                      | 1 | End Of Container (eoc)      |      |          |
  ff                      | 1 | End Of String (eos)         |      |          |

Extra rules
-----------

The rules below ensures minimum message size and consistency, which allows for cryptographically
signing of messages. All encoders MUST follow these rules.

 - A message is a single value. Most often this value is of type Object or type Array.
 - A string does not need to be terminated with an eos (0xff), if the ending of a string
   can be determined in other ways.
 - String MUST end with eos (0xff) when:
   - The string is empty,
   - When the next byte in the message starts another string,
   - If there are no more bytes left in the message.
 - Strings MUST be a valid UTF-8 encoded string.
 - Strings MAY contain any Unicode code-point between U+0000 and U+10ffff.
 - Integers and floating point numbers are stored most significant bits first (big endian).

Canonical Representation
------------------------
The rules below ensure a minimum message and a consistent encoding between different
implementation. The canonical representation is consistent enough to be used for cryptographically
signing of data.

 - The unicode string MUST be in Normalization Form C (NFC).
 - Floating point negative zero, infinite and NaN MUST be encoded as binary32.
 - NaN should be encoded as a 0x7f800001.
 - Floating point numbers that can be converted to binary32 without loss
   of precision or range MUST be encoded as binary32.
 - Strings, Integers, Arrays and Objects MUST be encoded with the least amount of bytes.
 - The keys of an Object MUST be lexically ordered based on UTF-8 code-units.

Examples
--------

### Single string

"ab"

 - A string at the end of a message needs to be terminated.


```
'a'  'b'  eos
0x61 0x62 0xff
```

### Array with two strings

["ab", "bc"]

 - The first string needs to be terminated to differentiate it with the second.
 - The array is not terminated because it has an count.
 - The second string is at the end of the message so it must be terminated.

```
[2   'a'  'b'  eos  'b'  'c'  eos
0x82 0x61 0x62 0xff 0x62 0x63 0xff
```

### Array with five strings

["a", "b", "c", "d", "e"]

 - All but the last string needs to be terminated to differentiate it with the next string.
 - The array is terminated, because it does not have an count.
 - The last string is not terminated because the array terminator is a natural ending.

```
[    'a'  eos  'b'  eos  'c'  eos  'd'  eos  'e'  ]
0x85 0x61 0xff 0x62 0xff 0x63 0xff 0x64 0xff 0x65 0xfe
```

### A object with two integer values

{"ab": 1, "bc", 2}

 - The strings are not terminated because an integer is a natural ending of a string.
 - The object is not terminated because it has a count.
 - The message ends because the object ends.

```
{2   'a'  'b'  1    'b'  'c'  2
0x88 0x61 0x62 0x91 0x62 0x63 0x92
```

### Nested array with strings

{"a": ["b", "c"], "d": 1}

 - The strings "b" and "c" need to be terminated because the following byte is the start of a string.
 - Both object and array do not have terminators because it has a count
 - The strings "a" and "d" are followed by a non string so do not need to be terminated.

```
{2   'a'  [2   'b'  'c'  'd'  1
0x88 0x61 0x82 0x62 0x63 0x64 0x91
```

### Object with empty string key

{"": 1, "a": 2}

 - The first string is empty so it will be terminated.
 - The second string is followed directly by an integer so is naturally terminated.

```
{2   eos  1    'a'  2
0x88 0xff 0x91 0x61 0x92
```



