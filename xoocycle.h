/* Xoodyak code written by sbp */

#include <stddef.h>
#include <stdint.h>

#define XOOCYCLE_SPONGE 48

#define FOR(i, len) for ((i) = 0; (i) < (len); (i)++)

typedef uint8_t u8;
typedef uint32_t u32;
typedef size_t size;
typedef enum { xoocycle_up, xoocycle_down } xoocycle_phase;
typedef enum { xoocycle_hash, xoocycle_keyed } xoocycle_mode;
typedef struct
{
  u8 sponge[XOOCYCLE_SPONGE];
  xoocycle_phase phase;
  xoocycle_mode mode;
  size absorb;
  size squeeze;
} xoocycle;

extern u8 xoocycle_empty[1];

extern void xoocycle_cyclist(xoocycle *cyc, const u8 *k, size k_len,
                             const u8 *id, size id_len,
                             const u8 *counter, size counter_len);

extern void xoocycle_absorb(xoocycle *cyc, const u8 *input, size len);

extern void xoocycle_encrypt(xoocycle *cyc, u8 *plain, size len);

extern void xoocycle_decrypt(xoocycle *cyc, u8 *cipher, size len);

extern void xoocycle_squeeze(xoocycle *cyc, u8 *output, size len);

extern void xoocycle_squeeze_key(xoocycle *cyc, u8 *output, size len);

extern void xoocycle_ratchet(xoocycle *cyc);

extern void xoocycle_erase_u8(void *delenda, size len);

extern void xoocycle_erase(xoocycle *cyc);
