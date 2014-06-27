#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <wand/magick_wand.h>
#include <string.h>
#include "io.h"
#include "debug.h"

#define MIN(x,y) (x < y ? x : y)

static MagickWand *wand;
static PixelWand *background = NULL;
static char* formats[] = {"NOOP", "INFO", "JPEG", "GIF", "PNG", "BMP"};

typedef struct {
  uint32_t scale_options;
  uint32_t scale_width;
  uint32_t scale_height;
  uint32_t crop_x;
  uint32_t crop_y;
  uint32_t crop_width;
  uint32_t crop_height;
  uint32_t rotate_degrees;
  uint32_t format;
} convert_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t format;
} convert_info_t;

int convert_format (convert_t *opts) {
  if (opts->format < 2 || opts->format > 5) return MagickPass;
  return MagickSetImageFormat(wand, formats[opts->format]);
}

int convert_rotate (convert_t *opts) {
  if (!opts->rotate_degrees) return MagickPass;
  if (opts->rotate_degrees == 360) opts->rotate_degrees = 0;

  char* exif_orientation = MagickGetImageAttribute(wand,  "EXIF:Orientation");
  if (exif_orientation != NULL) {
    int orientation = atoi(exif_orientation);

    switch (orientation) {
      case 3:
      opts->rotate_degrees += 180;
      break;

      case 6:
      opts->rotate_degrees += 90;
      break;

      case 8:
      opts->rotate_degrees += 270;
      break;
    }

    // just clear all exif data
    MagickProfileImage(wand, "exif", NULL, 0);
  }

  return opts->rotate_degrees ? MagickRotateImage(wand, background, opts->rotate_degrees) : MagickPass;
}

int convert_crop (convert_t *opts) {
  unsigned long crop_width = opts->crop_width;
  unsigned long crop_height = opts->crop_height;

  if (!crop_width && !opts->crop_x && !crop_height && !opts->crop_y) return MagickPass;
  if (!crop_width) crop_width = MagickGetImageWidth(wand);
  if (!crop_height) crop_height = MagickGetImageHeight(wand);

  return MagickCropImage(wand, crop_width, crop_height, opts->crop_x, opts->crop_y);
}

int convert_scale (convert_t *opts) {
  unsigned long scale_width = opts->scale_width;
  unsigned long scale_height = opts->scale_height;

  if (!scale_height && !scale_width) return MagickPass;

  unsigned long wid = MagickGetImageWidth(wand);
  unsigned long hei = MagickGetImageHeight(wand);
  double ratio = (double)wid / (double)hei;

  unsigned long new_wid = MIN(scale_width, wid);
  unsigned long new_hei = MIN(scale_height, hei);

  if (opts->scale_options & 0x01 && new_hei && new_wid) {
    double new_ratio = (double)new_wid / (double)new_hei;

    if (opts->scale_options & 0x02) { // cover
      if (new_ratio > ratio) new_hei = 0;
      else new_wid = 0;
    } else { // contain
      if (new_ratio > ratio) new_wid = 0;
      else new_hei = 0;
    }
  }

  if (!new_wid) new_wid = ratio * (double)new_hei;
  if (!new_hei) new_hei = (double)new_wid / ratio;

  return MagickScaleImage(wand, new_wid, new_hei);
}

void to_convert_info (convert_info_t *res) {
  res->width = MagickGetImageWidth(wand);
  res->height = MagickGetImageHeight(wand);
  res->format = 0;

  char *fmt = MagickGetImageFormat(wand);
  int i;

  for (i = 0; i < 6; i++) {
    if (strcmp(fmt, formats[i])) continue;
    res->format = i;
    break;
  }
}

int parse (size_t size, unsigned char *data) {
  convert_t *opts = (convert_t*) data;
  convert_info_t info_data;

  size -= sizeof(convert_t);
  data += sizeof(convert_t);

  if (MagickReadImageBlob(wand, data, size) != MagickPass) return -2;

  if (convert_format(opts) != MagickPass) return -3;
  if (convert_scale(opts) != MagickPass)  return -4;
  if (convert_rotate(opts) != MagickPass) return -5;
  if (convert_crop(opts) != MagickPass)   return -6;

  if (opts->format == 1) {
    to_convert_info(&info_data);
    data = (unsigned char*) &info_data;
    size = sizeof(convert_info_t);
  } else {
    data = MagickWriteImageBlob(wand, &size);
  }

  return io_write(size, data);
}

int main (int argc, char *argv[]) {
  InitializeMagick(*argv);
  background = NewPixelWand();
  wand = NewMagickWand();
  return io_read(parse);
}
