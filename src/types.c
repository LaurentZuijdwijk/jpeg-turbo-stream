#include <wand/magick_wand.h>
#include "types.h"
#include "debug.h"

static PixelWand *background = NULL;

int type_echo (MagickWand *wand, void *o) {
  return MagickPass;
}

int type_rotate (MagickWand *wand, void *o) {
  rotate_t *opts = (rotate_t*) o;

  if (background == NULL) background = NewPixelWand();

  char* exif_orientation = MagickGetImageAttribute(wand,  "EXIF:Orientation");
  if (opts->auto_rotate && exif_orientation != NULL) {
    int orientation = atoi(exif_orientation);

    switch (orientation) {
      case 3:
      opts->degrees = 180;
      break;

      case 6:
      opts->degrees = 90;
      break;

      case 8:
      opts->degrees = 270;
      break;
    }

    // just clear all exif data
    MagickProfileImage(wand, "exif", NULL, 0);
  }

  return MagickRotateImage(wand, background, opts->degrees);
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
    case 2:  return &type_rotate;
    default: return NULL;
  }
}
