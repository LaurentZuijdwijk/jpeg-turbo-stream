#ifndef TYPES
#define TYPES

#include <wand/magick_wand.h>
#include <stdint.h>

typedef struct {
  uint32_t width;
  uint32_t height;
} scale_t;

typedef int type_fn(MagickWand *wand, void *opts);

type_fn* type_to_function (int type);

#endif