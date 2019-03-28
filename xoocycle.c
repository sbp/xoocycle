/* Xoodyak code written by sbp */

#include "xoocycle.h"

#define XOOCYCLE_HASH 16
#define XOOCYCLE_KEYIN 44
#define XOOCYCLE_KEYOUT 24
#define XOOCYCLE_RATCHET 16

#define DOMAIN_DEFAULT 0x00
#define DOMAIN_KEY 0x02
#define DOMAIN_ABSORB 0x03
#define DOMAIN_RATCHET 0x10
#define DOMAIN_SQUEEZE 0x40
#define DOMAIN_CRYPT 0x80

#define PERMUTATION xoodoo8
#define ROUNDS 12
#define SPONGE_END ((XOOCYCLE_SPONGE) - 1)

#define ROTR(v, n) (((v) >> (n)) | ((v) << (32 - (n))))
#define SWAP(s, u, v) t = (s)[u], (s)[u] = (s)[v], (s)[v] = t
#define MIN(a, b) (size)((a) < (b) ? (a) : (b))

#define ZERO(i, data, len)                                 \
  FOR((i), (len)) {                                        \
    (data)[i] = 0;                                         \
  }

#define SPLIT(n, len, start, step)                         \
  for ((start) = 0, (step) = MIN((n), (len) - (start));    \
       ((start) == 0) || ((start) < (len));                \
       (start) += (n), (step) = MIN((n), (len) - (start)))

typedef enum { false, true } boolean;

u8 xoocycle_empty[1] = {0};

static void xoodoo32(u32 *s32, u32 rounds)
{
  u32 e[4], a, b, c, t, r, i;
  u32 k[24] = {120, 52, 576, 160, 384, 22, 112, 60, 832, 144, 320, 24,
               88, 56, 960, 208, 288, 20, 96, 44, 896, 240, 416, 18};

  FOR(r, rounds) {
    FOR(i, 4) {
      e[i] = ROTR(s32[i] ^ s32[i + 4] ^ s32[i + 8], 18),
      e[i] ^= ROTR(e[i], 9);
    }
    FOR(i, 12) {
      s32[i] ^= e[(i - 1) & 3];
    }
    SWAP(s32, 7, 4);
    SWAP(s32, 7, 5);
    SWAP(s32, 7, 6);
    s32[0] ^= k[(24 - rounds) + r];
    FOR(i, 4) {
      a = s32[i], b = s32[i + 4], c = ROTR(s32[i + 8], 21),
      s32[i + 8] = ROTR((b & ~a) ^ c, 24),
      s32[i + 4] = ROTR((a & ~c) ^ b, 31),
      s32[i] ^= c & ~b;
    }
    SWAP(s32, 8, 10);
    SWAP(s32, 9, 11);
  }
}

void xoodoo8(u8 *s8, u32 rounds)
{
  u32 *s32 = (u32 *)s8;

  xoodoo32(s32, rounds);
}

static void up(xoocycle *cyc, u8 *output, size len, u8 domain)
{
  size i = 0;

  cyc->phase = xoocycle_up;
  if (cyc->mode != xoocycle_hash) {
    cyc->sponge[SPONGE_END] ^= domain;
  }
  PERMUTATION(cyc->sponge, ROUNDS);
  FOR(i, len) {
    output[i] = cyc->sponge[i];
  }
}

static void down(xoocycle *cyc, const u8 *input, size len, u8 domain)
{
  size i = 0;

  cyc->phase = xoocycle_down;
  FOR(i, len) {
    cyc->sponge[i] ^= input[i];
  }
  cyc->sponge[len] ^= 0x01;
  if (cyc->mode == xoocycle_hash) {
    cyc->sponge[SPONGE_END] ^= (domain & 0x01);
  } else {
    cyc->sponge[SPONGE_END] ^= domain;
  }
}

static void squeeze_any(xoocycle *cyc, u8 *output, size len, u8 domain)
{
  size first_len = MIN(len, cyc->squeeze);
  size subsequent_len = 0;
  size aggregate_len = 0;

  up(cyc, &output[aggregate_len], first_len, domain);
  aggregate_len += first_len;
  while (aggregate_len < len) {
    down(cyc, (const u8 *)"", 0, DOMAIN_DEFAULT);
    subsequent_len = MIN(len - aggregate_len, cyc->squeeze);
    up(cyc, &output[aggregate_len], subsequent_len, DOMAIN_DEFAULT);
    aggregate_len += subsequent_len;
  }
}

static void crypto(xoocycle *cyc, u8 *io, size len, boolean decrypt)
{
  size start = 0;
  size step = 0;
  size i = 0;
  u8 up_output[XOOCYCLE_KEYOUT];
  u8 down_input[XOOCYCLE_KEYOUT];

  ZERO(i, up_output, XOOCYCLE_KEYOUT);
  ZERO(i, down_input, XOOCYCLE_KEYOUT);
  SPLIT(XOOCYCLE_KEYOUT, len, start, step) {
    if (start == 0) {
      up(cyc, &up_output[0], step, DOMAIN_CRYPT);
    } else {
      up(cyc, &up_output[0], step, DOMAIN_DEFAULT);
    }
    if (decrypt) {
      FOR(i, step) {
        io[start + i] ^= up_output[i];
        down_input[i] = io[start + i];
      }
    } else {
      FOR(i, step) {
        down_input[i] = io[start + i];
        io[start + i] ^= up_output[i];
      }
    }
    down(cyc, down_input, step, DOMAIN_DEFAULT);
  }
}

static void absorb_any(xoocycle *cyc, const u8 *input, size len,
                       size r, u8 domain)
{
  size start = 0;
  size step = 0;

  SPLIT(r, len, start, step) {
    if (cyc->phase != xoocycle_up) {
      up(cyc, xoocycle_empty, 0, DOMAIN_DEFAULT);
    }
    if (start == 0) {
      down(cyc, &input[start], step, domain);
    } else {
      down(cyc, &input[start], step, DOMAIN_DEFAULT);
    }
  }
}

static void absorb_key(xoocycle *cyc, const u8 *k, size k_len,
                       const u8 *id, size id_len,
                       const u8 *counter, size counter_len)
{
  size i = 0;
  u8 kie[XOOCYCLE_KEYIN];
  size kie_len = k_len + id_len + 1;

  cyc->mode = xoocycle_keyed;
  cyc->absorb = XOOCYCLE_KEYIN;
  cyc->squeeze = XOOCYCLE_KEYOUT;
  FOR(i, k_len) {
    kie[i] = k[i];
  }
  FOR(i, id_len) {
    kie[i + k_len] = id[i];
  }
  kie[k_len + id_len] = (u8)(id_len & 0xff);
  absorb_any(cyc, &kie[0], kie_len, cyc->absorb, DOMAIN_KEY);
  if (counter_len > 0) {
    absorb_any(cyc, counter, counter_len, 1, DOMAIN_DEFAULT);
  }
}

extern void xoocycle_cyclist(xoocycle *cyc, const u8 *k, size k_len,
                             const u8 *id, size id_len,
                             const u8 *counter, size counter_len)
{
  size i = 0;

  cyc->phase = xoocycle_up;
  FOR(i, XOOCYCLE_SPONGE) {
    cyc->sponge[i] = 0;
  }
  cyc->mode = xoocycle_hash;
  cyc->absorb = XOOCYCLE_HASH;
  cyc->squeeze = XOOCYCLE_HASH;
  if (k_len > 0) {
    absorb_key(cyc, k, k_len, id, id_len, counter, counter_len);
  }
}

extern void xoocycle_absorb(xoocycle *cyc, const u8 *input, size len)
{
  absorb_any(cyc, input, len, cyc->absorb, DOMAIN_ABSORB);
}

extern void xoocycle_encrypt(xoocycle *cyc, u8 *plain, size len)
{
  if (cyc->mode != xoocycle_keyed) {
    return;
  }
  crypto(cyc, plain, len, false);
}

extern void xoocycle_decrypt(xoocycle *cyc, u8 *cipher, size len)
{
  if (cyc->mode != xoocycle_keyed) {
    return;
  }
  crypto(cyc, cipher, len, true);
}

extern void xoocycle_squeeze(xoocycle *cyc, u8 *output, size len)
{
  squeeze_any(cyc, output, len, DOMAIN_SQUEEZE);
}

extern void xoocycle_squeeze_key(xoocycle *cyc, u8 *output, size len)
{
  if (cyc->mode != xoocycle_keyed) {
    return;
  }
  squeeze_any(cyc, output, len, DOMAIN_KEY);
}

extern void xoocycle_ratchet(xoocycle *cyc)
{
  size i;
  u8 io[XOOCYCLE_RATCHET];

  ZERO(i, io, XOOCYCLE_RATCHET);
  if (cyc->mode != xoocycle_keyed) {
    return;
  }
  squeeze_any(cyc, io, XOOCYCLE_RATCHET, DOMAIN_RATCHET);
  absorb_any(cyc, io, XOOCYCLE_RATCHET, cyc->absorb, DOMAIN_DEFAULT);
}

extern void xoocycle_erase_u8(void *delenda, size len)
{
  volatile u8 *volatile ephemeral = (volatile u8 *volatile)delenda;
  size i = (size)0U;

  while (i < len) {
    ephemeral[i++] = 0U;
  }
}

extern void xoocycle_erase(xoocycle *cyc)
{
  xoocycle_erase_u8(cyc->sponge, XOOCYCLE_SPONGE);
}
