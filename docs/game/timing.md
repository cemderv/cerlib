# Timing

Animations in your game mainly happen in the `update` method, which has a parameter called `time` of type [`GameTime`](../api/Game/GameTime/index.md#gametime):

The [`GameTime`](../api/Game/GameTime/index.md#gametime) structure is defined as follows:

```cpp
// Represents timing information about a running game.
struct GameTime
{
    /** The time that has elapsed since the last frame, in fractional seconds */
    double elapsed_time{};

    /** The time that has elapsed since the game started running, in fractional seconds */
    double total_time{};
};
```

This informs you about how much time has passed since the last call to `update` was made (_delta time_). By default, `update` is called in an interval equivalent to the display’s refresh rate. This means that on a 60 Hz display the delta time would be `#!cpp 1.0 / 60 = 0.1666` seconds, assuming that the system is able to keep up with this frame rate.

On this page we're going to animate a sprite's movement based on the total run time of the game.
Simply add a variable to your game and update it in the `update` method:

```cpp
struct MyGame : cer::Game
{
    bool update(const cer::GameTime& time) override
    {
        sprite_animation_time = time.total_time;
        return true;
    }

    // ...

    double sprite_animation_time = 0.0;
};
```

Modify the `draw` method so that the sprite is drawn at a different position, based on the time:

```cpp
void draw(const cer::Window& window) override
{
    const auto distance = 50.0;
    const auto speed    = 2.0;
    const auto offset   = float(cer::sin(sprite_animation_time * speed) * distance);

    cer::draw_sprite(my_image, {100.0f + offset, 50.0f});
}
```

The sprite should now move in a wavy pattern along the X-axis, due to the [`cer::sin`](../api/Math/index.md#sin) function used.

<figure markdown="span">
    ![img](../img/YourFirstSpriteAnim-1.gif){ width="240" }
</figure>

---

Related pages:

* [Drawing Sprites](../graphics/drawing-sprites.md)

