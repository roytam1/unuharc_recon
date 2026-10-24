/* UHARC 0.4 table stream crypt, default password.
 *
 * Originals (UHARC04.c):
 *   sub_401920  S-box init from password (byte_440250, CRC sub_404D10
 *               over dword_440DCC; key schedule recomputes CRC32 over
 *               the evolving 256-byte S-box before every 8-byte chunk,
 *               32 chunks total; identical to 0.6 sub_40173A).
 *   sub_4019C0  fresh S-box per table (byte_440250 -> byte_440150,
 *               byte_440E3C = 0) when its arg is nonzero; the flag it
 *               sets is byte_430EDF.
 *   sub_4056F0  stream transform over byte_440150 / byte_440E3C
 *               (identical to 0.6 sub_40367A).
 *   sub_4057D0  table output writer: sub_4056F0 when byte_430EDF is
 *               set, then block write via sub_40D5A0.
 *   sub_405800  table input reader (memory image + CRC sub_404D10).
 *   sub_40C6E0  tables driver (method, input sub_405800, size, output
 *               sub_4057D0): called with 0 / 17 / 2 / 7 for
 *               tab0..tab3, each preceded by sub_4019C0(1); default
 *               password S-box via sub_401920 when the flags byte has
 *               bit 2 clear.
 *   sub_404D10  CRC32 helper (IEEE, init FFFFFFFF).
 *
 * Cipher and key schedule are byte-identical to the 0.6 module; only
 * the symbol/buffer names differ. Only malloc/free/string.h are used;
 * no globals.
 */
#include <stdlib.h>
#include <string.h>
#include "uhcrypt04.h"

struct uhcrypt04 {
    unsigned char s[256];
    unsigned char pos;
    unsigned char mix;
};

static void uh04_crc_tab(unsigned long tab[256])
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

static unsigned long uh04_crc32(unsigned long tab[256], unsigned long crc,
    const unsigned char *d, unsigned n)
{
    unsigned i;
    for (i = 0; i < n; i++)
        crc = tab[(unsigned char)(crc ^ d[i])] ^ (crc >> 8);
    return crc & 0xFFFFFFFFUL;
}

uhcrypt04_t *uhcrypt04_init(void)
{
    const char pwd[] = "UHARC Default Password";
    uhcrypt04_t *c;
    unsigned char sb[256];
    unsigned long tab[256];
    unsigned var4, ecx, off, k;
    unsigned long crc;
    c = (uhcrypt04_t *)malloc(sizeof(*c));
    if (!c) return 0;
    var4 = (unsigned)strlen(pwd);
    sb[0] = (unsigned char)var4;
    for (ecx = 0; ecx < 255; ecx++)
        sb[1 + ecx] = (unsigned char)pwd[ecx % var4];
    uh04_crc_tab(tab);
    for (off = 0; off < 256; off += 8) {
        crc = uh04_crc32(tab, 0xFFFFFFFFUL, sb, 256);
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

void uhcrypt04_update(uhcrypt04_t *c, unsigned char *data, unsigned len)
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

void uhcrypt04_free(uhcrypt04_t *c)
{
    free(c);
}
