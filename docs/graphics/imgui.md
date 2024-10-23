# ImGui

ImGui is integrated into cerlib as a first-class component.

This means that it has tight integration and is automatically available to you.

ImGui is controlled via the `CERLIB_ENABLE_IMGUI` option, which is `ON` by default, so you don't have
to configure anything to access ImGui.

First, `#!cpp #include <imgui.h>` in your game code.

Then just override the `draw_imgui` method in your game class:

```cpp
struct MyGame : cer::Game
{
    // ...

    void draw_imgui(const cer::Window& window) override
    {
        ImGui::Begin("My ImGui Window");
        ImGui::Text("Hello World!");

        if (ImGui::Button("Click here"))
        {
            cer::log_info("Button was clicked!");
        }

        ImGui::End();
    }

    // ...
};
```

The `draw_imgui` method is automatically called by cerlib at the right time, **after** `draw` is called.

Because cerlib does not provide a customized ImGui API, but the ImGui API directly, please visit the
[ImGui website](https://github.com/ocornut/imgui) for further examples and tutorials on how to use it.
