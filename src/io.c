#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "io.h"

static int read_stdin (buffer_t *b, int len) {
  if (b->size < len) {
    free(b->data);
    b->size = len;
    b->data = malloc(len);
  }

  b->used = 0;
  int r;

  while ((r = read(0, b->data + b->used, len)) > 0) {
    b->used += r;
    len -= r;
    if (!len) return b->used;
  }

  return r;
}

static int to_int (buffer_t *b) {
  uint32_t *p = (uint32_t*) b->data;
  return *p;
}

int io_read (int fn(int size, unsigned char *buf)) {
  buffer_t s = {
    .size = 1024,
    .data = malloc(1024),
    .used = 0
  };

  int r;

  while (1) {
    r = read_stdin(&s, 4);
    if (r <= 0) return r;

    r = read_stdin(&s, to_int(&s));
    if (r <= 0) return r;

    r = fn(s.used, s.data);
    if (r < 0) return r;
  }

  return 0;
}

int io_write (int size, unsigned char *buf) {
  int r;

  r = write(1, &size, sizeof(uint32_t));
  if (r < 0) return r;

  r = write(1, buf, size);
  if (r < 0) return r;

  return size + r;
}