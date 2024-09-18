# Shading Language

## Syntax

The following table describes the syntax of the shading language.

| Construct          | Form                                                  | Example                              |
|--------------------|-------------------------------------------------------|--------------------------------------|
| Function parameter | `<type> <name>`                                       | `int a`                              |
| Function signature | `<type> <name> '(' <parameter> (',' <parameter)* ')'` | `int add(int a, int b)`              |
| Function body      | `'{' stmt* return_stmt '}'`                           | `{ return a + b; }`                  |
| Function           | `<signature> <body>`                                  | `float pow(int x) { return x * x; }` |
| Shader parameter   | `<type> <name>`                                       | `float some_parameter`               |
| Array type         | `<type>[<size>]`                                      | `Vector2[10]`                        |

## Types

| Type    | Description                  | C++ equivalent | Can be array |
|---------|------------------------------|----------------|--------------|
| bool    | Boolean true / false value   | `int32_t`      | ✅            |
| int     | Signed 32-bit integer        | `int32_t`      | ✅            |
| uint    | Unsigned 32-bit integer      | `uint32_t`     | ✅            |
| float   | 32-bit floating point number | `float`        | ✅            |
| Vector2 | 2D floating point vector     | `cer::Vector2` | ✅            |
| Vector3 | 3D floating point vector     | `cer::Vector3` | ✅            |
| Vector4 | 4D floating point vector     | `cer::Vector4` | ✅            |
| Matrix  | 4×4 row-major matrix         | `cer::Matrix`  | ✅            |
| Image   | 2D texture                   | `cer::Image`   | ❌            |

## Struct members

=== "Vector2"

    | Name | Type    |
    |------|---------|
    | x    | float   |
    | y    | float   |
    | xx   | Vector2 |
    | yy   | Vector2 |

=== "Vector3"

    | Name | Type    |
    |------|---------|
    | x    | float   |
    | y    | float   |
    | z    | float   |
    | xx   | Vector2 |
    | yy   | Vector2 |
    | zz   | Vector2 |
    | xy   | Vector2 |
    | yz   | Vector2 |
    | zy   | Vector2 |
    | xz   | Vector2 |
    | xxx  | Vector3 |
    | yyy  | Vector3 |
    | zzz  | Vector3 |

=== "Vector4"

    | Name | Type    |
    |------|---------|
    | x    | float   |
    | y    | float   |
    | z    | float   |
    | w    | float   |
    | xy   | Vector2 |
    | xyz  | Vector3 |
    | xxxx | Vector4 |
    | yyyy | Vector4 |
    | zzzz | Vector4 |
    | wwww | Vector4 |

=== "Matrix"

    The matrix type currently has no members.

## Built-in variables

The following lists all variables that are always available within a shader.

| Variable       | Description                                        | Type    |
|----------------|----------------------------------------------------|---------|
| `sprite_image` | The image of the sprite that is drawn              | Image   |
| `sprite_color` | The color of the sprite that is drawn              | Vector4 |
| `sprite_uv`    | The texture coordinate of the sprite that is drawn | Vector2 |

## Functions

The following lists all available intrinsic functions.

Within this table the following names are defined as groups of types:

- `Vec`: Vector2 | Vector3 | Vector4
- `Fto4`: float | Vector2 | Vector3 | Vector4
- `FtoM`: float | Vector2 | Vector3 | Vector4 | Matrix

### Function Table

| Name           | Parameters → Return Type          |
|----------------|-----------------------------------|
| `abs`          | Fto4 → Fto4                       |
| `acos`         | Fto4 → Fto4                       |
| `all`          | FtoM → FtoM                       |
| `any`          | FtoM → FtoM                       |
| `asin`         | Fto4 → Fto4                       |
| `atan`         | Fto4 → Fto4                       |
| `atan2`        | Fto4 → Fto4                       |
| `ceil`         | FtoM → FtoM                       |
| `clamp`        | Fto4 → Fto4                       |
| `cos`          | Fto4 → Fto4                       |
| `degrees`      | Fto4 → Fto4                       |
| `distance`     | Fto4 → Vec                        |
| `dot`          | Vec → Vec                         |
| `exp`          | Vec → Fto4                        |
| `exp2`         | Fto4 → Fto4                       |
| `floor`        | Fto4 → Fto4                       |
| `fmod`         | Fto4 → Fto4                       |
| `frac`         | Fto4 → Fto4                       |
| `length`       | Vec → Vec                         |
| `lerp`         | Fto4 → Fto4                       |
| `log`          | Fto4 → Fto4                       |
| `log2`         | Fto4 → Fto4                       |
| `max`          | Fto4 → Fto4                       |
| `min`          | Fto4 → Fto4                       |
| `normalize`    | Vec → Vec                         |
| `pow`          | Fto4 → Fto4                       |
| `radians`      | Fto4 → Fto4                       |
| `round`        | Fto4 → Fto4                       |
| `sample`       | (Image, Vector2) → Vector4        |
| `sample_level` | (Image, Vector2, float) → Vector4 |
| `saturate`     | Fto4 → Fto4                       |
| `sign`         | Fto4 → Fto4                       |
| `sin`          | Fto4 → Fto4                       |
| `smoothstep`   | Fto4 → Fto4                       |
| `sqrt`         | Fto4 → Fto4                       |
| `tan`          | Fto4 → Fto4                       |
| `transpose`    | Matrix → Matrix                   |
| `trunc`        | Fto4 → Fto4                       |

## Constructors

The following lists all available type constructors.

| Type    | Parameters                         | Effect                         |
|---------|------------------------------------|--------------------------------|
| float   | int x                              | Cast x to float                |
| float   | uint x                             | Cast x to float                |
| int     | float x                            | Cast x to int                  |
| int     | uint x                             | Cast x to int                  |
| uint    | int x                              | Cast x to uint                 |
| uint    | float x                            | Cast x to uint                 |
| Vector2 | float x, float y                   | x=x, y=y                       |
| Vector2 | float xy                           | x=xy, y=xy                     |
| Vector3 | float x, float y, float z          | x=x, y=y, z=z                  |
| Vector3 | float xyz                          | x=xyz, y=xyz, z=xyz            |
| Vector4 | float x, float y, float z, float w | x=x, y=y, z=z, w=w             |
| Vector4 | Vector2 xy, Vector2 zw             | x=xy.x, y=xy.y, z=zw.x, w=zw.y |
| Vector4 | Vector2 xy, float z, float w       | x=xy.x, y=xy.y, z=z, w=w       |
| Vector4 | Vector3 xyz, float w               | x=xyz.x, y=xyz.y, z=xyz.z, w=w |
| Vector4 | float xyzw                         | x=xyzw, y=xyzw, z=xyzw, w=xyzw |

