#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include "io.h"

static int read_stdin (io_buffer_t *b, int len) {
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

static int to_int (io_buffer_t *b) {
  uint32_t *p = (uint32_t*) b->data;
  return *p;
}

int io_read (int fn(size_t size, unsigned char *buf)) {
  io_buffer_t s = {
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

int io_write (size_t size, unsigned char *buf) {
  int r;

  r = write(1, &size, sizeof(uint32_t));
  if (r < 0) return r;

  r = write(1, buf, size);
  if (r < 0) return r;

  return r + sizeof(uint32_t);
}

int io_write_file_to_stdout(char* filename) {
  int r;
  size_t fileLen;
  const int buflen = 1024;
  int dataread = 0;
  FILE* file = fopen(filename, "rb");
  if (!file)
  {
    fprintf(stderr, "Unable to open file");
    return -1;
  }
  
  //Get file length
  fseek(file, 0, SEEK_END);
  fileLen=ftell(file);
  fseek(file, 0, SEEK_SET);

  //Write file length to std out
  r = write(1, &fileLen, sizeof(uint32_t));
  if (r < 0) return r;

  //Write file to std out
  char buf[buflen];
  int datalen;
  while((datalen = fread(buf, 1, buflen, file)) > 0)
  {
    dataread += write(1, buf, datalen);
  }
  fclose(file);
  return dataread;
}
