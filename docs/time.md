HikoGUI time handling
====================

High resolution UTC
-------------------

`hires_utc_clock` is a std::chrono clock. This is a clock with 1ns resolution, but its actual precision
is based on the clock of the operating system:

 - POSIX `clock_gettime(CLOCK_REALTIME)`
 - Windows 10 `GetSystemTimeAsFileTime()`
 - macOS/iOS `gettimeofday()`

During leap-seconds the UTC clock may slew or jump depending on how the operating system clock operates.

High performance Continues-UTC
------------------------------

`hiperf_utc_clock` is a std::chrono clock. This is a clock with 1ns resolution, but its actual precision
is based on a CPU counter and disciplined with `hires_utc_clock`.

If `hires_utc_clock` jumps during a leap second then `hiperf_utc_clock` will filter out the leap second
and keep counting as if there has not been a leap second. A `leapsecond_offset` attribute tracks the
detected jumps.

If `hires_utc_clock` slews during the leap second then `hiperf_utc_clock` will slew with it.
