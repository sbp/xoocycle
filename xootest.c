/* Xoodyak code written by sbp */

#include <stdio.h>
#include "xoocycle.h"

#define CU8P const u8 *
#define PLAIN 5
#define TAG 16

void print8(u8 *bytes, size len)
{
  size i;

  FOR(i, len) {
    printf("%02x", bytes[i]);
  }
  printf("\n");
}

int main(void)
{
  u8 plain[PLAIN] = {'p', 'l', 'a', 'i', 'n'};
  u8 tag[TAG];
  xoocycle cyc;

  xoocycle_cyclist(&cyc, (CU8P)"key", 3, xoocycle_empty, 0,
                   xoocycle_empty, 0);
  xoocycle_absorb(&cyc, (CU8P)"nonce", 5);
  xoocycle_absorb(&cyc, (CU8P)"associated", 10);
  xoocycle_encrypt(&cyc, plain, PLAIN);
  print8(plain, PLAIN);
  xoocycle_squeeze(&cyc, tag, TAG);
  print8(tag, TAG);
  xoocycle_erase(&cyc);
  return 0;
}
