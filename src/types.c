#include <wand/magick_wand.h>
#include "types.h"

int type_echo (MagickWand *wand, void *o) {
  return MagickPass;
}

int type_scale (MagickWand *wand, void *o) {
  scale_t *opts = (scale_t*) o;

  unsigned long wid = MagickGetImageWidth(wand);
  unsigned long hei = MagickGetImageHeight(wand);
  double ratio = (double)hei / (double)wid;

  return MagickScaleImage(wand, opts->width, opts->height);
}

type_fn *type_to_function (int type) {
  switch (type) {
    case 0:  return &type_echo;
    case 1:  return &type_scale;
    default: return NULL;
  }
}
