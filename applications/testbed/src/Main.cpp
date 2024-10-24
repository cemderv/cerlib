#include <cerlib.hpp>
#include <cerlib/Main.hpp>
#include <imgui.h>

using namespace cer;

static constexpr auto game_code = R"(
local anim = 0.0
local img = Image('logo.png')

function update()
    anim = anim + time.elapsed_time * 20
end

function draw()
    draw_sprite_simple(img, Vector2(100 + anim, 200), white)
end
)"_lua;

struct Testbed : Game
{
    void load_content() override
    {
        lua_state = LuaState{LuaLibraries::All,
                             {
                                 LuaScript{"SomeGameCode", game_code},
                             }};
    }

    auto update(const GameTime& time) -> bool override
    {
        lua_state.set_variable("time", time);
        lua_state.run_code("update()");

        return !lua_state.variable_as<bool>("should_exit").value_or(false);
    }

    void draw(const Window& window) override
    {
        lua_state.set_variable("window", window);
        lua_state.run_code("draw()");
    }

    Window   window{"Testbed"};
    LuaState lua_state;
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    return cer::run_game<Testbed>();
}
