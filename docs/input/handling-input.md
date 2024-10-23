# Handling Input

A game probably would not be that fun if you could not interact with it. Handling user input in your game
is relatively straightforward with cerlib.

It handles the input state management for you and provides a number of functions ready for use.

Let's first add some variables to your game that we can manipulate with some input:

```cpp
struct MyGame : cer::Game
{
    // ...

    void draw(const cer::Window& window) override
    {
        cer::draw_sprite(cer::Sprite {
            .image    = my_image,
            .dst_rect = { sprite_position, sprite_size },
            .origin   = my_image.size() / 2,
        });
    }

    cer::Vector2 sprite_position = {100, 100};
    float sprite_size            = 1.0f;
};
```

## Keyboard

First up is the keyboard. We want to move the sprite in the direction the user is pressing.
The easiest place to check the user's input is in the `update` method, since it runs once per
frame and just before the game is drawn.

It is as simple as:

```cpp
bool update(const cer::GameTime& time) override
{
  const auto dt       = float(time.elapsed_time);
  const auto movement = dt * 200.0f;
 
  if (cer::is_key_down(cer::Key::Up))
    sprite_position.y -= movement;
 
  if (cer::is_key_down(cer::Key::Down))
    sprite_position.y += movement;
 
  if (cer::is_key_down(cer::Key::Left))
    sprite_position.x -= movement;
 
  if (cer::is_key_down(cer::Key::Right))
    sprite_position.x += movement;
}
```

Now we should be able to move the sprite around using the arrow keys. A `movement` variable is used to amplify the amount of movement. A value of `#!cpp 200` multiplied by `dt` indicates that the sprite is able to move at most `#!cpp 200` units (pixels) per second, since `dt` is based on fractional seconds, where `#!cpp dt=1` means `#!cpp 1 second`.

The opposite of [`cer::is_key_down`](../api/Input/index.md#is_key_down) is also available, called [`cer::is_key_up`](../api/Input/index.md#is_key_up).

cerlib provides an additional function called [`cer::was_key_just_pressed`](../api/Input/index.md#was_key_just_pressed), which can be used to check a single key press.

This is useful for one-time actions, where it’s not unusual that the player presses a key but keeps holding it
for a couple of frames longer. If we used [`cer::is_key_down`](../api/Input/index.md#is_key_down), we would perform the action across multiple
frames, which is probably not what we want.

We only want to know **if** the player pressed that key, not **how long**. As an example, we’ll modify our `sprite_size` variable whenever ++u++ or ++d++ was pressed.

Append the following to our input checks:

```cpp
if (cer::was_key_just_pressed(cer::Key::U))
  sprite_size += 0.25f;
 
if (cer::was_key_just_pressed(cer::Key::D))
  sprite_size -= 0.25f;
```

We can now scale the sprite up and down by pressing the respective keys.

The opposite of [`cer::was_key_just_pressed`](../api/Input/index.md#was_key_just_pressed) is also available, called [`cer::was_key_just_released`](../api/Input/index.md#was_key_just_released).
This is true if the player held a key pressed during the previous frames but released it just this frame.

## Mouse

Checking for mouse input is very similar to checking the keyboard’s input.
The mouse however additionally provides a pointer position as well as a scroll wheel.

Let’s make use of the mouse position first. We use the convenience function
[`cer::current_mouse_position_delta`](../api/Input/index.md#current_mouse_position_delta) to obtain how much the mouse moved since the last frame.
If the mouse hasn’t moved, it returns a zero vector.
If the mouse has moved, then we set the sprite’s position as the current mouse position.

Append the following to our input checks:

```cpp
const auto mouse_motion = cer::mouse_position_delta();
 
if (!mouse_motion.is_zero())
  spritePosition = cer::mouse_position() * window.pixel_ratio();
```

The mouse position must be multiplied by the window’s pixel ratio, in case the window’s display is a high DPI display. This is necessary because the mouse position is expressed in logical display units, not pixels. Multiplying it with the pixel ratio converts it to pixel space, which is what cerlib’s rendering uses. If you don’t intent to support high DPI displays in your game, you don’t have to take the pixel ratio into account and can leave the multiplication out.

The sprite should now follow the mouse whenever we move the mouse.
Otherwise, it moves based on keyboard input.
Let’s also use [`cer::is_mouse_button_down`](../api/Input/index.md#is_mouse_button_down) to move the sprite around.

Modify the keyboard checks as follows:

```cpp
if (cer::is_key_down(cer::Key::Up) || cer::is_mouse_button_down(cer::MouseButton::Left))
  sprite_position.y -= movement;
 
if (cer::is_key_down(cer::Key::Down) || cer::is_mouse_button_down(cer::MouseButton::Right))
  sprite_position.y += movement;
```

[`cer::is_mouse_button_down`](../api/Input/index.md#is_mouse_button_down) and [`cer::is_mouse_button_up`](../api/Input/index.md#is_mouse_button_up) are the equivalent of [`cer::is_key_down`](../api/Input/index.md#is_key_down) and [`cer::is_key_up`](../api/Input/index.md#is_key_up) for mouse buttons.

Next, we also allow resizing the sprite using the mouse wheel.

Just modify our keyboard checks:

```cpp
const auto wheel_delta = cer::current_mouse_wheel_delta();
 
if (cer::was_key_just_pressed(cer::Key::U) || wheel_delta.y > 0)
  sprite_size += 0.25f;
 
if (cer::was_key_just_pressed(cer::Key::D) || wheel_delta.y < 0)
  sprite_size -= 0.25f;
```

---

API Reference:

* [Input](../api/Input/index.md)