#include <stdio.h>

extern "C" int main(int argc, char **argv) {
  int i;
  printf("argc = %d\n", argc);
  for (i = 0; i < argc; ++i)
    printf("argv[%d] = %s\n", i, argv[i]);
  puts("hello");
  fprintf(stderr, "hello\n");
  return 0;
}

