#include <cerlib.hpp>
#include <cerlib/Main.hpp>
#include <imgui.h>

using namespace std::chrono_literals;

class Testbed : public cer::Game
{
  public:
    Testbed()
    {
        window = cer::Window{"Testbed"};
        window.set_clear_color(cer::cornflowerblue * 0.5f);
    }

    void load_content() override
    {
        particles = cer::ParticleSystem{{
            cer::ParticleEmitter{
                .duration = 1s,
                // .shape       = cer::ParticleSprayShape{.direction = {1000, 0}, .spread = 100.0f},
                .image       = cer::load_image("blank.png"),
                .blend_state = cer::additive,
                .modifiers =
                    {
                        cer::ParticleOpacityMod{.initial_opacity = 1.0f, .final_opacity = 0.0f},
                        cer::ParticleScaleLerpMod{.initial_scale = 0.0f, .final_scale = 3.0f},
                        cer::ParticleRotationMod{.rotation_rate = cer::pi * 4.0f},
                        cer::ParticleVortexMod{.position = {}, .mass = 10.0f, .max_speed = 10.0f},
                    }},
        }};
    }

    bool update(const cer::GameTime& time) override
    {
        const auto dt = float(time.elapsed_time);
        particles.trigger_at(cer::current_mouse_position() * window.pixel_ratio());
        particles.update(dt);

        return true;
    }

    void draw(const cer::Window& window) override
    {
        cer::draw_particles(particles);
    }

    void draw_imgui(const cer::Window& window) override
    {
        ImGui::Begin("Particle Test");

        ImGui::SliderFloat("Speed", &particles.emitter_at(0).emission.speed.max, 0.0f, 10000.0f);

        ImGui::End();
    }

    cer::Window         window;
    cer::ParticleSystem particles;
};

int main(int argc, char* argv[])
{
    return cer::run_game<Testbed>();
}
