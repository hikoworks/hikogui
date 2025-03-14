Audio clock and time
====================
This document describes how clocks and time work in the audio subsystem.

Clocks and sample rate
----------------------
Audio samples are send to the computer at a rate that is determined by
the clock of the audio interface.

Professional audio interfaces may have the ability to synchronize their
clock to an external clock. This allows multiple audio interfaces to
share a clock, inside a clock-domain.

On MacOS these clocks are configurable through the API.

Example sample-clocks:
 - **Internal**: This is the internal oscillator, usually these internal clocks
   are not very accurate and the sample rate could be different by a few Hz
   and the rate can change over time and temperature.
 - **Word Clock**: Professional audio interfaces often have BNC connectors
   for input and output word-clock. A word clock is signal which goes high
   when a sample is taken.
 - **Digital Audio** SPDIF, ADAT, MADI digital audio inputs can also be used
   to synchronize to.
 - **LTC SMPTE/EBU** Some audio interfaces that have a builtin
   LTC SMPTE/EBU decoder it can use this signal as a clock source.
 - **Firewire** Firewire audio interface can use the clock of the Firewire
   interface, multiple audio interfaces connected on the same bus can
   synchronize through this.

