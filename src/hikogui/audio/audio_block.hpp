// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../time/time.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <span>

hi_export_module(hikogui.audio.audio_block);

hi_export namespace hi { inline namespace v1 {

hi_export enum class audio_block_state { normal, silent, corrupt };

/** A block of audio data.
 * This represents a block of audio data received from, or to be send to,
 * an audio device.
 *
 * The samples in this block are always in native floating point format for
 * easy processing. The samples are stored continues for each channel so that
 * processing can be done at a per-channel basis using SSE instructions.
 */
hi_export class audio_block {
public:
    /** A list of pointers to non-interleaved sample buffers.
     * It is undefined behavior to modify the samples on input.
     *
     * Each of the sample buffers is aligned to and a multiple of 4096 bytes in size
     * which will allow you to over-read or over-write with vector instructions
     * beyond the `num_samples` of samples.
     *
     * The sample buffers are NOT pre-cleared during recording.
     */
    float **samples;

    /** Number of samples for each channel in samples.
     */
    std::size_t num_samples;

    /** Number of channels in samples.
     */
    std::size_t num_channels;

    /** The sample rate this block was taken at.
     * This is the word-clock rate, not the sample rate the device was configured as.
     */
    int sample_rate;

    /** The sample count value for the first sample in the sample buffers.
     */
    int64_t sample_count;

    /** Time point when the sample was at the input or will be at the output of the audio interface.
     */
    utc_nanoseconds time_stamp;

    /** The state of the audio block.
     *  - normal: The sample buffers contain normalized -1.0 to 1.0 sample data.
     *  - silent: The sample buffers contain 0.0.
     *  - corrupt: The sample buffers contain NaN.
     *
     * Examples of how corruption could happen:
     *  - CRC error caused by a bad USB/Firewire/Ethernet cable.
     *  - Sample rate of the word clock or digital audio input
     *    and the sample rate of the audio device are too far off.
     *
     * When the state is corrupt; DO NOT READ THE SAMPLE_BUFFER.
     */
    audio_block_state state;
};

}} // namespace hi::inline v1
