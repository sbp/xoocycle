/* Xoodyak code written by sbp */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xoocycle.h"

#define IO 4096
#define HASH 32

static void print8(u8 *bytes, size len)
{
  size i;

  FOR(i, len) {
    printf("%02x", (unsigned int)bytes[i]);
  }
  printf("\n");
}

int main(void)
{
  u8 io[IO];
  int len = 0;
  size i = 0;
  xoocycle cyc;
  
  FOR(i, IO) {
    io[i] = 0;
  }
  xoocycle_cyclist(&cyc, xoocycle_empty, 0, xoocycle_empty, 0,
                   xoocycle_empty, 0);
  while (1) {
    len = read(STDIN_FILENO, io, IO);
    if (len < 0) {
      fprintf(stderr, "error\n");
      exit(EXIT_FAILURE);
    }
    if (len == 0) {
      break;
    }
    xoocycle_absorb(&cyc, io, len);
  }
  xoocycle_squeeze(&cyc, io, HASH);
  print8(io, HASH);
  xoocycle_erase(&cyc);
  return 0;
}
