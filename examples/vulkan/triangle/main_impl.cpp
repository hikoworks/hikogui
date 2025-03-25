// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "triangle.hpp"
#include "hikogui/hikogui.hpp"
#include "hikogui/crt.hpp"
#include <ranges>
#include <cassert>

// Every widget must inherit from hi::widget.
class triangle_widget : public hi::widget, public hi::gfx_surface_delegate {
public:
    // Every constructor of a widget starts with a `window` and `parent` argument.
    // In most cases these are automatically filled in when calling a container widget's `emplace()` function.
    triangle_widget(hi::gfx_surface& surface) noexcept : widget(), _surface(surface)
    {
        _surface.add_delegate(this);
    }

    ~triangle_widget()
    {
        _surface.remove_delegate(this);
    }

    // The set_constraints() function is called when the window is first initialized,
    // or when a widget wants to change its constraints.
    [[nodiscard]] hi::box_constraints update_constraints() noexcept override
    {
        // Certain expensive calculations, such as loading of images and shaping of text
        // can be done in this function.

        // The constrains below have different minimum, preferred and maximum sizes.
        // When the window is initially created it will try to size itself so that
        // the contained widgets are at their preferred size. Having a different minimum
        // and/or maximum size will allow the window to be resizable.
        return {{400, 300}, {640, 480}, {1024, 860}, style.margins_px};
    }

    // The `set_layout()` function is called when the window has resized, or when
    // a widget wants to change the internal layout.
    //
    // NOTE: The size of the layout may be larger than the maximum constraints of this widget.
    void set_layout(hi::widget_layout const& context) noexcept override
    {
        hi::widget::set_layout(context);

        auto view_port = context.rectangle_on_window();
        auto window_height = context.window_size.height();

        // We calculate the view-port used for 3D rendering from the location and size
        // of the widget within the window. We use the window-height so that we can make
        // Vulkan compatible coordinates. Vulkan uses y-axis down, while HikoGUI uses y-axis up.
        _view_port = VkRect2D{
            VkOffset2D{hi::round_cast<int32_t>(view_port.left()), hi::round_cast<int32_t>(window_height - view_port.top())},
            VkExtent2D{hi::round_cast<uint32_t>(view_port.width()), hi::round_cast<uint32_t>(view_port.height())}};
    }

    // The `draw()` function is called when all or part of the window requires redrawing.
    // This may happen when showing the window for the first time, when the operating-system
    // requests a (partial) redraw, or when a widget requests a redraw of itself.
    //
    // This draw() function only draws the GUI part of the widget, there is another draw() function
    // that will draw the 3D part.
    void draw(hi::draw_context const& context) const noexcept override
    {
        // We request a redraw for each frame, in case the 3D model changes on each frame.
        // In normal cases we should take into account if the 3D model actually changes before requesting a redraw.
        request_redraw();

        // We only need to draw the widget when it is visible and when the visible area of
        // the widget overlaps with the scissor-rectangle (partial redraw) of the drawing context.
        if (mode() > hi::widget_mode::invisible and overlaps(context, layout())) {
            // The 3D drawing will be done directly on the swap-chain before the GUI is drawn.
            // By making a hole in the GUI we can show the 3D drawing underneath it, otherwise
            // the solid-background color of the GUI would show instead.
            context.draw_hole(layout(), layout().rectangle());
        }

        return super::draw(context);
    }

    // This draw() function draws the 3D model.
    // It is called before the GUI is drawn and allows drawing directly onto the swap-chain.
    //
    // As HikoGUI reuses previous drawing of the swap-chain it is important to let the render-pass
    // load the data from the frame-buffer (not set to don't-care) and to not render outside the @a render_area.
    void draw(uint32_t swapchain_index, vk::Semaphore start, vk::Semaphore finish, vk::Rect2D render_area) const noexcept override
    {
        assert(_triangle_example != nullptr);

        // The _triangle_example is the "vulkan graphics engine", into which we pass:
        //  - Which swap-chain image to draw into,
        //  - the semaphores when to start drawing, and when the drawing is finished.
        //  - The render-area, which is like the dirty-rectangle that needs to be redrawn.
        //  - View-port the part of the frame buffer that matches this widget's rectangle.
        //
        // The "vulkan graphics engine" is responsible to not drawn outside the neither
        // the render-area nor outside the view-port.
        _triangle_example->render(
            swapchain_index,
            static_cast<VkSemaphore>(start),
            static_cast<VkSemaphore>(finish),
            static_cast<VkRect2D>(render_area),
            _view_port);
    }

    // This function is called when the vulkan-device changes.
    void build_for_new_device(
        VmaAllocator allocator,
        vk::Instance instance,
        vk::Device device,
        vk::Queue graphics_queue,
        uint32_t graphics_queue_family_index) noexcept override
    {
        // In our case if the vulkan-device changes, then we restart the complete "graphics engines".
        _triangle_example = std::make_shared<TriangleExample>(
            allocator, static_cast<VkDevice>(device), static_cast<VkQueue>(graphics_queue), graphics_queue_family_index);
    }

    // This function is called when the swap-chain changes, this can happen:
    //  - When a new window is created with this widget.
    //  - When the widget is moved to another window.
    //  - When the size of the window changes.
    //
    void build_for_new_swapchain(std::vector<vk::ImageView> const& views, vk::Extent2D size, vk::SurfaceFormatKHR format) noexcept
        override
    {
        assert(_triangle_example != nullptr);

        // Create a list of old style VkImageView of the swap-chain, HikoGUI uses the C++ vulkan bindings internally.
        auto views_ = std::ranges::to<std::vector>(std::views::transform(views, [](auto const& view) {
            return static_cast<VkImageView>(view);
        }));

        // Tell the "graphics engine" to make itself ready for a new swap-chain.
        // This often means the setup of most of the graphics pipelines and render-passes.
        _triangle_example->buildForNewSwapchain(views_, static_cast<VkExtent2D>(size), static_cast<VkFormat>(format.format));
    }

    // This function is called when the vulkan-device has gone away.
    // This may happen:
    //  - When the application is closed.
    //  - When the GPU device has a problem.
    void teardown_for_device_lost() noexcept override
    {
        // We shutdown the "graphics engine"
        _triangle_example = {};
    }

    // This function is called the surface is going away.
    // This may happen:
    //  - The window is closed.
    //  - The widget is being moved to another window.
    //  - The window is resizing.
    void teardown_for_swapchain_lost() noexcept override
    {
        assert(_triangle_example != nullptr);

        // Tell the graphics engine to tear down the pipelines and render-passes and everything
        // that is connected to the swap-chain.
        _triangle_example->teardownForLostSwapchain();
    }

private:
    hi::gfx_surface& _surface;
    std::shared_ptr<TriangleExample> _triangle_example;
    VkRect2D _view_port;
};

// This is a co-routine that manages the main window.
hi::task<> main_window()
{
    // Load the icon to show in the upper left top of the window.
    auto icon = hi::icon(hi::png::load(hi::URL{"resource:vulkan_triangle.png"}));

    // Create a window, when `window` gets out-of-scope the window is destroyed.
    auto widget_ptr = std::make_unique<hi::window_widget>(hi::label{std::move(icon), hi::txt("Vulkan Triangle")});
    auto& widget = *widget_ptr;

    // Create the window before we add the triangle widget as we need to get
    // the gfx_surface of the window to let the widget register itself to it.
    auto window = hi::gui_window{std::move(widget_ptr)};

    // Create the vulkan triangle-widget as the content of the window. The content
    // of the window is a grid, we only use the cell "A1" for this widget.
    widget.content().emplace<triangle_widget>("A1", *window.surface);

    // Wait until the window is "closing" because the operating system says so, or when
    // the X is pressed.
    co_await window.closing;
}

// The main (platform independent) entry point of the application.
int hi_main(int argc, char* argv[])
{
    hi::set_application_name("Triangle example");
    hi::set_application_vendor("HikoGUI");
    hi::set_application_version({1, 0, 0});

    // Start the RenderDoc server so that the application is easy to debug in RenderDoc.
    hi::start_render_doc();

    // Create and manage the main-window.
    main_window();

    // Start the main-loop until the main-window is closed.
    return hi::loop::main().resume();
}
