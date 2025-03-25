// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "audio_device.hpp"
#include "audio_stream_format.hpp"
#include "audio_stream_format_win32.hpp"
#include "win32_device_interface.hpp"
#include "win32_wave_device.hpp"
#include "audio_format_range.hpp"
#include "../algorithm/algorithm.hpp"
#include "../macros.hpp"
#include "../win32_headers.hpp"
#include <algorithm>
#include <vector>
#include <coroutine>

hi_export_module(hikogui.audio.audio_device_win32);

hi_export namespace hi { inline namespace v1 {

/*! A class representing an audio device on the system.
 */
hi_export class audio_device_win32 : public audio_device {
public:
    audio_device_win32(IMMDevice *device) :
        audio_device(), _previous_state(audio_device_state::uninitialized), _device(device), _audio_client(nullptr)
    {
        hi_assert(_device != nullptr);
        _end_point_id = get_device_id(_device);
        _id = std::string{"win32:"} + _end_point_id;
        hi_hresult_check(_device->QueryInterface(&_end_point));
        hi_hresult_check(_device->OpenPropertyStore(STGM_READ, &_property_store));

        EDataFlow data_flow;
        hi_hresult_check(_end_point->GetDataFlow(&data_flow));
        switch (data_flow) {
        case eRender:
            _direction = audio_direction::output;
            break;
        case eCapture:
            _direction = audio_direction::input;
            break;
        case eAll:
            _direction = audio_direction::bidirectional;
            break;
        default:
            hi_no_default();
        }

        update_state();
    }

    ~audio_device_win32()
    {
        _property_store->Release();
        _end_point->Release();
        _device->Release();

        if (_audio_client != nullptr) {
            _audio_client->Release();
        }
    }

    /** Get the device id for the given win32 audio end-point.
     *
     * @param device The win32 device instance to get the device id from.
     * @return A device id as string, to differentiate with asio it will start with "win32:"
     * @throws io_error When the device id could not be retrieved.
     */
    [[nodiscard]] static std::string get_device_id(IMMDevice *device)
    {
        hi_assert(device);

        // Get the cross-reboot-unique-id-string of the device.
        LPWSTR device_id;
        hi_hresult_check(device->GetId(&device_id));
        hi_assert(device_id);

        auto device_id_ = hi::to_string(device_id);

        CoTaskMemFree(device_id);
        return device_id_;
    }

    /// @privatesection
    void update_state() noexcept override
    {
        hi_axiom(loop::main().on_thread());

        _name = end_point_name();

        auto new_state = state();

        // Log the correct message.
        if (_previous_state == audio_device_state::uninitialized) {
            hi_log_info(" * Found new audio device '{}' {} ({})", name(), id(), state());

        } else if (_previous_state != new_state) {
            hi_log_info(" * Audio device changed state '{}' {} ({})", name(), id(), state());
        }

        // Start and stop the audio device depending if it was enabled/disabled for some reason.
        if (_previous_state == audio_device_state::active and new_state != audio_device_state::active) {
            _audio_client->Release();
            _audio_client = nullptr;

        } else if (_previous_state != audio_device_state::active and new_state == audio_device_state::active) {
            if (FAILED(_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void **>(&_audio_client)))) {
                hi_log_error("Audio device {} does not have IAudioClient interface", name());
                _audio_client = nullptr;
            }

            update_supported_formats();

            // By setting exclusivity to false at the start the audio stream format is initialized properly.
            set_exclusive(false);
        }
        _previous_state = new_state;
    }

    [[nodiscard]] hi::label label() const noexcept override
    {
        return {elusive_icon::Speaker, txt("{}", name())};
    }

    [[nodiscard]] audio_device_state state() const noexcept override
    {
        DWORD state;
        hi_hresult_check(_device->GetState(&state));

        switch (state) {
        case DEVICE_STATE_ACTIVE:
            return audio_device_state::active;
        case DEVICE_STATE_DISABLED:
            return audio_device_state::disabled;
        case DEVICE_STATE_NOTPRESENT:
            return audio_device_state::not_present;
        case DEVICE_STATE_UNPLUGGED:
            return audio_device_state::unplugged;
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] audio_direction direction() const noexcept override
    {
        return _direction;
    }

    [[nodiscard]] bool exclusive() const noexcept override
    {
        return _exclusive;
    }

    void set_exclusive(bool exclusive) noexcept override
    {
        if (exclusive) {
            _current_stream_format = find_exclusive_stream_format(_sample_rate, _speaker_mapping);
        } else {
            _current_stream_format = shared_stream_format();
        }

        _exclusive = exclusive;
    }

    [[nodiscard]] double sample_rate() const noexcept override
    {
        return _sample_rate;
    }

    void set_sample_rate(double sample_rate) noexcept override
    {
        _sample_rate = sample_rate;
    }

    [[nodiscard]] hi::speaker_mapping input_speaker_mapping() const noexcept override
    {
        switch (direction()) {
        case audio_direction::input:
            [[fallthrough]];
        case audio_direction::bidirectional:
            return _speaker_mapping;
        case audio_direction::output:
            return hi::speaker_mapping::none;
        default:
            hi_no_default();
        }
    }

    void set_input_speaker_mapping(hi::speaker_mapping speaker_mapping) noexcept override
    {
        switch (direction()) {
        case audio_direction::input:
            [[fallthrough]];
        case audio_direction::bidirectional:
            _speaker_mapping = speaker_mapping;
            break;
        case audio_direction::output:
            break;
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] std::vector<hi::speaker_mapping> available_input_speaker_mappings() const noexcept override
    {
        return {};
    }

    [[nodiscard]] hi::speaker_mapping output_speaker_mapping() const noexcept override
    {
        switch (direction()) {
        case audio_direction::output:
            [[fallthrough]];
        case audio_direction::bidirectional:
            return _speaker_mapping;
        case audio_direction::input:
            return hi::speaker_mapping::none;
        default:
            hi_no_default();
        }
    }

    void set_output_speaker_mapping(hi::speaker_mapping speaker_mapping) noexcept override
    {
        switch (direction()) {
        case audio_direction::output:
            [[fallthrough]];
        case audio_direction::bidirectional:
            _speaker_mapping = speaker_mapping;
            break;
        case audio_direction::input:
            break;
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] std::vector<hi::speaker_mapping> available_output_speaker_mappings() const noexcept override
    {
        return {};
    }

    [[nodiscard]] bool supports_format(audio_stream_format const& format) const noexcept
    {
        if (not win32_use_extensible(format)) {
            // First try the simple format.
            auto format_ = audio_stream_format_to_win32(format, false);
            switch (_audio_client->IsFormatSupported(
                AUDCLNT_SHAREMODE_EXCLUSIVE, reinterpret_cast<WAVEFORMATEX const *>(&format_), NULL)) {
            case S_OK:
                return true;
            case S_FALSE:
            case AUDCLNT_E_UNSUPPORTED_FORMAT:
                break;
            default:
                hi_log_error("Failed to check format. {}", get_last_error_message());
            }
        }

        // Always check the extensible format as fallback.
        {
            auto format_ = audio_stream_format_to_win32(format, true);
            switch (_audio_client->IsFormatSupported(
                AUDCLNT_SHAREMODE_EXCLUSIVE, reinterpret_cast<WAVEFORMATEX const *>(&format_), NULL)) {
            case S_OK:
                return true;
            case S_FALSE:
            case AUDCLNT_E_UNSUPPORTED_FORMAT:
                break;
            default:
                hi_log_error("Failed to check format. {}", get_last_error_message());
            }
        }

        return false;
    }
    /// @endprivatesection
private:
    std::string _end_point_id;
    audio_device_state _previous_state;
    audio_direction _direction;
    bool _exclusive = false;
    double _sample_rate = 0.0;
    hi::speaker_mapping _speaker_mapping = hi::speaker_mapping::none;
    audio_stream_format _current_stream_format;

    IMMDevice *_device = nullptr;
    IMMEndpoint *_end_point = nullptr;
    IPropertyStore *_property_store = nullptr;
    IAudioClient *_audio_client = nullptr;

    template<typename T>
    [[nodiscard]] static T get_property(IPropertyStore *property_store, REFPROPERTYKEY key)
    {
        hi_not_implemented();
    }

    template<>
    [[nodiscard]] std::string get_property(IPropertyStore *property_store, REFPROPERTYKEY key)
    {
        hi_assert(property_store != nullptr);

        PROPVARIANT property_value;
        PropVariantInit(&property_value);

        hi_hresult_check(property_store->GetValue(key, &property_value));
        if (property_value.vt == VT_LPWSTR) {
            auto value_wstring = std::wstring_view(property_value.pwszVal);
            auto value_string = to_string(value_wstring);
            PropVariantClear(&property_value);
            return value_string;

        } else {
            PropVariantClear(&property_value);
            throw io_error(std::format("Unexpected property value type {}.", static_cast<int>(property_value.vt)));
        }
    }

    template<>
    [[nodiscard]] uint32_t get_property(IPropertyStore *property_store, REFPROPERTYKEY key)
    {
        hi_assert(property_store != nullptr);

        PROPVARIANT property_value;
        PropVariantInit(&property_value);

        hi_hresult_check(property_store->GetValue(key, &property_value));
        if (property_value.vt == VT_UI4) {
            auto value = narrow_cast<uint32_t>(property_value.ulVal);
            PropVariantClear(&property_value);
            return value;

        } else {
            PropVariantClear(&property_value);
            throw io_error(std::format("Unexpected property value type {}.", static_cast<int>(property_value.vt)));
        }
    }

    template<>
    [[nodiscard]] GUID get_property(IPropertyStore *property_store, REFPROPERTYKEY key)
    {
        hi_assert(property_store != nullptr);

        PROPVARIANT property_value;
        PropVariantInit(&property_value);

        hi_hresult_check(property_store->GetValue(key, &property_value));
        if (property_value.vt == VT_CLSID) {
            auto value = static_cast<GUID>(*property_value.puuid);
            PropVariantClear(&property_value);
            return value;

        } else {
            PropVariantClear(&property_value);
            throw io_error(std::format("Unexpected property value type {}.", static_cast<int>(property_value.vt)));
        }
    }

    [[nodiscard]] std::string end_point_name() const noexcept
    {
        try {
            return get_property<std::string>(_property_store, PKEY_Device_FriendlyName);
        } catch (io_error const&) {
            return "<unknown name>";
        }
    }

    /** Get a user friendly name of the audio device.
     * This is the name of the audio device itself, such as
     * "Realtek High Definition Audio".
     */
    [[nodiscard]] std::string device_name() const noexcept
    {
        try {
            return get_property<std::string>(_property_store, PKEY_DeviceInterface_FriendlyName);
        } catch (io_error const&) {
            return "<unknown device name>";
        }
    }

    /** Get a user friendly description of the audio end-point device.
     * This is the name of the end point, such as "Microphone".
     */
    [[nodiscard]] std::string end_point_description() const noexcept
    {
        try {
            return get_property<std::string>(_property_store, PKEY_Device_DeviceDesc);
        } catch (io_error const&) {
            return "<unknown end point name>";
        }
    }

    GUID pin_category() const noexcept
    {
        try {
            return get_property<GUID>(_property_store, PKEY_AudioEndpoint_Association);
        } catch (io_error const& e) {
            hi_log_error("Could not get pin-category for audio end-point {}: {}", name(), e.what());
            return {};
        }
    }

    /** Get the legacy waveIn or waveOut id.
     *
     * The waveIn/waveOut id are required to get access to the audio device driver
     * to query the formats that it supports.
     */
    //unsigned int wave_id() const;

    /*[[nodiscard]] std::optional<audio_stream_format>
    best_format(double sample_rate, hi::speaker_mapping speaker_mapping) const noexcept
    {
        constexpr auto sample_formats = std::array{
            audio_sample_format::float32(),
            audio_sample_format::fix8_23(),
            audio_sample_format::int24(),
            audio_sample_format::int20(),
            audio_sample_format::int16()};

        for (auto const &sample_format : sample_formats) {
            auto const format = audio_stream_format{sample_format, sample_rate, speaker_mapping};
            if (supports_format(format)) {
                return format;
            }
        }
        return {};
    }

    [[nodiscard]] std::optional<audio_stream_format> best_format(double sample_rate, int num_channels) const
    noexcept
    {
        constexpr auto sample_formats = std::array{
            audio_sample_format::float32(),
            audio_sample_format::fix8_23(),
            audio_sample_format::int24(),
            audio_sample_format::int20(),
            audio_sample_format::int16()};

        for (auto const &sample_format : sample_formats) {
            auto const format = audio_stream_format{sample_format, sample_rate, num_channels};
            if (supports_format(format)) {
                return format;
            }
        }
        return {};
    }

    [[nodiscard]] std::vector<audio_stream_format> enumerate_formats(double sample_rate) const noexcept
    {
        constexpr max_num_channels = 128;

        auto r = std::vector<audio_stream_format>{};
        r.reserve(size(speaker_mappings) + (max_num_channels / 2) + 2);

        for (auto const &info: speaker_mappings) {
            if (auto f = best_format(sample_rate, info.mapping)) {
                r.push_back(*std::move(f));
            }
        }
        for (auto num_channels = 1; num_channels != 4; ++num_channels) {
            if (auto f = best_format(sample_rate, num_channels)) {
                r.push_back(*std::move(f));
            }
        }
        for (auto num_channels = 4; num_channels <= max_num_channels; num_channels += 2) {
            if (auto f = best_format(sample_rate, num_channels)) {
                r.push_back(*std::move(f));
            }
        }

        return r;
    }*/

    /** Query the audio device through the driver to determine the supported formats.
     */
    void update_supported_formats() noexcept
    {
        // https://stackoverflow.com/questions/50396224/how-to-get-audio-formats-supported-by-physical-device-winapi-windows
        // https://github.com/EddieRingle/portaudio/blob/master/src/os/win/pa_win_wdmks_utils.c
        // https://docs.microsoft.com/en-us/previous-versions/ff561658(v=vs.85)

        for (auto const& format_range : get_format_ranges()) {
            hi_log_info("      * {}", format_range);
        }
    }

    /** Find a stream format based on the prototype_stream_format.
     *
     * This function looks for a supported stream format when the device is used in exclusive-mode.
     * The prototype consists of a sample-rate and speaker mapping. From this the best sample format
     * is selected: int24 -> float32 -> int20 -> int16. int24 is selected above float, so that hikogui
     * will handle dithering.
     *
     * @param sample_rate The sample rate selected by the user.
     * @param speaker_mapping The speaker mapping selected by the user.
     * @return A supported audio_stream_format or empty.
     */
    [[nodiscard]] audio_stream_format
    find_exclusive_stream_format(double sample_rate, hi::speaker_mapping speaker_mapping) noexcept
    {
        return {};
    }

    /** Get the shared stream format for the device.
     */
    [[nodiscard]] audio_stream_format shared_stream_format() const
    {
        if (not _audio_client) {
            return {};
        }

        WAVEFORMATEX *ex;
        hi_hresult_check(_audio_client->GetMixFormat(&ex));
        hi_assert_not_null(ex);
        auto const r = audio_stream_format_from_win32(*ex);
        CoTaskMemFree(ex);
        return r;
    }

    [[nodiscard]] generator<audio_format_range> get_format_ranges() const noexcept
    {
        // try {
        auto wave_device = win32_wave_device::find_matching_end_point(direction(), _end_point_id);
        auto device_interface = wave_device.open_device_interface();
        auto format_ranges = std::ranges::to<std::vector>(device_interface.get_format_ranges(direction()));
        auto tmp = std::vector<audio_format_range>{};

        auto first = format_ranges.begin();
        auto last = format_ranges.end();

        // Split the ranged sample rates into individual sample rates.
        auto it = first;
        while (it != last) {
            auto const format = audio_stream_format{it->format, it->min_sample_rate, it->num_channels};

            // Eliminate bit-depths that are not supported.
            if (not supports_format(format)) {
                last = unordered_remove(first, last, it);
                continue;
            }

            // Check the speaker mapping capability at this bit-depth and sample rate.
            it->surround_mode_mask = surround_mode::none;
            for (auto const mode : enumerate_surround_modes()) {
                auto surround_format = format;
                surround_format.speaker_mapping = to_speaker_mapping(mode);
                surround_format.num_channels = narrow_cast<uint16_t>(popcount(surround_format.speaker_mapping));

                if (surround_format.num_channels <= it->num_channels and supports_format(surround_format)) {
                    it->surround_mode_mask |= mode;
                }
            }

            auto odd_rate_format = format;
            ++odd_rate_format.sample_rate;
            if (not supports_format(odd_rate_format)) {
                // The device was lying that it could handle the full range of sample rates.
                // Look for specific sample rates and create separate format ranges.
                for (auto sample_rate : common_sample_rates) {
                    auto common_rate_format = format;
                    common_rate_format.sample_rate = sample_rate;

                    if (it->min_sample_rate <= sample_rate and sample_rate <= it->max_sample_rate and
                        supports_format(common_rate_format)) {
                        tmp.emplace_back(it->format, it->num_channels, sample_rate, sample_rate, it->surround_mode_mask);
                    }
                }

                last = unordered_remove(first, last, it);
                continue;
            }

            ++it;
        }
        format_ranges.erase(last, format_ranges.end());
        format_ranges.insert(format_ranges.cend(), tmp.cbegin(), tmp.cend());

        std::sort(format_ranges.begin(), format_ranges.end(), std::greater{});
        last = std::unique(format_ranges.begin(), format_ranges.end(), [](auto const& lhs, auto const& rhs) {
            return equal_except_bit_depth(lhs, rhs);
        });
        format_ranges.erase(last, format_ranges.end());

        for (auto& format_range : format_ranges) {
            co_yield format_range;
        }

        //} catch (std::exception const& e) {
        //    hi_log_error("get_format_ranges() on audio device {}: {}", name(), e.what());
        //}
    }
};

}} // namespace hi::inline v1
