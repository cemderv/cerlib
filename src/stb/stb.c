#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Woverflow"
#endif

#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
#include <stb_truetype.h>

