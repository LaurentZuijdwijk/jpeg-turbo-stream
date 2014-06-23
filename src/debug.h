#ifndef DEBUG
#define DEBUG(format, ...) fprintf(stderr, "debug: " format, ##__VA_ARGS__); fflush(stderr)
#endif