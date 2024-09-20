#include "cerlib/Drawing.hpp"
#include <cerlib.hpp>

#include <cerlib/Main.hpp>

class Testbed : public cer::Game
{
  public:
    Testbed()
    {
        window = cer::Window("Testbed");
    }

    void load_content() override
    {
        img = cer::load_image("logo.png");
    }

    bool update(const cer::GameTime& time) override
    {
        return true;
    }

    void draw(const cer::Window& window) override
    {
        draw_sprite(img, (window.size_px() - img.size()) / 2);
    }

    cer::Window window;
    cer::Image  img;
};

int main(int argc, char* argv[])
{
    return cer::run_game<Testbed>();
}
