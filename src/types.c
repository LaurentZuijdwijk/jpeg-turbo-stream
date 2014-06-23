#include <wand/magick_wand.h>
#include "types.h"

int on_echo (MagickWand *wand, void *o) {
  return MagickPass;
}

int on_scale (MagickWand *wand, void *o) {
  scale_t *opts = (scale_t*) o;
  return MagickScaleImage(wand, opts->width, opts->height);
}

type_fn *type_to_function (int type) {
  switch (type) {
    case 0:  return &on_echo;
    case 1:  return &on_scale;
    default: return NULL;
  }
}
