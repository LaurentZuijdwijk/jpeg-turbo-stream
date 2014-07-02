#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <wand/magick_wand.h>
#include <string.h>
#include "io.h"
#include "debug.h"

#define MIN(x,y) (x < y ? x : y)

static PixelWand *default_background = NULL;
static char* formats[] = {"NOOP", "INFO", "JPEG", "GIF", "PNG", "BMP"};
enum {NOOP=0, INFO=1, JPEG=2, GIF=3, PNG=4, BMP=5};

typedef struct {
  uint32_t scale_options;
  uint32_t scale_width;
  uint32_t scale_height;
  uint32_t crop_x;
  uint32_t crop_y;
  uint32_t crop_width;
  uint32_t crop_height;
  uint32_t rotate_degrees;
  uint32_t density;
  uint32_t format;
} convert_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t format;
} convert_info_t;

int convert_format (MagickWand *wand, convert_t *opts) {
  if (opts->format < JPEG || opts->format > BMP) return MagickPass;
  return MagickSetImageFormat(wand, formats[opts->format]);
}

int convert_density (MagickWand *wand, convert_t* opts) {
  if (!opts->density) return MagickPass;
  return MagickSetResolution(wand, opts->density, opts->density);
}

int convert_rotate (MagickWand *wand, convert_t *opts) {
  if (!opts->rotate_degrees) return MagickPass;
  if (opts->rotate_degrees == 360) opts->rotate_degrees = 0;

  char* exif_orientation = MagickGetImageAttribute(wand,  "EXIF:Orientation");

  if (exif_orientation != NULL) {
    switch (atoi(exif_orientation)) {
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

  return opts->rotate_degrees ? MagickRotateImage(wand, default_background, opts->rotate_degrees) : MagickPass;
}

int convert_crop (MagickWand *wand, convert_t *opts) {
  unsigned long crop_width = opts->crop_width;
  unsigned long crop_height = opts->crop_height;

  if (!crop_width && !opts->crop_x && !crop_height && !opts->crop_y) return MagickPass;
  if (!crop_width) crop_width = MagickGetImageWidth(wand);
  if (!crop_height) crop_height = MagickGetImageHeight(wand);

  return MagickCropImage(wand, crop_width, crop_height, opts->crop_x, opts->crop_y);
}

int convert_scale (MagickWand *wand, convert_t *opts) {
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

void to_convert_info (MagickWand *wand, convert_info_t *res) {
  res->width = MagickGetImageWidth(wand);
  res->height = MagickGetImageHeight(wand);
  res->format = NOOP;

  char *fmt = MagickGetImageFormat(wand);
  int i;

  for (i = JPEG; i <= BMP; i++) {
    if (strcmp(fmt, formats[i])) continue;
    res->format = i;
    break;
  }
}

int convert (MagickWand *wand, convert_t *opts, unsigned char* data, size_t size) {
  if (convert_density(wand, opts) != MagickPass) return -7;

  if (MagickReadImageBlob(wand, data, size) != MagickPass) return -2;

  if (convert_format(wand, opts) != MagickPass) return -3;
  if (convert_scale(wand, opts) != MagickPass)  return -4;
  if (convert_rotate(wand, opts) != MagickPass) return -5;
  if (convert_crop(wand, opts) != MagickPass)   return -6;

  return 0;
}

int parse (size_t size, unsigned char *data) {
  MagickWand *wand = NewMagickWand();

  convert_t *opts = (convert_t*) data;
  convert_info_t info_data;

  size -= sizeof(convert_t);
  data += sizeof(convert_t);

  int status = convert(wand, opts, data, size);

  if (status < 0) {
    DestroyMagickWand(wand);
    return status;
  }

  if (opts->format == INFO) {
    to_convert_info(wand, &info_data);
    data = (unsigned char*) &info_data;
    size = sizeof(convert_info_t);
  } else {
    data = MagickWriteImageBlob(wand, &size);
  }

  DestroyMagickWand(wand);
  return io_write(size, data);
}

int main (int argc, char *argv[]) {
  InitializeMagick(*argv);
  default_background = NewPixelWand();
  return io_read(parse);
}
