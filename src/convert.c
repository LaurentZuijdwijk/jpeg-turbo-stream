#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <wand/magick_wand.h>
#include "io.h"
#include "types.h"

static MagickWand *wand;

typedef struct {
  uint32_t type;
  uint32_t opts_offset;
  uint32_t data_offset;
} header_t;

int parse (int size, unsigned char *data) {
  header_t *header = (header_t*) data;
  size_t result_size;
  unsigned char *result_data;

  void *opts = data + header->opts_offset;
  data += header->data_offset;
  size -= header->data_offset;

  if (MagickReadImageBlob(wand, data, size) != MagickPass) return -10;

  type_fn *fn = type_to_function(header->type);

  if (fn == NULL) return -11;
  if (fn(wand, opts) != MagickPass) return -12;

  result_data = MagickWriteImageBlob(wand, &result_size);
  return io_write(result_size, result_data);
}

int main (int argc, char *argv[]) {
  InitializeMagick(*argv);
  wand = NewMagickWand();
  return io_read(parse);
}