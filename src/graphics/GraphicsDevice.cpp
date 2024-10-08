// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "GraphicsDevice.hpp"
#include "FontImpl.hpp"
#include "ImageImpl.hpp"
#include "ShaderImpl.hpp"
#include "SpriteBatch.hpp"
#include "cerlib/Logging.hpp"
#include "cerlib/ParticleSystem.hpp"
#include "shadercompiler/BinOpTable.hpp"
#include "shadercompiler/BuiltInSymbols.hpp"
#include "shadercompiler/Casting.hpp"
#include "shadercompiler/Expr.hpp"
#include "shadercompiler/GLSLShaderGenerator.hpp"
#include "shadercompiler/Lexer.hpp"
#include "shadercompiler/Naming.hpp"
#include "shadercompiler/Parser.hpp"
#include "shadercompiler/Scope.hpp"
#include "shadercompiler/SemaContext.hpp"
#include "shadercompiler/Token.hpp"
#include "shadercompiler/Type.hpp"
#include "shadercompiler/TypeCache.hpp"
#include "util/InternalError.hpp"
#include "util/StringViewUnorderedSet.hpp"
#include <cassert>
#include <gsl/util>
#include <ranges>

namespace cer::details
{
GraphicsDevice::GraphicsDevice()
    : m_must_flush_draw_calls(false)
    , m_blend_state(BlendState::non_premultiplied())
    , m_sampler(Sampler::linear_clamp())
{
    FontImpl::create_built_in_fonts();
}

void GraphicsDevice::notify_resource_created(gsl::not_null<GraphicsResourceImpl*> resource)
{
    assert(std::ranges::find(m_resources, resource) == m_resources.cend());
    m_resources.push_back(resource);
}

void GraphicsDevice::notify_resource_destroyed(gsl::not_null<GraphicsResourceImpl*> resource)
{
    m_resources.erase(std::ranges::find(std::as_const(m_resources), resource));
}

void GraphicsDevice::notify_user_shader_destroyed(gsl::not_null<ShaderImpl*> resource)
{
    m_sprite_batch->on_shader_destroyed(resource);
}

GraphicsDevice::~GraphicsDevice() noexcept
{
    log_verbose("Destroying GraphicsDevice");

    assert(!m_current_window);
    m_sprite_shader = {};
    m_sprite_batch->release_resources();
}

void GraphicsDevice::start_frame(const Window& window)
{
    m_current_window = window;
    m_draw_stats     = {};

    set_canvas({}, true);
    m_current_category = {};

    if (auto* impl = dynamic_cast<ShaderImpl*>(m_sprite_shader.impl()))
    {
        impl->m_is_in_use = true;
    }

    on_start_frame(window);
}

void GraphicsDevice::end_frame(const Window&                window,
                               const std::function<void()>& post_draw_callback)
{
    flush_draw_calls();

    if (post_draw_callback)
    {
        post_draw_callback();
    }

    on_end_frame(window);

    m_current_window = {};
    m_canvas         = {};

    if (auto* impl = dynamic_cast<ShaderImpl*>(m_sprite_shader.impl()))
    {
        impl->m_is_in_use = false;
    }
}

void GraphicsDevice::start_imgui_frame(const Window& window)
{
    on_start_imgui_frame(window);
}

void GraphicsDevice::end_imgui_frame(const Window& window)
{
    on_end_imgui_frame(window);
}

static auto to_parameter_type(const shadercompiler::Type& type) -> ShaderParameterType
{
    if (&type == &shadercompiler::FloatType::instance())
    {
        return ShaderParameterType::Float;
    }

    if (&type == &shadercompiler::Vector2Type::instance())
    {
        return ShaderParameterType::Vector2;
    }

    if (&type == &shadercompiler::Vector3Type::instance())
    {
        return ShaderParameterType::Vector3;
    }

    if (&type == &shadercompiler::Vector4Type::instance())
    {
        return ShaderParameterType::Vector4;
    }

    if (&type == &shadercompiler::IntType::instance())
    {
        return ShaderParameterType::Int;
    }

    if (&type == &shadercompiler::BoolType::instance())
    {
        return ShaderParameterType::Bool;
    }

    if (&type == &shadercompiler::MatrixType::instance())
    {
        return ShaderParameterType::Matrix;
    }

    if (&type == &shadercompiler::ImageType::instance())
    {
        return ShaderParameterType::Image;
    }

    if (type.is_array())
    {
        const auto& element_type =
            dynamic_cast<const shadercompiler::ArrayType*>(&type)->element_type();

        if (&element_type == &shadercompiler::FloatType::instance())
        {
            return ShaderParameterType::FloatArray;
        }

        if (&element_type == &shadercompiler::Vector2Type::instance())
        {
            return ShaderParameterType::Vector2Array;
        }

        if (&element_type == &shadercompiler::Vector3Type::instance())
        {
            return ShaderParameterType::Vector3Array;
        }

        if (&element_type == &shadercompiler::Vector4Type::instance())
        {
            return ShaderParameterType::Vector4Array;
        }

        if (&element_type == &shadercompiler::IntType::instance())
        {
            return ShaderParameterType::IntArray;
        }

        if (&element_type == &shadercompiler::BoolType::instance())
        {
            return ShaderParameterType::BoolArray;
        }

        if (&element_type == &shadercompiler::MatrixType::instance())
        {
            return ShaderParameterType::MatrixArray;
        }
    }

    CER_THROW_INTERNAL_ERROR_STR("Invalid parameter type encountered");
}

auto GraphicsDevice::demand_create_shader(std::string_view                  name,
                                          std::string_view                  source_code,
                                          std::span<const std::string_view> defines)
    -> gsl::not_null<ShaderImpl*>
{
    auto tokens = std::vector<shadercompiler::Token>{};
    do_lexing(source_code, name, true, tokens);

    auto type_cache       = shadercompiler::TypeCache{};
    auto built_in_symbols = shadercompiler::BuiltInSymbols{};
    auto bin_op_table     = shadercompiler::BinOpTable{};
    auto parser           = shadercompiler::Parser{type_cache};
    auto decls            = parser.parse(tokens);

    auto defines_set = StringViewUnorderedSet{};

    for (const auto& define : defines)
    {
        defines_set.insert(define);
    }

    auto ast = shadercompiler::AST{name, std::move(decls), &defines_set};

    auto context      = shadercompiler::SemaContext{ast, built_in_symbols, bin_op_table};
    auto global_scope = shadercompiler::Scope{};

    context.set_allow_forbidden_identifier_prefix(true);

    for (const auto& symbol : built_in_symbols.all_decls())
    {
        symbol->verify(context, global_scope);
    }

    for (const auto& symbol : built_in_symbols.all_decls())
    {
        if (auto* var = asa<shadercompiler::VarDecl>(symbol.get());
            var != nullptr && var->is_system_value())
        {
            global_scope.remove_symbol(*var);
        }
    }

    context.set_allow_forbidden_identifier_prefix(false);

    ast.verify(context, global_scope);

#ifdef CERLIB_GFX_IS_GLES
    constexpr bool is_gles = true;
#else
    constexpr bool is_gles = false;
#endif

    auto glsl_code_generator = shadercompiler::GLSLShaderGenerator{is_gles};

    const auto code_gen_results =
        glsl_code_generator.generate(context,
                                     ast,
                                     shadercompiler::naming::shader_entry_point,
                                     true);

    log_verbose("Generated OpenGL shader code: {}", code_gen_results.glsl_code);

    auto parameters = ShaderImpl::ParameterList{};
    parameters.reserve(code_gen_results.parameters.size());

    for (const auto* param : code_gen_results.parameters)
    {
        const auto type       = to_parameter_type(param->type());
        const auto array_size = param->is_array() ? param->array_size() : 0u;

        const auto calculate_size_in_bytes = [type, array_size] {
            switch (type)
            {
                case ShaderParameterType::Float:
                case ShaderParameterType::Int:
                case ShaderParameterType::Bool: return sizeof(int32_t);
                case ShaderParameterType::Vector2: return sizeof(float) * 2;
                case ShaderParameterType::Vector3: return sizeof(float) * 3;
                case ShaderParameterType::Vector4: return sizeof(float) * 4;
                case ShaderParameterType::Matrix: return sizeof(float) * 4 * 4;
                case ShaderParameterType::Image: return size_t();
                case ShaderParameterType::FloatArray: return sizeof(float) * array_size;
                case ShaderParameterType::IntArray: return sizeof(int32_t) * array_size;
                case ShaderParameterType::BoolArray: return sizeof(int32_t) * array_size;
                case ShaderParameterType::Vector2Array: return sizeof(float) * 2 * array_size;
                case ShaderParameterType::Vector3Array: return sizeof(float) * 3 * array_size;
                case ShaderParameterType::Vector4Array: return sizeof(float) * 4 * array_size;
                case ShaderParameterType::MatrixArray: return sizeof(float) * 3 * 3 * array_size;
            }

            CER_THROW_INTERNAL_ERROR_STR("Invalid parameter type encountered");
        };

        const auto size_in_bytes = gsl::narrow_cast<uint16_t>(calculate_size_in_bytes());
        const auto is_image      = type == ShaderParameterType::Image;

        if (!is_image)
        {
            assert(size_in_bytes > 0);
        }

        parameters.push_back(ShaderParameter{
            .name          = std::string{param->name()},
            .type          = type,
            .offset        = /*offset will be determined later:*/ 0,
            .size_in_bytes = size_in_bytes,
            .array_size    = param->is_array() ? param->array_size() : uint16_t(0),
            .is_image      = is_image,
            .image         = {},
            .default_value = param->default_value(),
        });
    }

    gsl::not_null shader =
        create_native_user_shader(code_gen_results.glsl_code, std::move(parameters)).release();

    shader->set_name(name);

    return shader;
}

auto GraphicsDevice::all_resources() const
    -> const std::vector<gsl::not_null<GraphicsResourceImpl*>>&
{
    return m_resources;
}

auto GraphicsDevice::current_canvas() const -> const Image&
{
    return m_canvas;
}

void GraphicsDevice::set_canvas(const Image& canvas, bool force)
{
    if (canvas)
    {
        if (const auto* image_impl = dynamic_cast<const ImageImpl*>(canvas.impl());
            image_impl->window_for_canvas() != m_current_window.impl())
        {
            CER_THROW_INVALID_ARG_STR("The specified canvas image is not compatible with the "
                                      "current window. A canvas can "
                                      "only be used within the window it was created for.");
        }
    }

    if (m_canvas != canvas || force)
    {
        m_canvas = canvas;
        flush_draw_calls();

        auto new_viewport = Rectangle{};
        if (canvas)
        {
            const auto [width, height] = canvas.size();
            new_viewport.width         = width;
            new_viewport.height        = height;
        }
        else
        {
            const auto [windowWidth, windowHeight] = m_current_window.size_px();
            new_viewport.width                     = windowWidth;
            new_viewport.height                    = windowHeight;
        }

        if (new_viewport != m_viewport)
        {
            m_viewport                = new_viewport;
            m_viewport_transformation = compute_viewport_transformation(m_viewport);
            compute_combined_transformation();
        }

        on_set_canvas(canvas, new_viewport);
    }
}

void GraphicsDevice::set_scissor_rects(std::span<const Rectangle> scissor_rects)
{
    flush_draw_calls();
    on_set_scissor_rects(scissor_rects);
}

void GraphicsDevice::set_transformation(const Matrix& transformation)
{
    m_transformation = transformation;
    compute_combined_transformation();
    m_must_flush_draw_calls = true;
}

auto GraphicsDevice::current_sprite_shader() const -> const Shader&
{
    return m_sprite_shader;
}

void GraphicsDevice::set_sprite_shader(const Shader& pixel_shader)
{
    if (m_sprite_shader != pixel_shader)
    {
        if (auto* impl = dynamic_cast<ShaderImpl*>(m_sprite_shader.impl()))
        {
            impl->m_is_in_use = false;
        }

        m_sprite_shader         = pixel_shader;
        m_must_flush_draw_calls = true;

        if (auto* impl = dynamic_cast<ShaderImpl*>(m_sprite_shader.impl()))
        {
            impl->m_is_in_use = true;
        }
    }
}

void GraphicsDevice::set_sampler(const Sampler& sampler)
{
    if (m_sampler != sampler)
    {
        m_sampler               = sampler;
        m_must_flush_draw_calls = true;
    }
}

auto GraphicsDevice::current_blend_state() const -> const BlendState&
{
    return m_blend_state;
}

void GraphicsDevice::set_blend_state(const BlendState& blend_state)
{
    if (m_blend_state != blend_state)
    {
        m_blend_state           = blend_state;
        m_must_flush_draw_calls = true;
    }
}

void GraphicsDevice::draw_sprite(const Sprite& sprite)
{
    ensure_category(Category::SpriteBatch);
    m_sprite_batch->draw_sprite(sprite, SpriteBatch::SpriteShaderKind::Default);
}

void GraphicsDevice::draw_string(std::string_view                     text,
                                 const Font&                          font,
                                 uint32_t                             font_size,
                                 const Vector2&                       position,
                                 const Color&                         color,
                                 const std::optional<TextDecoration>& decoration)
{
    ensure_category(Category::SpriteBatch);
    m_sprite_batch->draw_string(text, font, font_size, position, color, decoration);
}

void GraphicsDevice::draw_text(const Text& text, Vector2 position, const Color& color)
{
    ensure_category(Category::SpriteBatch);
    m_sprite_batch->draw_text(text, position, color);
}

void GraphicsDevice::draw_particles(const ParticleSystem& particle_system)
{
    const auto previous_blend_state = m_blend_state;


    for (const auto& emitter_data : particle_system.m_emitters)
    {
        const auto& emitter = emitter_data.emitter;
        const auto& image   = emitter.image;

        if (!image)
        {
            continue;
        }

        set_blend_state(emitter.blend_state);
        ensure_category(Category::SpriteBatch);

        const auto image_size = image.size();
        const auto origin     = image_size * 0.5f;

        const auto particles_span =
            std::span{emitter_data.particle_buffer.data(), emitter_data.active_particle_count};

        auto sprite = Sprite{
            .image  = image,
            .origin = origin,
        };

        for (const auto& particle : particles_span)
        {
            sprite.dst_rect = {particle.position, image_size * particle.scale};
            sprite.color    = particle.color;
            sprite.rotation = particle.rotation;

            m_sprite_batch->draw_sprite(sprite, SpriteBatch::SpriteShaderKind::Default);
        }
    }

    set_blend_state(previous_blend_state);
}

void GraphicsDevice::fill_rectangle(const Rectangle& rectangle,
                                    const Color&     color,
                                    float            rotation,
                                    const Vector2&   origin)
{
    ensure_category(Category::SpriteBatch);
    m_sprite_batch->fill_rectangle(rectangle, color, rotation, origin);
}

auto GraphicsDevice::current_window() const -> const Window&
{
    return m_current_window;
}

auto GraphicsDevice::frame_stats_ptr() -> gsl::not_null<FrameStats*>
{
    return &m_draw_stats;
}

void GraphicsDevice::ensure_category(Category category)
{
    if (!m_current_category || *m_current_category != category || m_must_flush_draw_calls)
    {
        flush_draw_calls();

        switch (category)
        {
            case Category::SpriteBatch:
                m_sprite_batch->begin(m_combined_transformation,
                                      m_blend_state,
                                      m_sprite_shader,
                                      m_sampler);
                break;
        }

        m_current_category = category;
    }
}

void GraphicsDevice::flush_draw_calls()
{
    if (m_current_category)
    {
        switch (*m_current_category)
        {
            case Category::SpriteBatch: m_sprite_batch->end(); break;
        }

        m_current_category = {};
    }

    m_must_flush_draw_calls = false;
}

auto GraphicsDevice::compute_viewport_transformation(const Rectangle& viewport) -> Matrix
{
    const auto x_scale = viewport.width > 0 ? 2.0f / viewport.width : 0.0f;
    const auto y_scale = viewport.height > 0 ? 2.0f / viewport.height : 0.0f;

    return {x_scale, 0, 0, 0, 0, -y_scale, 0, 0, 0, 0, 1, 0, -1, 1, 0, 1};
}

void GraphicsDevice::compute_combined_transformation()
{
    m_combined_transformation = m_transformation * m_viewport_transformation;
}

auto GraphicsDevice::frame_stats() const -> FrameStats
{
    return m_draw_stats;
}

auto GraphicsDevice::current_canvas_size() const -> Vector2
{
    return m_viewport.size();
}

void GraphicsDevice::post_init(std::unique_ptr<SpriteBatch> sprite_batch)
{
    assert(sprite_batch);

    FontImpl::create_built_in_fonts();

    m_sprite_batch = std::move(sprite_batch);
}
} // namespace cer::details
