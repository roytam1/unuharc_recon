/* UHARC 0.6 table stream crypt, default password.
 *
 * Originals (UNUHARC06.c / UNUHARC06.lst):
 *   sub_40173A  S-box init from password (byte_433250, CRC sub_403035
 *               over dword_433DD4 with table dword_421EB3; key schedule
 *               loc_401788 recomputes CRC32 over the evolving 256-byte
 *               S-box before every 8-byte chunk, 32 chunks total).
 *   sub_4017BE  fresh S-box per table (byte_433250 -> byte_433150,
 *               byte_433E38 = 0) when its arg is nonzero; the flag it
 *               sets is byte_422589.
 *   sub_40367A  stream transform over byte_433150 / byte_433E38.
 *   sub_4036E2  table input reader: file bytes via sub_407AB0, then
 *               sub_40367A when byte_422589 is set.
 *   sub_403840  tables driver: with default password when the flags
 *               byte has bit 2 clear; sub_4017BE(1) before every table;
 *               tab1/tab2 go through sub_4036E2 as decoder input.
 *   sub_403035  CRC32 helper (IEEE, init FFFFFFFF).
 *
 * This module implements init (sub_40173A with "UHARC Default
 * Password") + per-table fresh state (sub_4017BE(1)) + transform
 * (sub_40367A). Only malloc/free/string.h are used; no globals.
 */
#include <stdlib.h>
#include <string.h>
#include "uhcrypt06.h"

struct uhcrypt06 {
    unsigned char s[256];
    unsigned char pos;
    unsigned char mix;
};

static void uh06_crc_tab(unsigned long tab[256])
{
    unsigned i, j;
    unsigned long c;
    for (i = 0; i < 256; i++) {
        c = i;
        for (j = 0; j < 8; j++) {
            if (c & 1UL) c = 0xEDB88320UL ^ (c >> 1);
            else c >>= 1;
        }
        tab[i] = c;
    }
}

static unsigned long uh06_crc32(unsigned long tab[256], unsigned long crc,
    const unsigned char *d, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++)
        crc = tab[(unsigned char)(crc ^ d[i])] ^ (crc >> 8);
    return crc & 0xFFFFFFFFUL;
}

uhcrypt06_t *uhcrypt06_init(void)
{
    const char pwd[] = "UHARC Default Password";
    uhcrypt06_t *c;
    unsigned char sb[256];
    unsigned long tab[256];
    unsigned var4, ecx, off, k;
    unsigned long crc;
    c = (uhcrypt06_t *)malloc(sizeof(*c));
    if (!c) return 0;
    var4 = (unsigned)strlen(pwd);
    sb[0] = (unsigned char)var4;
    for (ecx = 0; ecx < 255; ecx++)
        sb[1 + ecx] = (unsigned char)pwd[ecx % var4];
    uh06_crc_tab(tab);
    for (off = 0; off < 256; off += 8) {
        crc = uh06_crc32(tab, 0xFFFFFFFFUL, sb, 256);
        for (k = 0; k < 4; k++) {
            sb[off + k] ^= (unsigned char)(crc >> (8 * k));
            sb[off + 4 + k] ^= (unsigned char)((~crc) >> (8 * k));
        }
    }
    memcpy(c->s, sb, 256);
    memset(sb, 0, sizeof(sb));
    c->pos = 0;
    c->mix = (unsigned char)(0 - 2);
    return c;
}

void uhcrypt06_update(uhcrypt06_t *c, unsigned char *data, unsigned len)
{
    unsigned i;
    if (!c) return;
    for (i = 0; i < len; i++) {
        unsigned char a, b, dh;
        a = c->s[c->pos];
        c->pos = (unsigned char)(c->pos + 1);
        b = c->s[c->pos];
        c->mix = (unsigned char)(c->mix + 1);
        dh = (unsigned char)(b + a);
        c->s[c->pos] = dh;
        data[i] ^= dh;
        c->s[c->mix] = (unsigned char)(c->s[c->mix] - b);
    }
}

void uhcrypt06_free(uhcrypt06_t *c)
{
    free(c);
}
