# Content Management

Content management in the context of games relates to how a game loads, manages and unloads its assets such as images, sounds, fonts and shaders.

cerlib makes this process as easy as possible, while being equally as efficient.
The assets of a game are stored in the `assets` folder.

Let's have a look at how to load and draw the cerlib logo:

<figure markdown="span">
    ![img](../img/cerlib-logo300.png){ width="100" }
</figure>

Simply drop this image into the `assets` folder, then load in the `load_content` method of your game:

```cpp
struct MyGame : cer::Game
{
    void load_content() override
    {
        my_image = cer::Image{"cerlib-logo300.png"};
    }

    // ...

    cer::Image my_image;
};
```

There is no special asset loading function in cerlib. Any asset type such as [`cer::Image`](../api/Graphics/Image/index.md),
[`cer::Font`](../api/Graphics/Font/index.md) and [`cer::Sound`](../api/Audio/Sound/index.md) is loadable directly using its constructor that takes a single
string argument.

To draw the image, call [`cer::draw_sprite`](../api/Graphics/index.md#draw_sprite) in the game's `draw` method:

```cpp
void draw(const cer::Window& window) override
{
    // Draw the image at position {200, 200}, with a red tint.
    cer::draw_sprite(my_image, {200, 200}, cer::red);
}
```

For a detailed introduction into drawing sprites, please read [Drawing Sprites](../graphics/drawing-sprites.md).

## Caching

Loaded assets are cached, which means that when you load a specific asset multiple times,
it always refers to the same object in memory.

Consider this example:

```cpp
auto image1 = cer::Image{"MyImage.png"};
auto image2 = cer::Image{"MyImage.png"};
```

In this case, both `image1` and `image2` refer to the same internal image object.
The asset is therefore guaranteed to be only loaded once.

All cerlib objects are automatically reference counted using :fontawesome-brands-youtube:{.youtube} [RAII](https://youtu.be/Rfu06XAhx90). Copying an object simply
increments the internal object's reference count. When an object goes out of scope (i.e. its destructor is called), the reference count is decremented.

This ensures that an object's memory, and therefore an assets memory, is automatically managed for you. You don't have to unload assets explicitly; just let them go out of scope.

In the example above, the `#!cpp "MyImage.png"` asset would have a reference count of 2. When both objects would go out of scope, the asset would automatically be unloaded.

This behavior is similar to that of [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr).

---

Related pages:

* [Drawing Sprites](../graphics/drawing-sprites.md)
* [Custom Assets](custom-assets.md)

---

API Reference:

* [Content](../api/Content/index.md)