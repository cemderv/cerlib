# Transformations

Sometimes it's necessary to transform a specific group of 2D objects (or even all of them) without manually modifying their positions, rotations and sizes.

A transformation matrix allows you to do just that. It encompasses a certain order of transformations that are applied to your objects relative to their default transformation.

Imagine drawing a 2D scene and wanting to scale the entirety of it by a specific factor, e.g. `#!cpp 1.5`. You could draw all objects of that scene by offset their positions and scaling their sizes accordingly, for each [`draw_sprite`](../api/Graphics/index.md#draw_sprite) call.

Or you could construct a matrix using [`cer::scale`](../api/Math/index.md#scale)`#!cpp ({1.5f, 1.5f})` and apply that to all of them. We can apply a transformation using the [`cer::set_transformation`](../api/Graphics/index.md#set_transformation) function, like this:

```cpp
void draw(const cer::Window& window) override
{
  cer::set_transformation(cer::scale({1.5f, 1.5f}));
  cer::draw_sprite(my_image, {200, 200});
  cer::draw_sprite(my_image, {400, 300});
  cer::draw_sprite(my_image, {600, 400});
}
```

This would scale everything by a factor of `#!cpp 1.5` across both the X- and Y-axis. As an example, `#!cpp {-1.5f, 2.0f}` would mirror the objects along the Y-axis, and also scale them by a factor of `#!cpp 2.0` along the Y-axis. In other words, negative factors allow mirroring effects.

Another example would be if you wanted to first rotate the objects, then scale them, and finally offset their positions by a specific amount. This is done by multiplying such matrices in a specific order:

```cpp
auto transform = cer::rotate(cer::radians(45.0f)) // First, rotate by 45 degrees
               * cer::scale({1.5f, 3.5f})         // ... then scale by factor {1.5, 3.5}
               * cer::translate({100, -100})      // ... finally move by offset {100, -100}
               
// Apply to all subsequent drawings
cer::set_transformation(transform);
```

Transformation matrices are often used to implement 2D cameras, since they typically require movement ([`cer::translate`](../api/Math/index.md#translate)) as well as a zoom ([`cer::scale`](../api/Math/index.md#scale)). A camera’s transformation is therefore a combination of both matrices.

As with all states, the transformation is remembered by cerlib until it’s changed again. To restore the default transformation, which is an identity matrix, simply set a default-constructed matrix:

```cpp
cer::set_transformation({});
```

If you wish to set a transformation temporarily and restore the previously set transformation afterwards, you can obtain the active transformation before setting yours.

Like so:

```cpp
const auto previous_transformation = cer::current_transformation();
cer::set_transformation(my_transformation);
 
// Draw your objects
// ...
 
// Restore the previous transformation
cer::set_transformation(previousTransformation);
```

---

Related pages:

* [Drawing Sprites](drawing-sprites.md)
* [Drawing Text](drawing-text.md)
