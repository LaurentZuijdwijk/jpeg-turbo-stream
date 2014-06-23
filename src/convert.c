#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wand/magick_wand.h>
#include <stdint.h>
#include "io.h"

static MagickWand *wand;

typedef int converter(void *opts);

typedef struct {
  uint32_t type;
  uint32_t opts_offset;
  uint32_t data_offset;
} header_t;

typedef struct {
  uint32_t width;
  uint32_t height;
} scale_t;

int on_echo (void *o) {
  return MagickPass;
}

int on_scale (void *o) {
  scale_t *opts = (scale_t*) o;
  return MagickScaleImage(wand, opts->width, opts->height);
}

converter *to_converter (int type) {
  if (type == 0) return &on_echo;
  if (type == 1) return &on_scale;
  return NULL;
}

int parse (int size, unsigned char *data) {
  header_t *header = (header_t*) data;
  size_t result_size;
  unsigned char *result_data;

  void *opts = data + header->opts_offset;
  data += header->data_offset;
  size -= header->data_offset;

  if (MagickReadImageBlob(wand, data, size) != MagickPass) return -10;

  converter *convert = to_converter(header->type);

  if (convert == NULL) return -11;
  if (convert(opts) != MagickPass) return -12;

  result_data = MagickWriteImageBlob(wand, &result_size);
  return io_write(result_size, result_data);
}

int main (int argc, char *argv[]) {
  InitializeMagick(*argv);
  wand = NewMagickWand();
  return io_read(parse);
}