

#pragma once


hi_export_module(hikogui.text : shaper_hl);

hi_export namespace hi::inline v1 {

class text_shaper {
public:
    constexpr text_shaper() noexcept = default;

    /** Set the style of the text.
     *
     * @param text_style The text style set for the text.
     */
    void set_style(text_style_set const& text_style);

    /** Set the base font size of the text.
     *
     * @param font_size The base font-size.
     */
    void set_font_size(unit::pixels_per_em font_size);

    /** Set the base color of the text.
     *
     * @param color The base color.
     */
    void set_color(hi::color color)

    /** Set the alignment of the text.
     *
     * @param alignment The horizontal and vertical alignment of the text.
     */
    void set_alignment(hi::alignment alignment);

    /** Set the text to format.
     *
     * @param text The text to shape.
     */
    void set_text(gstring const& text);

    extent2 get_size(std::optional<float> maximum_width = std::nullopt);


private:
    enum class state_type {
        /** No calculations are done.
         */
        idle,

        /** Sizes can be easilly calculated.
         */
        constrain,

        /** The text has been laid out.
         */
        layout
    };
};


}

