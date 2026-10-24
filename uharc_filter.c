/* UHARC 0.3/0.4 executable (E8/E9) post-filter - C89.
 *
 * Inverse of the compressor's EXE filter, shared byte-for-byte by the 0.3
 * and 0.4 archivers (verified from uharc_u.lst sub_4090A0 and UHARC04.lst
 * sub_409940; the encoder pair is sub_407D44 / sub_4085A0).  It is NOT the
 * simple 0.2 filter in uha02.c: 0.3/0.4 also rewrite the MZ relocation
 * table (delta + 4-way transpose) and resolve E8/E9 operands through a
 * 128-entry move-to-front address table.
 *
 * Called with the whole file in one buffer.  The original applies this to
 * files of at least 0x400 bytes when header flags_filt bit 0 is set; the
 * caller performs that gate, and this function additionally applies the
 * original's internal magic gate (MZ/ZM/MQ, ELF, or "FBOV").
 *
 * No globals, no prints; `ver` is 3 or 4 (the algorithm is identical, the
 * parameter exists to document the caller and for future divergence).
 */
#include <stdlib.h>
#include <string.h>
#include "uharc_filter.h"

static unsigned rd16(const unsigned char *p)
{
    return (unsigned)(p[0] | (p[1] << 8));
}

static unsigned long rd32(const unsigned char *p)
{
    return (unsigned long)(p[0] | (p[1] << 8) | (p[2] << 16) |
                           ((unsigned long)p[3] << 24));
}

/* 32-bit little-endian value as a signed long (long is 32-bit here) */
static long rd32s(const unsigned char *p)
{
    unsigned long u = rd32(p);
    if (u & 0x80000000UL)
        return -(long)(0xFFFFFFFFUL - u) - 1;
    return (long)u;
}

static void wr16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)(v & 0xFFU);
    p[1] = (unsigned char)((v >> 8) & 0xFFU);
}

static void wr32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v & 0xFFUL);
    p[1] = (unsigned char)((v >> 8) & 0xFFUL);
    p[2] = (unsigned char)((v >> 16) & 0xFFUL);
    p[3] = (unsigned char)((v >> 24) & 0xFFUL);
}

void uharc_filter_exe(unsigned char *buf, unsigned long len, int ver)
{
    unsigned long end, esi, d7c, d90;
    unsigned long i;
    unsigned char *tmp;
    int exe, elf, fbov;
    unsigned tab[128];
    unsigned j;

    (void)ver;
    if (!buf || len < 0x400UL || len <= 15UL) return;

    end = len - 15UL;

    /* ---- gate (sub_4090A0: MZ/ZM/MQ word, ELF dword, or "FBOV") ---- */
    {
        unsigned w0 = rd16(buf);
        unsigned long d0 = rd32(buf);
        exe = (w0 == 0x5A4DU || w0 == 0x4D5AU || w0 == 0x514DU);
        elf = (d0 == 0x464C457FUL);
        fbov = (d0 == 0x564F4246UL);
    }
    if (!exe && !elf && !fbov) return;

    esi = 0;

    /* ELF: the original refuses e_type/entry bytes 18/19 == 3/0 mismatch */
    if (elf) {
        while (esi < 20UL && esi < end) {
            if ((esi == 18UL && buf[esi] != 3) ||
                (esi == 19UL && buf[esi] != 0))
                return;
            esi++;
        }
    }

    d7c = 0;
    d90 = 0;

    /* ---- MZ header scan: e_crlc (+6) and e_lfarlc (+0x18) ---- */
    if (exe) {
        while (exe && esi < 26UL && esi < end) {
            if (esi == 6UL) {
                d90 = buf[esi];
            } else if (esi == 7UL) {
                d90 += (unsigned long)buf[esi] << 8;
                if (d90 < 0x10UL || d90 > 0x2000UL) exe = 0;
            } else if (esi == 24UL) {
                d7c = buf[esi];
            } else if (esi == 25UL) {
                d7c += (unsigned long)buf[esi] << 8;
                d90 = d7c + 4UL * d90;
                if (d7c < 0x1CUL || d90 >= len) exe = 0;
            }
            esi++;
        }
    }

    /* ---- relocation table: de-transpose, inverse delta, keep verbatim ---- */
    if (exe && esi >= 26UL) {
        while (esi < d7c && esi < end) esi++;
        if (esi == d7c) {
            unsigned long rlen;
            unsigned long p = 0;
            unsigned long k;
            int r;

            /* advance over the stored (filtered) relocation bytes */
            while (esi < d90 && esi < end) esi++;
            if (esi == d90) {
                rlen = d90 - d7c;
                tmp = (unsigned char *)malloc((size_t)(rlen ? rlen : 1));
                if (!tmp) return;
                for (r = 0; r < 4; r++) {
                    for (k = (unsigned long)r; k < rlen; k += 4UL)
                        tmp[k] = buf[d7c + p++];
                }
                /* inverse delta: word[k] += word[k-2], k = 2..rlen/2-1 */
                for (k = 2; k < (rlen >> 1); k++) {
                    unsigned w = rd16(tmp + 2 * k) + rd16(tmp + 2 * k - 4);
                    wr16(tmp + 2 * k, w & 0xFFFFU);
                }
                memcpy(buf + d7c, tmp, (size_t)rlen);
                free(tmp);
                exe = 0;
                /* scanning resumes at d90 (esi already there) */
            }
        }
    }

    /* ---- E8/E9 rewrite with the 128-entry address table ---- */
    for (j = 0; j < 128; j++) tab[j] = j;

    i = esi;
    while (i < end) {
        unsigned char c = buf[i];
        if (c != 0xE8 && c != 0xE9) { i++; continue; }

        {
            long v = rd32s(buf + i + 1);
            unsigned long adv = 4;

            if (v != 0) {
                if (v > 0 && (unsigned long)v <= len - 1UL) {
                    if (v == (long)i) v = 0;
                    wr32(buf + i + 1, (unsigned long)v - i);
                    adv = 5;
                } else if (v < 0 && v >= -(long)i) {
                    wr32(buf + i + 1, len + (unsigned long)v);
                    adv = 5;
                } else {
                    unsigned hi = (unsigned)(((unsigned long)v >> 16) &
                                             0xFFFFUL);
                    unsigned hitot = (unsigned)((len - 1UL) >> 16) & 0xFFFFU;
                    unsigned hineg = (unsigned)(((0UL - i) >> 16) & 0xFFFFUL);
                    if (hi != 0 && hi != hitot && hi != hineg) {
                        unsigned low = (unsigned)v & 0xFFFFU;
                        unsigned idx = (unsigned)((low - 0x8000U) & 0xFFFFU);
                        if (idx < 0x80U) {
                            unsigned val = tab[idx];
                            wr16(buf + i + 1,
                                 (unsigned)((val - (unsigned)i) & 0xFFFFU));
                            if (idx != 0) {
                                unsigned q = idx;
                                while (q > 0) {
                                    tab[q] = tab[q - 1];
                                    q--;
                                }
                                tab[0] = val;
                            }
                            adv = 4;
                        } else {
                            unsigned guard = 0;
                            for (;;) {
                                unsigned target =
                                    (unsigned)((low + (unsigned)i) & 0xFFFFU);
                                int found = -1;
                                unsigned a;
                                for (a = 0; a < 128U; a++) {
                                    if (tab[a] == target) {
                                        found = (int)a;
                                        break;
                                    }
                                }
                                if (found < 0) {
                                    /* insert unless the low word would look
                                     * like a table reference (0x8000..0x807F) */
                                    if (!(target >= 0x8000U && target < 0x8080U)) {
                                        unsigned q = 127U;
                                        while (q > 0) {
                                            tab[q] = tab[q - 1];
                                            q--;
                                        }
                                        tab[0] = target;
                                    }
                                    break;
                                }
                                low = (unsigned)(0x8000U | (unsigned)found);
                                wr16(buf + i + 1, low);
                                if (++guard > 0x10000UL) break;
                            }
                            adv = 4;
                        }
                    }
                }
            }
            i += adv;
        }
    }
}
