#include "RenderingTestHelper.hpp"

#include "contentmanagement/FileSystem.hpp"
#include <cerlib/Color.hpp>
#include <cerlib/Drawing.hpp>
#include <cerlib/Logging.hpp>
#include <format>
#include <iostream>
#include <snitch/snitch.hpp>

RenderingTestHelper::RenderingTestHelper(uint32_t width, uint32_t height, const cer::Window& window)
    : m_canvas(width, height, cer::ImageFormat::R8G8B8A8_UNorm, window)
{
    m_canvas.set_canvas_clear_color(cer::black);
}

void RenderingTestHelper::test_render(std::string_view test_name, const RenderFunction& function)
{
    cer::log_info("Rendering image for test '{}'", test_name);

    cer::set_canvas(m_canvas);
    function();
    cer::set_canvas({});

    const auto rendered_image_data =
        cer::read_canvas_data(m_canvas, 0, 0, m_canvas.width(), m_canvas.height());

    const auto reference_image_data = cer::filesystem::decode_image_data_from_file_on_disk(
        get_reference_image_filename(test_name));

    if (rendered_image_data != reference_image_data)
    {
        // Save rendered image for inspection.
        cer::filesystem::encode_image_data_to_file_on_disk("RENDERING_MISMATCH.png",
                                                           rendered_image_data,
                                                           m_canvas.width(),
                                                           m_canvas.height());

        // Trigger test failure
        REQUIRE(rendered_image_data == reference_image_data);
    }
}

void RenderingTestHelper::generate_reference_image(std::string_view      test_name,
                                                   const RenderFunction& function)
{
    cer::log_info("Generating reference image for test '{}'", test_name);

    cer::set_canvas(m_canvas);
    function();
    cer::set_canvas({});
    cer::save_canvas_to_file(m_canvas, get_reference_image_filename(test_name));
}

std::string RenderingTestHelper::get_reference_image_filename(std::string_view test_name) const
{
    return std::format("{}/{}.png", REFERENCE_IMAGES_DIR, test_name);
}
