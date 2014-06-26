#ifndef DEBUG
#define DEBUG(format, ...) fprintf(stderr, "debug: " format "\n", ##__VA_ARGS__); fflush(stderr)
#endif