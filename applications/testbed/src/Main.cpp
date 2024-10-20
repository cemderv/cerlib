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
    }

    void load_content() override
    {
    }

    bool update(const cer::GameTime& time) override
    {
        return true;
    }

    void draw(const cer::Window& window) override
    {
    }

    void draw_imgui(const cer::Window& window) override
    {
    }

    cer::Window window;
};

int main(int argc, char* argv[])
{
    return cer::run_game<Testbed>();
}
