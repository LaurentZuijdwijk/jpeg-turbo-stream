#include <wand/magick_wand.h>
#include "types.h"

int type_echo (MagickWand *wand, void *o) {
  return MagickPass;
}

int type_scale (MagickWand *wand, void *o) {
  scale_t *opts = (scale_t*) o;

  if (opts->keep_ratio == 0) return MagickScaleImage(wand, opts->width, opts->height);

  unsigned long wid = MagickGetImageWidth(wand);
  unsigned long hei = MagickGetImageHeight(wand);
  double ratio = (double)wid / (double)hei;

  uint32_t new_wid = MIN(opts->width, wid);
  uint32_t new_hei = MIN(opts->height, hei);

  if (ratio > 1) new_hei = (double)new_wid / ratio;
  else new_wid = (double)new_hei * ratio;

  return MagickScaleImage(wand, new_wid, new_hei);
}

type_fn *type_to_function (int type) {
  switch (type) {
    case 0:  return &type_echo;
    case 1:  return &type_scale;
    default: return NULL;
  }
}
