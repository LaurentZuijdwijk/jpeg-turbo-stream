#ifndef IO
#define IO

#include <inttypes.h>
//#include <stddef.h>
typedef unsigned long size_t;

typedef struct {
  uint32_t used;
  uint32_t size;
  unsigned char *data;
} io_buffer_t;

int io_read (int fn(size_t size, unsigned char *buf));
int io_write (size_t size, unsigned char*);
int io_write_file_to_stdout(char* filename);

#endif
