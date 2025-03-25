
#pragma once

#include "../color/color.hpp"
#include "../geometry/geometry.hpp"
#include "../file/file.hpp"
#include "../macros.hpp"
#include "sfloat_rgba16.hpp"
#include "pixmap.hpp"
#include "png_types.hpp"
#include <gsl/gsl>
#include <png.h>
#include <span>
#include <string>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <csetjmp>
#include <filesystem>
#include <functional>
#include <system_error>

hi_warning_push();
// warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable.
// This warning is caused by using setjmp/longjmp in C++ code, even when it is
// used correctly.
hi_warning_ignore_msvc(4611);
// warning C4702: unreachable code.
// This warning is caused by using std::unreachable() in a place where the
// compiler determines that it is unreachable. I am using std::unreachable()
// here because png_error() is not marked as [[noreturn]]. Therfore a different
// compiler may think that the code is reachable; and fall of the end of the
// function.
hi_warning_ignore_msvc(4702); 

hi_export_module(hikogui.image : png_loader);

hi_export namespace hi::inline v1 {

class png_loader;

class png_loader_delegate {
public:
    /** Destructor.
     */
    virtual ~png_loader_delegate() = default;

    /** Get the destination row index to start filling in pixels.
     * 
     * @param src The source row of pixels. Used to determine interlace type.
     * @param src_nr The row number of the source row.
     * @param pass The current interlace pass.
     * @param width The width of the image.
     * @param height The height of the image.
     * @return The index of the destination-row to fetch next. Or height to stop.
     */
    [[nodiscard]] static size_t
    fill_start(std::span<sfloat_rgba16> src, size_t src_nr, unsigned int pass, size_t width, size_t height) noexcept
    {
        // clang-format off
        if (pass == 0 and src.size() == width) {
            return src_nr;
        } else switch (pass) {
            case 0: return src_nr * 8 + 0;
            case 1: return src_nr * 8 + 0;
            case 2: return src_nr * 8 + 4;
            case 3: return src_nr * 4 + 0;
            case 4: return src_nr * 4 + 2;
            case 5: return src_nr * 2 + 0;
            case 6: return src_nr * 2 + 1;
            default: return height;
        }
        // clang-format on
    }

    /** Fill the destination row with pixels.
     *
     * @param src The source row of pixels.
     * @param src_nr The row number of the source row.
     * @param pass The current interlace pass.
     * @param step The current step in the interlace pass.
     * @param width The width of the image.
     * @param height The height of the image.
     * @param dst The destination row of pixels.
     * @return Index of the destination-row to fetch next. Or height to stop.
     */
    [[nodiscard]] static size_t fill(
        std::span<sfloat_rgba16> src,
        size_t src_nr,
        unsigned int pass,
        unsigned int step,
        size_t width,
        size_t height,
        std::span<sfloat_rgba16> dst) noexcept
    {
        auto const next_step = step + 1;

        if (pass == 0 and src.size() == width) {
            std::copy(src.begin(), src.end(), dst.begin());
            return height;

        } else {
            switch (pass) {
            case 0: // x=%8+0, y=%8+0
                for (size_t x = 0; x != width; ++x) {
                    dst[x] = src[x >> 3];
                }
                return next_step < 8 ? src_nr * 8 + 0 + next_step : height;

            case 1: // x=%8+4, y=%8+0
                for (size_t x = 0; x != width; ++x) {
                    if (x % 8 >= 4) {
                        dst[x] = src[(x - 4) >> 3];
                    }
                }
                return next_step < 8 ? src_nr * 8 + 0 + next_step : height;

            case 2: // x=%4+0, y=%8+4
                for (size_t x = 0; x != width; ++x) {
                    dst[x] = src[x >> 2];
                }
                return next_step < 4 ? src_nr * 8 + 4 + next_step : height;

            case 3: // x=%4+2, y=%4+0
                for (size_t x = 0; x != width; ++x) {
                    if (x % 4 >= 2) {
                        dst[x] = src[(x - 2) >> 2];
                    }
                }
                return next_step < 4 ? src_nr * 4 + 0 + next_step : height;

            case 4: // x=%2+0, y=%4+2
                for (size_t x = 0; x != width; ++x) {
                    dst[x] = src[x >> 1];
                }
                return next_step < 2 ? src_nr * 4 + 2 + next_step : height;

            case 5: // x=%2+1, y=%2+0
                for (size_t x = 0; x != width; ++x) {
                    if (x % 2 >= 1) {
                        dst[x] = src[(x - 1) >> 1];
                    }
                }
                return next_step < 2 ? src_nr * 2 + 0 + next_step : height;

            case 6: // x=%1+0, y=%2+1
                std::copy(src.begin(), src.end(), dst.begin());
                return height;

            default:
                return height;
            }
        }
    }

    /** Called when the image information is available.
     *
     * @param sender The png-loader that sent the information.
     */
    virtual void info(png_loader* sender) = 0;

    /** Called when a row of pixels is available.
     *
     * @param sender The png-loader that sent the row.
     * @param src The row of pixels.
     * @param src_nr The row number of the row.
     * @param pass The current interlace pass.
     */
    virtual void row(png_loader* sender, std::span<sfloat_rgba16> src, size_t src_nr, unsigned int pass) = 0;

    /** Called when the image is fully loaded.
     *
     * @param sender The png-loader that sent the end.
     */
    virtual void end(png_loader* sender) {}
};

class png_loader {
public:
    ~png_loader()
    {
        _cleanup();
    }

    /** Construct a png-loader with a delegate.
     * 
     * @param delegate The delegate to use to build up the image.
     */
    png_loader(png_loader_delegate& delegate) : _delegate(std::addressof(delegate))
    {
        _png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, this, _user_error_fn, nullptr);
        if (_png_ptr == nullptr) {
            _cleanup();
            throw std::runtime_error("png_create_read_struct");
        }
    }

    /** Set the maximum size of the image.
     *
     * This is useful to prevent loading of images that are too large.
     *
     * @param width The maximum width of the image.
     * @param height The maximum height of the image.
     */
    void set_limit(size_t width, size_t height)
    {
        if (std::setjmp(png_jmpbuf(_png_ptr))) {
            throw std::runtime_error(std::move(_last_error_message));
        }

        png_set_user_limits(_png_ptr, gsl::narrow<png_uint_32>(width), gsl::narrow<png_uint_32>(height));
    }

    /** Specify how many bytes you have already read from the file.
     *
     * This is useful if you read png-data from a file using a method that
     * does not include an ability to seek.
     *
     * @param nr_bytes The number of bytes at the start of the png-data
     *                 which will not be included in calls to `add_data()`.
     */
    void skip_signature(size_t nr_bytes)
    {
        if (std::setjmp(png_jmpbuf(_png_ptr))) {
            throw std::runtime_error(std::move(_last_error_message));
        }

        png_set_sig_bytes(_png_ptr, gsl::narrow<int>(nr_bytes));
    }

    /** Add more data to the png-loader.
     *
     * This will process the data and call the delegate when information
     * and rows are available.
     * 
     * @param data The data to add to the png-loader.
     */
    void add_data(std::span<std::byte const> data)
    {
        if (std::setjmp(png_jmpbuf(_png_ptr))) {
            throw std::runtime_error(std::move(_last_error_message));
        }

        if (_info_ptr == nullptr) {
            // Delayed initialization of info_ptr and progressive read.
            // So that we don't need to call std::setjmp() in the constructor.
            png_set_progressive_read_fn(_png_ptr, this, _info_callback, _row_callback, _end_callback);

            _info_ptr = png_create_info_struct(_png_ptr);
            if (_info_ptr == nullptr) {
                png_error(_png_ptr, "PNG: Could not create info_ptr struct.");
            }
        }

        auto const bytes = reinterpret_cast<png_bytep>(const_cast<std::byte*>(data.data()));
        png_process_data(_png_ptr, _info_ptr, bytes, data.size());
    }

    /** The width of the image.
     * 
     * @return The width of the image.
     */
    [[nodiscard]] size_t width() const noexcept
    {
        return _width;
    }

    /** The height of the image.
     * 
     * @return The height of the image.
     */
    [[nodiscard]] size_t height() const noexcept
    {
        return _height;
    }

    /** Interlace method of image.
     *
     * Interlace methods:
     *  - none: 1 pass, all data in one go.
     *  - adam7: 7 passes.
     */
    [[nodiscard]] png_interlace_type interlace_type() const noexcept
    {
        return _interlace_type;
    }

private:
    png_loader_delegate* _delegate = nullptr;
    png_structp _png_ptr = nullptr;
    png_infop _info_ptr = nullptr;
    std::string _last_error_message = {};
    size_t _width = 0;
    size_t _height = 0;
    int _bit_depth = 0;
    png_color_type _color_type = png_color_type::gray;
    png_interlace_type _interlace_type = png_interlace_type::none;
    std::optional<hi::color_primaries> _color_primaries = {};
    std::optional<matrix3> _color_matrix = {};
    png_iccp_profile _iccp_profile = png_iccp_profile::unknown;
    float _component_scale = 1.0f / 65535.0f;
    std::vector<sfloat_rgba16> _row;

    [[nodiscard]] static png_interlace_type _png_get_interlace_type(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        switch (png_get_interlace_type(png_ptr, info_ptr)) {
        case PNG_INTERLACE_NONE:
            return png_interlace_type::none;
        case PNG_INTERLACE_ADAM7:
            return png_interlace_type::adam7;
        default:
            png_error(png_ptr, "PNG: unknown interlace method");
        }
        std::unreachable();
    }

    [[nodiscard]] static png_color_type _png_get_color_type(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        switch (png_get_color_type(png_ptr, info_ptr)) {
        case PNG_COLOR_TYPE_GRAY:
            return png_color_type::gray;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            return png_color_type::gray_alpha;
        case PNG_COLOR_TYPE_PALETTE:
            return png_color_type::palette;
        case PNG_COLOR_TYPE_RGB:
            return png_color_type::rgb;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            return png_color_type::rgb_alpha;
        default:
            png_error(png_ptr, "PNG: unknown color type");
        }
    }

    [[nodiscard]] static std::optional<hi::color_primaries> _png_get_cHRM(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        double wx = 0.0;
        double wy = 0.0;
        double rx = 0.0;
        double ry = 0.0;
        double gx = 0.0;
        double gy = 0.0;
        double bx = 0.0;
        double by = 0.0;

        if (png_get_cHRM(png_ptr, info_ptr, &wx, &wy, &rx, &ry, &gx, &gy, &bx, &by)) {
            return hi::color_primaries{
                gsl::narrow<float>(wx),
                gsl::narrow<float>(wy),
                gsl::narrow<float>(rx),
                gsl::narrow<float>(ry),
                gsl::narrow<float>(gx),
                gsl::narrow<float>(gy),
                gsl::narrow<float>(bx),
                gsl::narrow<float>(by)};
        }

        return std::nullopt;
    }

    [[nodiscard]] static png_iccp_profile _png_get_iCCP(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        png_charp name = nullptr;
        int compression_type = 0;
        png_bytep profile = nullptr;
        png_uint_32 profile_size = 0;

        if (png_get_iCCP(png_ptr, info_ptr, &name, &compression_type, &profile, &profile_size)) {
            auto const _name = std::string_view{name};

            if (_name == "ITUR_2100_PQ_FULL") {
                return png_iccp_profile::ITUR_2100_PQ_FULL;
            }
        }

        return png_iccp_profile::unknown;
    }

    void info_callback(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        _width = png_get_image_width(png_ptr, info_ptr);
        _height = png_get_image_height(png_ptr, info_ptr);
        _bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        _interlace_type = _png_get_interlace_type(png_ptr, info_ptr);
        _color_type = _png_get_color_type(png_ptr, info_ptr);
        _color_primaries = _png_get_cHRM(png_ptr, info_ptr);
        _iccp_profile = _png_get_iCCP(png_ptr, info_ptr);

        // First convert palette and gray images to RGB (with optional alpha)
        if (_color_type == png_color_type::palette) {
            png_set_palette_to_rgb(png_ptr);

        } else if (_color_type == png_color_type::gray or _color_type == png_color_type::gray_alpha) {
            if (_bit_depth < 8) {
                png_set_expand_gray_1_2_4_to_8(png_ptr);
            }
            png_set_gray_to_rgb(png_ptr);
        }

        // Expand the image to 16-bit per channel.
        // This is needed to have enough precision for conversion to linear space.
        if (_bit_depth < 16) {
            png_set_expand_16(png_ptr);
        }

        // If the image is missing an alpha channel, then add one.
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr);
        } else if (_color_type == png_color_type::rgb) {
            png_set_add_alpha(png_ptr, 0xffff, PNG_FILLER_AFTER);
        }

        constexpr auto use_premultiplied_alpha = true;
        if constexpr (use_premultiplied_alpha) {
            // PNG_ALPHA_STANDARD: The RGB values are premultiplied with the
            //                     alpha value. The RGB values are always
            //                     linearly encoded, even if the specified gamma
            //                     is not PNG_GAMMA_LINEAR. Instead the gamma
            //                     value specified is the default gamma value
            //                     for the file when the file does not include
            //                     a gAMA chunk.
            //
            // PNG_DEFAULT_sRGB: The default sRGB-like gama value to use when
            //                   the file does not have a gAMA chunk.
            png_set_alpha_mode(png_ptr, PNG_ALPHA_STANDARD, PNG_DEFAULT_sRGB);

        } else {
            // PNG_ALPHA_PNG: The RGB values are not premultiplied with the alpha value.
            //                But the RGB values are gamma encoded.
            //
            // PNG_DEFAULT_sRGB: First set the default "gamma" value to sRGB in case
            //                   the file does not have a gAMA chunk.
            png_set_alpha_mode(png_ptr, PNG_ALPHA_PNG, PNG_DEFAULT_sRGB);
            // PNG_GAMMA_LINEAR: Convert RGB values to linear space.
            png_set_alpha_mode(png_ptr, PNG_ALPHA_PNG, PNG_GAMMA_LINEAR);
        }

        // Calculate the optional color conversion matrix.
        if (_color_primaries) {
            auto const image_to_XYZ = color_primaries_to_RGBtoXYZ(*_color_primaries);
            _color_matrix = XYZ_to_sRGB * image_to_XYZ;
        } else {
            _color_matrix = std::nullopt;
        }

        // Calculate how to convert 16-bit color components to float.
        switch (_iccp_profile) {
        case png_iccp_profile::unknown:
            _component_scale = 1.0f / 65535.0f;
            break;
        case png_iccp_profile::ITUR_2100_PQ_FULL:
            // HDR full scale luminosity is 1000 cd/m2.
            _component_scale = 12.5f / 65535.0f;
            break;
        }

        if (_delegate == nullptr) {
            png_error(png_ptr, "PNG: info_callback() delegate is null");
        }
        try {
            _delegate->info(this);
        } catch (...) {
            png_error(png_ptr, "PNG: info_callback() delegate->info() failed");
        }

        // Check if the rowbytes is correct: RRGGBBAA, 8 bytes.
        png_read_update_info(png_ptr, info_ptr);
        if (png_get_rowbytes(png_ptr, info_ptr) != _width * 8) {
            png_error(png_ptr, "PNG: info_callback() rowbytes != width * 8");
        }

        // Reserve the row to pass to the delegate later on.
        _row.resize(_width);
    }

    void row_callback(png_structp png_ptr, png_bytep src, png_uint_32 src_nr, int pass)
    {
        auto const num_columns = [&]{
            switch (_interlace_type) {
            case png_interlace_type::none:
                return width();
            case png_interlace_type::adam7:
                return width() >> PNG_PASS_COL_SHIFT(pass);
            }
            std::unreachable();
        }();

        auto const float_src = std::span{_row.data(), num_columns};

        auto const c_scale = _component_scale;
        constexpr auto a_scale = 1.0f / 65535.0f;

        if (_color_matrix) {
            auto const M = *_color_matrix;
            auto off = size_t{0};
            for (auto x = size_t{0}; x != num_columns; ++x, off += 8) {
                auto const RR = (static_cast<uint16_t>(src[off + 0]) << 8) | static_cast<uint16_t>(src[off + 1]);
                auto const GG = (static_cast<uint16_t>(src[off + 2]) << 8) | static_cast<uint16_t>(src[off + 3]);
                auto const BB = (static_cast<uint16_t>(src[off + 4]) << 8) | static_cast<uint16_t>(src[off + 5]);
                auto const AA = (static_cast<uint16_t>(src[off + 6]) << 8) | static_cast<uint16_t>(src[off + 7]);

                float_src[x] = M * color{RR * c_scale, GG * c_scale, BB * c_scale, AA * a_scale};
            }

        } else {
            auto off = size_t{0};
            for (auto x = size_t{0}; x != num_columns; ++x, off += 8) {
                auto const RR = (static_cast<uint16_t>(src[off + 0]) << 8) | static_cast<uint16_t>(src[off + 1]);
                auto const GG = (static_cast<uint16_t>(src[off + 2]) << 8) | static_cast<uint16_t>(src[off + 3]);
                auto const BB = (static_cast<uint16_t>(src[off + 4]) << 8) | static_cast<uint16_t>(src[off + 5]);
                auto const AA = (static_cast<uint16_t>(src[off + 6]) << 8) | static_cast<uint16_t>(src[off + 7]);

                float_src[x] = color{RR * c_scale, GG * c_scale, BB * c_scale, AA * a_scale};
            }
        }

        if (_delegate == nullptr) {
            png_error(png_ptr, "PNG: row_callback() delegate is null");
        }
        try {
            _delegate->row(this, float_src, src_nr, pass);
        } catch (...) {
            png_error(png_ptr, "PNG: row_callback() delegate->row() failed");
        }
    }

    void end_callback(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        if (_delegate == nullptr) {
            png_error(png_ptr, "PNG: end_callback() delegate is null");
        }
        try {
            _delegate->end(this);
        } catch (...) {
            png_error(png_ptr, "PNG: end_callback() delegate->row() failed");
        }
    }

    static void _info_callback(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        auto const self = std::launder(static_cast<png_loader*>(png_get_progressive_ptr(png_ptr)));
        if (self == nullptr) {
            png_error(png_ptr, "PNG: _info_callback() self is null.");
        }
        return self->info_callback(png_ptr, info_ptr);
    }

    static void _row_callback(png_structp png_ptr, png_bytep src, png_uint_32 src_nr, int pass)
    {
        auto const self = std::launder(static_cast<png_loader*>(png_get_progressive_ptr(png_ptr)));
        if (self == nullptr) {
            png_error(png_ptr, "PNG: _row_callback() self is null.");
        }

        return self->row_callback(png_ptr, src, src_nr, pass);
    }

    static void _end_callback(png_structp png_ptr, png_infop info_ptr) noexcept
    {
        auto const self = std::launder(static_cast<png_loader*>(png_get_progressive_ptr(png_ptr)));
        if (self == nullptr) {
            png_error(png_ptr, "PNG: _end_callback() self is null.");
        }

        return self->end_callback(png_ptr, info_ptr);
    }

    [[noreturn]] static void _user_error_fn(png_structp png_ptr, png_const_charp msg) noexcept
    {
        auto const self = std::launder(static_cast<png_loader*>(png_get_error_ptr(png_ptr)));
        assert(self != nullptr);

        self->_last_error_message = msg;
        std::longjmp(png_jmpbuf(png_ptr), 1);
    }

    void _cleanup() noexcept
    {
        _delegate = nullptr;
        if (std::setjmp(png_jmpbuf(_png_ptr))) {
            // At this point if something goes wrong, it must be really bad.
            std::abort();
        }

        if (_info_ptr) {
            png_destroy_info_struct(_png_ptr, &_info_ptr);
        }

        if (_png_ptr) {
            png_destroy_read_struct(&_png_ptr, nullptr, nullptr);
        }
    }
};

class png_loader_pixmap_delegate : public png_loader_delegate {
public:
    pixmap<sfloat_rgba16> image;

    png_loader_pixmap_delegate() : png_loader_delegate(), image() {}

    void info(png_loader* sender) override
    {
        image = pixmap<sfloat_rgba16>{sender->width(), sender->height()};
    }

    void row(png_loader* sender, std::span<sfloat_rgba16> src, size_t src_nr, unsigned int pass) override
    {
        auto const width = sender->width();
        auto const height = sender->height();

        std::span<sfloat_rgba16> dst_row = {};
        auto dst_nr = fill_start(src, src_nr, pass, width, height);
        for (unsigned int step = 0; dst_nr < height; ++step) {
            dst_nr = fill(src, src_nr, pass, step, width, height, image[height - dst_nr - 1]);
        }
    }
};

[[nodiscard]] inline pixmap<sfloat_rgba16> load_png(std::span<std::byte const> data)
{
    auto delegate = png_loader_pixmap_delegate{};
    auto loader = png_loader{delegate};
    loader.add_data(data);
    return std::move(delegate.image);
}

[[nodiscard]] inline pixmap<sfloat_rgba16> load_png(hi::file_view const &view)
{
    return load_png(as_span<std::byte const>(view));
}

[[nodiscard]] inline pixmap<sfloat_rgba16> load_png(std::filesystem::path path)
{
    return load_png(hi::file_view{path});
}

}

hi_warning_pop();
