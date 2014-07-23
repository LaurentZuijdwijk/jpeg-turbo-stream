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
static char* formats[] = {"NOOP", "INFO", "JPEG", "GIF", "PNG", "BMP", "PDF"};
enum {NOOP=0, INFO=1, JPEG=2, GIF=3, PNG=4, BMP=5, PDF=6};
enum {CONTAIN=0x01, FIXED=0x02, COVER=0x04, MULTIPAGE=0x08};

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
  uint32_t page_start;
  uint32_t page_end;
  uint32_t format;
} convert_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t format;
  uint32_t pages;
} convert_info_t;

int convert_format (MagickWand *input, convert_t *opts) {
  if (opts->format < JPEG || opts->format > BMP) return MagickPass;
  return MagickSetImageFormat(input, formats[opts->format]);
}

int convert_density (MagickWand *input, convert_t* opts) {
  if (!opts->density) return MagickPass;
  return MagickSetResolution(input, opts->density, opts->density);
}

int convert_rotate (MagickWand *input, convert_t *opts) {
  if (!opts->rotate_degrees) return MagickPass;
  if (opts->rotate_degrees == 360) opts->rotate_degrees = 0;

  char* exif_orientation = MagickGetImageAttribute(input,  "EXIF:Orientation");

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
    MagickProfileImage(input, "exif", NULL, 0);
  }

  return opts->rotate_degrees ? MagickRotateImage(input, default_background, opts->rotate_degrees) : MagickPass;
}

int convert_crop (MagickWand *input, convert_t *opts) {
  unsigned long crop_width = opts->crop_width;
  unsigned long crop_height = opts->crop_height;

  if (!crop_width && !opts->crop_x && !crop_height && !opts->crop_y) return MagickPass;
  if (!crop_width) crop_width = MagickGetImageWidth(input);
  if (!crop_height) crop_height = MagickGetImageHeight(input);

  return MagickCropImage(input, crop_width, crop_height, opts->crop_x, opts->crop_y);
}

int convert_scale (MagickWand *input, convert_t *opts) {
  unsigned long scale_width = opts->scale_width;
  unsigned long scale_height = opts->scale_height;

  if (!scale_height && !scale_width) return MagickPass;

  unsigned long wid = MagickGetImageWidth(input);
  unsigned long hei = MagickGetImageHeight(input);
  double ratio = (double)wid / (double)hei;

  unsigned long new_wid = 0;
  unsigned long new_hei = 0;

  if (opts->scale_options & FIXED) {
    new_wid = scale_width;
    new_hei = scale_height;
  } else {
    new_wid = MIN(scale_width, wid);
    new_hei = MIN(scale_height, hei);

    if (new_hei && new_wid) {
      double new_ratio = (double)new_wid / (double)new_hei;

      if (opts->scale_options & COVER) {
        if (new_ratio > ratio) new_hei = 0;
        else new_wid = 0;
      }
      if (opts->scale_options & CONTAIN) {
        if (new_ratio > ratio) new_wid = 0;
        else new_hei = 0;
      }
    }
  }

  if (!new_wid) new_wid = ratio * (double)new_hei;
  if (!new_hei) new_hei = (double)new_wid / ratio;

  return MagickScaleImage(input, new_wid, new_hei);
}

int convert_remove_range(MagickWand *input, uint32_t start, uint32_t end) {
  MagickSetImageIndex(input, start);
  while (start++ < end) {
    if (MagickRemoveImage(input) != MagickPass) return MagickFail;
  }
  return MagickPass;
}

int convert_adjoin (MagickWand *input, MagickWand **output, convert_t *opts) {
  unsigned long pages = MagickGetNumberImages(input);
  uint32_t start = opts->page_start;
  uint32_t end = opts->page_end;

  if (pages < 2 || opts->format == INFO) return MagickPass;

  if (end && end < pages) {
    if (convert_remove_range(input, end, pages) != MagickPass) return MagickFail;
    pages = end;
  }

  if (start && pages) {
    start = MIN(start-1, pages);
    if (convert_remove_range(input, 0, start) != MagickPass) return MagickFail;
    pages -= start;
  }

  if (!pages) return MagickFail;

  MagickResetIterator(input);
  *output = MagickAppendImages(input, 1);
  if (*output == NULL) return MagickFail;

  if (opts->scale_height && !(opts->scale_options & MULTIPAGE)) opts->scale_height *= pages;

  return MagickPass;
}

void to_convert_info (MagickWand *input, convert_info_t *res) {
  res->width = MagickGetImageWidth(input);
  res->height = MagickGetImageHeight(input);
  res->format = NOOP;

  res->pages = MagickGetNumberImages(input);

  char *fmt = MagickGetImageFormat(input);
  int i;

  for (i = JPEG; i <= PDF; i++) {
    if (strcmp(fmt, formats[i])) continue;
    res->format = i;
    break;
  }
}

int convert (MagickWand *input, MagickWand **output, convert_t *opts, unsigned char* data, size_t size) {
  if (convert_density(input, opts) != MagickPass) return -7;
  if (MagickReadImageBlob(input, data, size) != MagickPass) return -2;
  if (convert_adjoin(input, output, opts) != MagickPass) return -8;

  input = *output;

  if (convert_format(input, opts) != MagickPass) return -3;
  if (convert_scale(input, opts) != MagickPass)  return -4;
  if (convert_rotate(input, opts) != MagickPass) return -5;
  if (convert_crop(input, opts) != MagickPass)   return -6;

  return 0;
}

void destroy (MagickWand *input, MagickWand *output) {
  if (input != NULL) DestroyMagickWand(input);
  if (output != NULL && input != output) DestroyMagickWand(output);
}

int parse (size_t size, unsigned char *data) {
  MagickWand *input = NewMagickWand();
  MagickWand *output = input;

  convert_t *opts = (convert_t*) data;
  convert_info_t info_data;

  size -= sizeof(convert_t);
  data += sizeof(convert_t);

  int status = convert(input, &output, opts, data, size);

  if (status < 0) {
    destroy(input, output);
    return status;
  }

  if (opts->format == INFO) {
    to_convert_info(output, &info_data);
    data = (unsigned char*) &info_data;
    size = sizeof(convert_info_t);
  } else {
    data = MagickWriteImageBlob(output, &size);
  }

  status = io_write(size, data);
  destroy(input, output);
  return status;
}

int main (int argc, char *argv[]) {
  InitializeMagick(*argv);
  default_background = NewPixelWand();
  return io_read(parse);
}
