#ifndef IO
#define IO

typedef struct {
  int used;
  int size;
  unsigned char *data;
} buffer_t;

int io_read (int fn(int size, unsigned char *buf));
int io_write (int size, unsigned char*);

#endif