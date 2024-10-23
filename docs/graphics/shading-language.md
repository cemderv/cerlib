# Shading Language

cerlib uses its own platform-agnostic shading language.

It includes a hand-written, small and efficient compiler that translates its shaders to native shading languages such as GLSL, HLSL and MSL.

The language is designed as a C-like language with simple constructs. Its goal is to provide an easy to understand shading language tailored to cerlib’s domain, namely sprite shading.

The advantage of having a custom shading language is the ability to closely match it to what the library is capable of. Conversely, the library can optimize how shader data is stored and sent to the GPU, because it understands the language’s behavior and restrictions.

Users of GLSL, HLSL or MSL should feel familiar with the language.

## Basic Syntax

### Functions

Similar to C++, a function is defined in the form of:

```cpp
Vector3 multiply_add(Vector3 first, Vector3 second, Vector3 third)
{
  return first * second + third;
}
```

Some rules for functions:

* Every function must return a value; there is no `void` type 
    * A function is allowed at most one `return` statement, which must be the last statement in its body
* Function parameters are immutable
* Function overloading is not allowed
* Every code block must be surrounded by `{` and `}`, even if it contains a single statement

### Comments

Comments start with `//`. Multiline comments in the form of `/* ... */` are not supported.

### Variables

There are two ways to define a variable: mutable and immutable. Mutable variables are defined using the `var` keyword, while immutable variables are defined using the `const` keyword:

```cpp
var a = 1;
a = a * 2; // ok: value of a can be changed
a += 2;
 
const b = 2;
b = b * 2; // error: value of b can't be changed
b += 3;
```

In either case, every variable statement has to be initialized with an expression, from which its type is deduced. There is no explicit type declaration for variables.

!!! note
    The prefix `cer_` is a reserved prefix for built-in variables and may not be used as a prefix for identifiers.

### Data Structures

A data structure is defined using the `struct` keyword:

```cpp
// Definition:
struct LightingResult
{
  Vector3 diffuse;
  Vector3 specular;
  float intensity;
}
 
// Usage:
const result = LightingResult
{
  diffuse   = Vector3(1, 2, 3),  // initialize field 'diffuse'
  specular  = Vector3(4, 5, 6),  // initialize field 'specular'
  intensity = 7.0                // initialize field 'intensity'
};
```

When initializing a struct, **all or none** of its fields must be initialized. The following would therefore be not allowed:

```cpp
const result = LightingResult
{
  diffuse = Vector3(1, 2, 3)
}; // error: missing initializers for fields 'specular' and 'intensity'
```

Whereas this would be allowed:

```cpp
const result1 = LightingResult{};
result1.diffuse = Vector3(1, 2, 3); // error: 'result1' is immutable
 
var result2 = LightingResult{};
result2.diffuse = Vector3(1, 2, 3); // ok: result2 is mutable
```

### If Statements

Use `if` statements to conditionally execute a portion of code at runtime:

```cpp
Vector3 some_conditions(Vector3 v)
{
  var result = v;
  const len = length(v);
 
  if (len > 4.0)
  {
    result *= 10.0;
  }
  else if (len > 2.0)
  {
    result *= 5.0;
  }
  else
  {
    result = Vector3(0);
  }
 
  return result;
}
```

Ternary conditional operators are also supported:

```cpp
float max(float a, float b)
{
  return a > b ? a : b;
}
```

### Loops

Loops can be realized using a `for` statement. A for loop requires a name for the iterator variable and a range in the form of `<start> .. <end_exclusive>`:

```cpp
var sum = 0;
for i in 1 .. 4 // i will be 1, 2, 3
{
  sum += i;
}
// sum: 1+2+3 = 6
```

!!! note
    The iterator variable (in this case `i`) is immutable.

### Shader Functions

Shader functions are the main entry points in a shader and are always called `main`:

```cpp
Vector4 main()
{
  return Vector4(0);
}
```

There are special restrictions for shader functions. For example, a shader function must always return a value of type `Vector4`, which is the output pixel color.

### Using Shaders

The default way to use shaders is to load them using the single-string `Shader` constructor, i.e.:

```cpp
auto shader = cer::Shader{"MyShader.shd"}; // Loads a shader from the asset storage
```

In this case however we’ll look at how a shader can be constructed directly from a C++ string. It’s as simple as:

```cpp
auto shader = cer::Shader{my_shader_name, my_shader_code};
```

Where `my_shader_name` and `my_shader_code` are string object. The shader’s name is used to report it in compilation error messages. If the constructor did not throw an exception, the shader was compiled successfully and is ready for use.

### Parameters

A shader can declare parameters that are accessible to all functions within it, for example:

```cpp
Vector3 some_color;
float intensity = 1.0; // Assigning a default value
 
Vector3 some_function(Vector3 value)
{
  return value + some_color;
}
```

Parameter declarations may optionally assign a default value. If no default value is specified for a parameter, it receives a zero-value. Meaning that a `float` will be `0.0`, a `Vector2` will be `Vector2(0, 0)`, a `Matrix` will be all zeroes, etc.

Supported parameter types are:

* `bool`
* `int`, `uint`, `float`
* `Vector2`, `Vector3`, `Vector4`
* `Matrix`
* `Image`

!!! note
    The compiler will optimize any unused parameters away.

### Setting parameter values

To set parameters on shader objects, call the `Shader::set_value` method:

```cpp
my_shader.set_value("base_color", cer::Vector3{1, 0, 0});
my_shader.set_value("intensity", 2.0f);
```

The method has overloads for each parameter type. When attempting to set a value that is incompatible with the parameter type, an exception is thrown.

Conversely, shader parameter values can be obtained using the `Shader::*_value()` methods:

```cpp
std::optional<cer::Vector3> base_color = my_shader.vector3_value("base_color");
std::optional<float>        intensity  = my_shader.float_value("intensity");
```

The value of a parameter is always part of a shader object’s state. This means that shader parameters can be updated even when a shader is not actively used.

### Array parameters

It is possible to declare array parameters for scalar types using an array specifier:

```cpp
// Arrays must always have a known size at compile time.
Vector3[12] some_array_of_3d_vectors;
 
// Expressions may be used as an array size, but are required to be known at compile time.
const some_value = 4;
const some_constant = 12 * some_value;
 
float[some_constant + 2] some_array_of_floats;
```

Setting array parameter values are also modified using the `set_value()` method. Arrays are specified as `std::span` values:

```cpp
my_shader.set_value("some_floats", {{ 0.5f, 1.0f, 1.25f, 5.0f }});
```

## Shading Language Reference

### Syntax

The following table describes the syntax of the shading language.

| Construct          | Form                                                  | Example                              |
|--------------------|-------------------------------------------------------|--------------------------------------|
| Function parameter | `<type> <name>`                                       | `int a`                              |
| Function signature | `<type> <name> '(' <parameter> (',' <parameter)* ')'` | `int add(int a, int b)`              |
| Function body      | `'{' stmt* return_stmt '}'`                           | `{ return a + b; }`                  |
| Function           | `<signature> <body>`                                  | `float pow(int x) { return x * x; }` |
| Shader parameter   | `<type> <name>`                                       | `float some_parameter`               |
| Array type         | `<type>[<size>]`                                      | `Vector2[10]`                        |

### Types

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

### Struct fields

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

### Built-in variables

The following lists all variables that are always available within a shader.

| Variable       | Description                                        | Type    |
|----------------|----------------------------------------------------|---------|
| `sprite_image` | The image of the sprite that is drawn              | Image   |
| `sprite_color` | The color of the sprite that is drawn              | Vector4 |
| `sprite_uv`    | The texture coordinate of the sprite that is drawn | Vector2 |

### Functions

The following lists all available intrinsic functions.

Within this table the following names are defined as groups of types:

- `Vec`: Vector2 | Vector3 | Vector4
- `Fto4`: float | Vector2 | Vector3 | Vector4
- `FtoM`: float | Vector2 | Vector3 | Vector4 | Matrix

#### Function Table

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

### Constructors

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

