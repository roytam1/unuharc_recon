/* UHARC 0.6 container: header, directory, solid extraction - C89 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "uharc06.h"
#include "uharc_dec.h"
#include "uharc_filter.h"
#include "uhcrypt06.h"

/* ---------- CRC32 (IEEE, matches dword_421EB3) ---------- */
static unsigned long crc_tab[256];
static int crc_tab_init = 0;

static void crc_init_tab(void)
{
    unsigned i, j;
    unsigned long c;
    if (crc_tab_init) return;
    for (i = 0; i < 256; i++) {
        c = i;
        for (j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320UL ^ (c >> 1);
            else c >>= 1;
        }
        crc_tab[i] = c;
    }
    crc_tab_init = 1;
}

unsigned long uharc06_crc32(const unsigned char *buf, unsigned len,
                            unsigned long crc)
{
    unsigned i;
    crc_init_tab();
    for (i = 0; i < len; i++)
        crc = crc_tab[(unsigned char)(crc ^ buf[i])] ^ (crc >> 8);
    return crc;
}

void uharc06_header_decrypt(unsigned char hdr[39])
{
    unsigned char key;
    unsigned i;
    key = hdr[37];
    for (i = 0; i < 31; i++) {
        hdr[4 + i] ^= (unsigned char)((i * i + key) & 0xFF);
    }
}

static unsigned rd16le(const unsigned char *p)
{
    return (unsigned)(p[0] | (p[1] << 8));
}

static unsigned long rd24le(const unsigned char *p)
{
    return (unsigned long)(p[0] | (p[1] << 8) | (p[2] << 16));
}

int uharc06_header_parse(const unsigned char raw[39],
                         uharc06_header *h)
{
    unsigned char b[39];
    unsigned long crc;
    unsigned i;
    memcpy(b, raw, 39);
    if (!(b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x41 && b[3] == 0x06))
        return UHARC06_ERR_MAGIC;
    uharc06_header_decrypt(b);
    crc_init_tab();
    crc = 0xFFFFFFFFUL;
    for (i = 4; i < 4 + 31; i++)
        crc = crc_tab[(unsigned char)(crc ^ b[i])] ^ (crc >> 8);
    if (((unsigned)(~crc & 0xFFFFU)) != rd16le(b + 37))
        return UHARC06_ERR_CRC;
    h->magic = 0x06414855UL;
    h->dostime = rd16le(b + 4);
    h->dosdate = rd16le(b + 6);
    h->flags1 = b[8];
    h->flags2 = b[9];
    h->method = b[10];
    h->ndirs = rd16le(b + 11);
    h->total_size = (unsigned long)(b[13] | (b[14] << 8) | (b[15] << 16) | (b[16] << 24));
    h->data_off = (unsigned long)(b[17] | (b[18] << 8) | (b[19] << 16) | (b[20] << 24));
    h->nfiles = rd24le(b + 21);
    h->tab1_dec_size = rd24le(b + 24);
    h->tab1_cmp_size = rd24le(b + 27);
    h->tab2_cmp_size = rd24le(b + 30);
    h->tab1_crc = rd16le(b + 33);
    h->tab2_crc = rd16le(b + 35);
    h->hdr_crc = rd16le(b + 37);
    return UHARC06_OK;
}

/* ---------- internal handle ---------- */
struct uharc06_s {
    FILE *fp;
    char *path;
    uharc06_header hdr;
    unsigned long nfiles;
    uharc06_entry *entries;
    char *strpool; /* dir + name strings */
    /* raw compressed table sizes for seeking */
    long tab0_off;
    long tab1_off;
    long tab2_off;
    long data_off;
    char *pwd; /* heap copy of -pw password, NULL when none given */
};

const char *uharc06_strerror(int code)
{
    switch (code) {
    case UHARC06_OK: return "OK";
    case UHARC06_ERR_IO: return "I/O error";
    case UHARC06_ERR_NOMEM: return "Out of memory";
    case UHARC06_ERR_MAGIC: return "No UHARC 0.6 archive";
    case UHARC06_ERR_VERSION: return "Unsupported version";
    case UHARC06_ERR_CRC: return "CRC failure";
    case UHARC06_ERR_METHOD: return "Unknown method";
    case UHARC06_ERR_DATA: return "Data error";
    case UHARC06_ERR_PASSWORD: return "Password required";
    default: return "Unknown error";
    }
}

/* ---------- I/O helpers for decoder ---------- */
typedef struct file_reader_s {
    FILE *fp;
    unsigned long remaining; /* input limit (dword_433DDC) */
    int use_crypt;
    unsigned char crypt_state; /* byte_433E38 */
    unsigned char crypt_tab[256]; /* byte_433150 */
} file_reader;

static int fr_read(void *ctx, unsigned char *buf, unsigned want)
{
    file_reader *fr = (file_reader *)ctx;
    unsigned n, i;
    if (fr->remaining == 0) return 0;
    if (want > fr->remaining) want = (unsigned)fr->remaining;
    n = (unsigned)fread(buf, 1, want, fr->fp);
    if (n == 0) return 0;
    fr->remaining -= n;
    if (fr->use_crypt) {
        for (i = 0; i < n; i++) {
            /* sub_40367A stream: simplified - disabled when no password */
            buf[i] ^= 0;
        }
    }
    return (int)n;
}

typedef struct mem_writer_s {
    unsigned char *buf;
    unsigned long cap;
    unsigned long pos;
    unsigned long crc;
} mem_writer;

static int mw_write(void *ctx, const unsigned char *buf, unsigned len)
{
    mem_writer *mw = (mem_writer *)ctx;
    if (mw->pos + len > mw->cap) return 1;
    memcpy(mw->buf + mw->pos, buf, len);
    mw->crc = uharc06_crc32(buf, len, mw->crc);
    mw->pos += len;
    return 0;
}

typedef struct mem_reader_s {
    const unsigned char *buf;
    unsigned long len;
    unsigned long pos;
} mem_reader;

static int mr_read(void *ctx, unsigned char *buf, unsigned want)
{
    mem_reader *m = (mem_reader *)ctx;
    unsigned long avail = m->len - m->pos;
    if (avail == 0) return 0;
    if (want > avail) want = (unsigned)avail;
    memcpy(buf, m->buf + m->pos, want);
    m->pos += want;
    return (int)want;
}

/* decode one table: read cmp_len bytes at off, decrypt with fresh
 * S-box (sub_4017BE/sub_40367A, fresh per table), then decode from
 * memory. A non-NULL pwd selects the -pw password cipher instead of
 * the default password. crc_init is incoming CRC (FFFFFFFF for
 * fresh, or accumulated for cumulative tab0+tab1 check). */
static int decode_table(FILE *fp, long off, unsigned long cmp_size,
                        int method, unsigned char *out,
                        unsigned long dec_size, unsigned long crc_init,
                        unsigned long *crc_out, const char *pwd)
{
    mem_reader mr;
    mem_writer mw;
    unsigned char *cmp = 0;
    unsigned long cmp_len;
    uhcrypt06_t *cr = 0;
    int rc;
    size_t got;
    if (fseek(fp, off, SEEK_SET) != 0) return UHARC06_ERR_IO;
    cmp_len = (method == 0) ? dec_size : cmp_size;
    if (cmp_len == 0 && dec_size == 0) {
        if (crc_out) *crc_out = crc_init;
        return UHARC06_OK;
    }
    cmp = (unsigned char *)malloc(cmp_len ? cmp_len : 1);
    if (!cmp) return UHARC06_ERR_NOMEM;
    got = fread(cmp, 1, cmp_len, fp);
    if (got != cmp_len) {
        free(cmp);
        return UHARC06_ERR_IO;
    }
    /* fresh cipher per table (sub_4017BE), in-place; -pw archives
     * use the user password, plain archives the default password */
    cr = pwd ? uhcrypt06_init_pw(pwd) : uhcrypt06_init();
    if (!cr) {
        free(cmp);
        return UHARC06_ERR_NOMEM;
    }
    uhcrypt06_update(cr, cmp, cmp_len);
    uhcrypt06_free(cr);
    mr.buf = cmp;
    mr.len = cmp_len;
    mr.pos = 0;
    mw.buf = out;
    mw.cap = dec_size;
    mw.pos = 0;
    mw.crc = crc_init;
    rc = uharc_dec_decode(method, mr_read, &mr, mw_write, &mw, dec_size);
    free(cmp);
    if (rc != 0) {
        if (rc == 2) return UHARC06_ERR_NOMEM;
        return UHARC06_ERR_DATA;
    }
    if (mw.pos != dec_size) return UHARC06_ERR_DATA;
    if (crc_out) *crc_out = mw.crc;
    return UHARC06_OK;
}

unsigned long uharc06_nfiles(uharc06_t *h)
{
    return h ? h->nfiles : 0;
}

const uharc06_entry *uharc06_entry_get(uharc06_t *h, unsigned long idx)
{
    if (!h || idx >= h->nfiles) return 0;
    return &h->entries[idx];
}

const uharc06_header *uharc06_header_get(uharc06_t *h)
{
    return h ? &h->hdr : 0;
}

int uharc06_open(const char *path, uharc06_t **ph)
{
    return uharc06_open_pw(path, ph, 0);
}

int uharc06_open_pw(const char *path, uharc06_t **ph, const char *pwd)
{
    FILE *fp;
    unsigned char raw[39];
    uharc06_header hdr;
    uharc06_t *h;
    int rc;
    long tab0_off, tab1_off, tab2_off, data_stream_off;
    unsigned long nfiles, ndirs;
    unsigned char *tab0 = 0, *tab1 = 0, *tab2 = 0;
    unsigned long crc;
    unsigned long i;
    int have_pw;
    int tab_pw;
    int stm_pw;
    const char *tpwd;

    if (!path || !ph) return UHARC06_ERR_IO;
    *ph = 0;
    fp = fopen(path, "rb");
    if (!fp) return UHARC06_ERR_IO;
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return UHARC06_ERR_IO;
    }
    /* SFX overlay check (sub_4031D7): if MZ, skip to overlay.
     * Minimal: check first 2 bytes; if "MZ", read e_lfanew at 0x3C. */
    {
        unsigned char mz[64];
        size_t n;
        n = fread(mz, 1, 64, fp);
        if (n >= 64 && mz[0] == 'M' && mz[1] == 'Z') {
            unsigned long e_lfanew;
            unsigned long pe_off;
            unsigned char pe[8];
            e_lfanew = (unsigned long)(mz[0x3C] | (mz[0x3D] << 8) |
                                       (mz[0x3E] << 16) | (mz[0x3F] << 24));
            if (fseek(fp, (long)e_lfanew, SEEK_SET) == 0 &&
                fread(pe, 1, 6, fp) == 6 &&
                pe[0] == 'P' && pe[1] == 'E') {
                /* overlay starts after sections: fallback to search for UHA magic.
                 * Simple scan from end: try data_off approach.
                 * For now, rewind and require plain UHA (SFX rare). */
            }
            fseek(fp, 0, SEEK_SET);
        } else {
            fseek(fp, 0, SEEK_SET);
        }
    }
    if (fread(raw, 1, 39, fp) != 39) {
        fclose(fp);
        return UHARC06_ERR_IO;
    }
    rc = uharc06_header_parse(raw, &hdr);
    if (rc != UHARC06_OK) {
        fclose(fp);
        return rc;
    }
    nfiles = hdr.nfiles;
    ndirs = hdr.ndirs;
    if (nfiles > 1000000UL || ndirs > 100000UL) {
        fclose(fp);
        return UHARC06_ERR_DATA;
    }
    /* -pw protection (flags1 bit 2 = tables, bit 1 = packed stream).
     * A protected archive without a password cannot be opened. */
    have_pw = (pwd && *pwd) ? 1 : 0;
    tab_pw = (hdr.flags1 & 4) ? 1 : 0;
    stm_pw = (hdr.flags1 & 2) ? 1 : 0;
    tpwd = (have_pw && tab_pw) ? pwd : 0;
    if ((tab_pw || stm_pw) && !have_pw) {
        fclose(fp);
        return UHARC06_ERR_PASSWORD;
    }
    h = (uharc06_t *)calloc(1, sizeof(*h));
    if (!h) {
        fclose(fp);
        return UHARC06_ERR_NOMEM;
    }
    h->fp = fp;
    h->hdr = hdr;
    h->nfiles = nfiles;
    h->path = (char *)malloc(strlen(path) + 1);
    if (h->path) strcpy(h->path, path);
    h->pwd = 0;
    if (have_pw) {
        h->pwd = (char *)malloc(strlen(pwd) + 1);
        if (!h->pwd) {
            uharc06_close(h);
            return UHARC06_ERR_NOMEM;
        }
        strcpy(h->pwd, pwd);
    }

    tab0_off = (long)(hdr.data_off + 39);
    /* tab0 STORE: dec==4*nfiles */
    tab1_off = tab0_off + (long)(4 * nfiles);
    tab2_off = tab1_off + (long)hdr.tab1_cmp_size;
    data_stream_off = tab2_off + (long)hdr.tab2_cmp_size;
    h->tab0_off = tab0_off;
    h->tab1_off = tab1_off;
    h->tab2_off = tab2_off;
    h->data_off = data_stream_off;

    if (nfiles == 0) {
        *ph = h;
        return UHARC06_OK;
    }

    tab0 = (unsigned char *)malloc(4 * (nfiles ? nfiles : 1));
    tab1 = (unsigned char *)malloc(hdr.tab1_dec_size ? hdr.tab1_dec_size : 1);
    tab2 = (unsigned char *)malloc(11 * nfiles);
    if (!tab0 || !tab1 || !tab2) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return UHARC06_ERR_NOMEM;
    }

    /* table0: method 0 STORE, 4*nfiles (file CRCs).
     * CRC accumulates across tab0+tab1 for hdr.tab1_crc check
     * (sub_403840 keeps dword_433DD4 across first two tables). */
    rc = decode_table(fp, tab0_off, 4 * nfiles, 0,
                      tab0, 4 * nfiles, 0xFFFFFFFFUL, &crc, tpwd);
    if (rc != UHARC06_OK) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return rc;
    }
    /* table1: method 17 (core3 ALZ), dec=tab1_dec, cmp=tab1_cmp.
     * Pass accumulated tab0 CRC so output CRC is cumulative. */
    rc = decode_table(fp, tab1_off, hdr.tab1_cmp_size, 17,
                      tab1, hdr.tab1_dec_size, crc, &crc, tpwd);
    if (rc != UHARC06_OK) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return rc;
    }
    if (((unsigned)(~crc & 0xFFFFU)) != (hdr.tab1_crc & 0xFFFFU)) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return UHARC06_ERR_CRC;
    }
    /* table2: method 17, dec=11*nfiles, cmp=tab2_cmp.
     * CRC reset before third table (sub_403840 sets dword_433DD4=-1). */
    rc = decode_table(fp, tab2_off, hdr.tab2_cmp_size, 17,
                      tab2, 11 * nfiles, 0xFFFFFFFFUL, &crc, tpwd);
    if (rc != UHARC06_OK) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return rc;
    }
    if (((unsigned)(~crc & 0xFFFFU)) != (hdr.tab2_crc & 0xFFFFU)) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return rc;
    }

    /* rebuild 19-byte entries */
    {
        unsigned char *dirs = 0;
        unsigned long dirs_total = 0;
        unsigned long strs_total = 0;
        unsigned long p1 = 0, p2 = 0;
        unsigned i2;
        unsigned long *dir_offs = 0;
        /* tab1 layout: ndirs pascal strings, then nfiles pascal strings */
        /* first pass: compute sizes */
        p1 = 0;
        for (i2 = 0; i2 < ndirs; i2++) {
            unsigned l;
            if (p1 >= hdr.tab1_dec_size) break;
            l = tab1[p1++];
            if (p1 + l > hdr.tab1_dec_size) break;
            p1 += l;
            dirs_total += l + 1;
        }
        for (i = 0; i < nfiles; i++) {
            unsigned l;
            if (p1 >= hdr.tab1_dec_size) break;
            l = tab1[p1++];
            if (p1 + l > hdr.tab1_dec_size) break;
            p1 += l;
            strs_total += l + 1;
        }
        h->entries = (uharc06_entry *)calloc(nfiles ? nfiles : 1,
                                             sizeof(uharc06_entry));
        if (!h->entries) {
            free(tab0);
            free(tab1);
            free(tab2);
            uharc06_close(h);
            return UHARC06_ERR_NOMEM;
        }
        h->strpool = (char *)malloc(dirs_total + strs_total + nfiles * 260 + 1024);
        if (!h->strpool) {
            free(tab0);
            free(tab1);
            free(tab2);
            uharc06_close(h);
            return UHARC06_ERR_NOMEM;
        }
        dir_offs = (unsigned long *)malloc(sizeof(unsigned long) * (ndirs ? ndirs : 1));
        if (!dir_offs && ndirs) {
            free(tab0);
            free(tab1);
            free(tab2);
            uharc06_close(h);
            return UHARC06_ERR_NOMEM;
        }
        /* copy dir strings */
        p1 = 0;
        p2 = 0;
        for (i2 = 0; i2 < ndirs; i2++) {
            unsigned l;
            l = tab1[p1++];
            dir_offs[i2] = p2;
            memcpy(h->strpool + p2, tab1 + p1, l);
            h->strpool[p2 + l] = 0;
            p1 += l;
            p2 += l + 1;
        }
        /* tab2 planes: [0..2N)=time? Actually layout from lst:
         * bytes 0..2N-1: time/date words? Let's follow lst order:
         * first N words -> +13 (time), second N words -> +11 (date),
         * next 2N bytes -> +0 word (dir_index+flags?),
         * next 4N bytes -> assembled dword for +3 (size),
         * last N bytes -> +2 method.
         * tab0 gives +7 crc.
         */
        {
            unsigned char *b = tab2;
            unsigned long off = 0;
            char *sp;
            unsigned k;
            /* sizes plane is at 6N..10N, method at 10N */
            sp = h->strpool + dirs_total;
            for (k = 0; k < nfiles; k++) {
                uharc06_entry *e = &h->entries[k];
                unsigned t, d;
                unsigned w0;
                unsigned char m;
                unsigned long sz;
                unsigned long crcf;
                t = (unsigned)(b[2 * k] | (b[2 * k + 1] << 8));
                d = (unsigned)(b[2 * nfiles + 2 * k] |
                               (b[2 * nfiles + 2 * k + 1] << 8));
                w0 = (unsigned)(b[4 * nfiles + k] |
                                (b[5 * nfiles + k] << 8));
                sz = (unsigned long)(b[6 * nfiles + k] |
                                     (b[7 * nfiles + k] << 8) |
                                     (b[8 * nfiles + k] << 16) |
                                     (b[9 * nfiles + k] << 24));
                /* note: on-disk +3 is stored as negative cumulative?
                 * The lst sub_403C28 does var-=dword, storing negative.
                 * But sub_4051A2 uses [esi+3] directly as size.
                 * Resolve: sizes are stored directly; the negative
                 * accumulation is for progress table c5, not entry.
                 * Re-check: entry+3 = assembled dword (not negated)?
                 * Actually var_1C accumulates, but entry gets var_1C.
                 * For D3X we validate sum==total; if mismatch, try
                 * complement. Handled below.
                 */
                m = b[10 * nfiles + k];
                crcf = (unsigned long)(tab0[4 * k] | (tab0[4 * k + 1] << 8) |
                                       (tab0[4 * k + 2] << 16) |
                                       (tab0[4 * k + 3] << 24));
                e->time = t;
                e->date = d;
                e->dir_index = w0;
                e->method = m;
                /* raw var (negative cumulative, sub_403C28 var-=dword).
                 * Converted to sizes below via differences. */
                e->size = sz & 0xFFFFFFFFUL;
                e->crc = crcf;
                e->offset = 0;
                /* filename from tab1 second part */
                {
                    unsigned l;
                    /* find k-th file name: skip ndirs dirs already (p1 at file section) */
                    /* recompute file name offset */
                    unsigned long q = 0;
                    unsigned kk;
                    unsigned long qq = 0;
                    /* locate: after dirs, file names sequential */
                    /* we already consumed dirs; now walk to k */
                    /* instead, precompute during first pass */
                    (void)q;
                    (void)kk;
                    (void)qq;
                    l = 0; /* placeholder, fixed below */
                }
                e->dir = "";
                e->name = "";
                e->path = "";
            }
            /* entry+3 on disk is delta (prev_size - curr_size, sub_403C28
             * var-=dword pattern with var holding previous size? Actually
             * var_i = size_{i-1} - size_i, var_0 = 0 - size_0.
             * Recover sizes iteratively: size_0 = -var_0,
             * size_i = size_{i-1} - var_i (32-bit wrap). Validated on
             * 06_m1 corpus (vars [0,-2280,1280,-24,0,512] -> sizes
             * [0,2280,1000,1024,1024,512], sum 5840). */
            {
                unsigned long k2;
                unsigned long prev_sz = 0;
                for (k2 = 0; k2 < nfiles; k2++) {
                    unsigned long var = h->entries[k2].size & 0xFFFFFFFFUL;
                    unsigned long sz2;
                    if (k2 == 0) sz2 = (0UL - var) & 0xFFFFFFFFUL;
                    else sz2 = (prev_sz - var) & 0xFFFFFFFFUL;
                    h->entries[k2].size = sz2;
                    prev_sz = sz2;
                }
                {
                    unsigned long off2 = 0;
                    for (k2 = 0; k2 < nfiles; k2++) {
                        h->entries[k2].offset = off2;
                        off2 = (off2 + h->entries[k2].size) & 0xFFFFFFFFUL;
                    }
                }
            }
            /* second pass for names: walk tab1 again */
            {
                unsigned long q = 0;
                unsigned kk;
                /* skip dirs */
                for (kk = 0; kk < ndirs; kk++) {
                    unsigned l = tab1[q++];
                    q += l;
                }
                for (kk = 0; kk < nfiles; kk++) {
                    unsigned l = tab1[q++];
                    uharc06_entry *e = &h->entries[kk];
                    memcpy(sp, tab1 + q, l);
                    sp[l] = 0;
                    q += l;
                    e->name = sp;
                    sp += l + 1;
                    /* dir */
                    if ((e->dir_index & 0xFFFFU) < ndirs) {
                        e->dir = h->strpool + dir_offs[e->dir_index & 0xFFFFU];
                    } else {
                        e->dir = "";
                    }
                    /* path: strip trailing slashes from dir to avoid // */
                    e->path = sp;
                    if (e->dir[0]) {
                        char *dend;
                        strcpy(sp, e->dir);
                        dend = sp + strlen(sp);
                        while (dend > sp && (dend[-1] == '/' || dend[-1] == '\\'))
                            *--dend = 0;
                        strcat(sp, "/");
                        strcat(sp, e->name);
                    } else {
                        strcpy(sp, e->name);
                    }
                    /* normalize backslash */
                    {
                        char *s = sp;
                        while (*s) {
                            if (*s == '\\') *s = '/';
                            s++;
                        }
                    }
                    sp += strlen(sp) + 1;
                }
            }
        }
        free(dir_offs);
    }

    free(tab0);
    free(tab1);
    free(tab2);
    *ph = h;
    return UHARC06_OK;
}

void uharc06_close(uharc06_t *h)
{
    if (!h) return;
    if (h->fp) fclose(h->fp);
    free(h->path);
    free(h->pwd);
    free(h->entries);
    free(h->strpool);
    free(h);
}

/* ---- solid-stream splitter (mirrors sub_4051A2) ----
 * Single-threaded globals pointing at extract() locals. */
static uharc06_t *g_sp_h;
static const char *g_sp_outdir;
static int g_sp_with_paths;
static int g_sp_test_only;
static unsigned long *g_sp_pidx;
static unsigned long *g_sp_prem;
static FILE **g_sp_pcur;
static unsigned long *g_sp_pcrc;
static char *g_sp_curpath;
static unsigned g_sp_curpath_cap;
static int g_sp_crc_err;
/* 0.6 EXE post-filter (sub_40481E) is applied per file, after decode and
 * before CRC/write (the original buffers to the file length).  Gate is
 * header flags2 bit0 and file size >= 0x400 (sub_4055B1).  Bit1 (the
 * multimedia filter sub_404665) is not implemented yet. */
static int g_sp_filt;
static int g_sp_fuse;
static unsigned char *g_sp_fbuf;
static unsigned long g_sp_fcap;
static unsigned long g_sp_ffill;

static void g_sp_mkdir_parents(char *path)
{
    char *p;
    for (p = path; *p; p++) {
        if (*p == '/') {
            char save = *p;
            *p = 0;
#ifdef _WIN32
            mkdir(path);
#else
            mkdir(path, 0755);
#endif
            *p = save;
        }
    }
}

static int g_sp_open_cur(void)
{
    uharc06_t *h = g_sp_h;
    unsigned long i = *g_sp_pidx;
    const uharc06_entry *e = &h->entries[i];
    char *dst = g_sp_curpath;
    unsigned long fsz = e->size & 0xFFFFFFFFUL;
    dst[0] = 0;
    if (g_sp_with_paths && e->path[0]) {
        size_t od = strlen(g_sp_outdir);
        size_t need = od + 1 + strlen(e->path) + 1;
        if (need > g_sp_curpath_cap) return 1;
        memcpy(dst, g_sp_outdir, od);
        dst[od] = '/';
        strcpy(dst + od + 1, e->path);
    } else {
        size_t od = strlen(g_sp_outdir);
        size_t need = od + 1 + strlen(e->name) + 1;
        if (need > g_sp_curpath_cap) return 1;
        memcpy(dst, g_sp_outdir, od);
        dst[od] = '/';
        strcpy(dst + od + 1, e->name);
    }
    if (!g_sp_test_only) {
        g_sp_mkdir_parents(dst);
        *g_sp_pcur = fopen(dst, "wb");
        if (!*g_sp_pcur) return 1;
    } else {
        *g_sp_pcur = 0;
    }
    *g_sp_pcrc = 0xFFFFFFFFUL;
    g_sp_fuse = 0;
    g_sp_ffill = 0;
    if (g_sp_filt && fsz >= 0x400UL) {
        if (fsz > g_sp_fcap) {
            unsigned char *nb = (unsigned char *)realloc(g_sp_fbuf,
                                                         (size_t)fsz);
            if (!nb) return 1;
            g_sp_fbuf = nb;
            g_sp_fcap = fsz;
        }
        g_sp_fuse = 1;
    }
    return 0;
}

/* returns 0 ok, 1 io/error (decoder maps non-zero to err 4; CRC flag distinguishes) */
static int uharc06_split_write(void *ctx, const unsigned char *buf, unsigned len)
{
    uharc06_t *h = g_sp_h;
    (void)ctx;
    while (len > 0) {
        unsigned long chunk;
        unsigned long ci;
        if (*g_sp_pidx >= h->nfiles) {
            return 1;
        }
        /* TODO skip marker method 0xFF: no test vectors; size should be 0.
         * Fail fast on size>0 with method FF rather than mis-extract. */
        if (h->entries[*g_sp_pidx].method == 0xFF &&
            (h->entries[*g_sp_pidx].size & 0xFFFFFFFFUL) != 0) {
            return 1;
        }
        chunk = len;
        if (chunk > *g_sp_prem) chunk = *g_sp_prem;
        if (!g_sp_test_only) {
            if (!*g_sp_pcur) {
                if (g_sp_open_cur() != 0) return 1;
            }
        }
        if (chunk > 0) {
            if (g_sp_fuse) {
                if (g_sp_ffill + chunk > g_sp_fcap) return 1;
                memcpy(g_sp_fbuf + g_sp_ffill, buf, (size_t)chunk);
                g_sp_ffill += chunk;
            } else {
                if (!g_sp_test_only) {
                    size_t w = fwrite(buf, 1, (size_t)chunk, *g_sp_pcur);
                    if (w != (size_t)chunk) return 1;
                }
                *g_sp_pcrc = uharc06_crc32(buf, (unsigned)chunk, *g_sp_pcrc);
            }
        }
        buf += chunk;
        len -= (unsigned)chunk;
        *g_sp_prem -= chunk;
        if (*g_sp_prem == 0) {
            ci = *g_sp_pidx;
            if (g_sp_fuse) {
                /* whole file is buffered: apply the 0.6 inverse EXE filter
                 * before CRC and write. f6 = native byte_433E36, set once per
                 * archive from byte_42251F[ARCHIVE method]==4 (asm 0x405C16;
                 * the table holds 4 exactly at 24..39). It is NOT per-file:
                 * gmud-06m3 (archive method 87) and gmud-06z (23) must not
                 * get the pre-transform even for 24..39-method members. */
#ifdef SPLITDBG
                if (g_sp_ffill > 0) {
                    FILE *df = fopen("C:/Users/Roy/AppData/Local/Temp/opencode/fileX_raw.bin", "wb");
                    if (df) { fwrite(g_sp_fbuf, 1, (size_t)g_sp_ffill, df); fclose(df); }
                }
#endif
                uharc_filter_exe(g_sp_fbuf, g_sp_ffill, 6,
                    (h->hdr.method >= 24 &&
                     h->hdr.method <= 39) ? 1 : 0);
#ifdef SPLITDBG
                if (g_sp_ffill > 0) {
                    FILE *df = fopen("C:/Users/Roy/AppData/Local/Temp/opencode/fileX_out.bin", "wb");
                    if (df) { fwrite(g_sp_fbuf, 1, (size_t)g_sp_ffill, df); fclose(df); }
                }
#endif
                if (!g_sp_test_only) {
                    size_t w = fwrite(g_sp_fbuf, 1, (size_t)g_sp_ffill,
                                      *g_sp_pcur);
                    if (w != (size_t)g_sp_ffill) return 1;
                }
                *g_sp_pcrc = uharc06_crc32(g_sp_fbuf, (unsigned)g_sp_ffill,
                                           *g_sp_pcrc);
                g_sp_fuse = 0;
                g_sp_ffill = 0;
            }
            if (*g_sp_pcur) { fclose(*g_sp_pcur); *g_sp_pcur = 0; }
            if (((~(*g_sp_pcrc)) & 0xFFFFFFFFUL) !=
                (h->entries[ci].crc & 0xFFFFFFFFUL)) {
                g_sp_crc_err = 1;
#ifdef SPLITDBG
                fprintf(stderr, "SPLITDBG crcfail idx=%u name=%s\n",
                    ci, g_sp_curpath);
#else
                return 1;
#endif
            }
            (*g_sp_pidx)++;
            /* skip empties: create empty file unless test, check crc==0 */
            while (*g_sp_pidx < h->nfiles &&
                   (h->entries[*g_sp_pidx].size & 0xFFFFFFFFUL) == 0) {
                unsigned long zi = *g_sp_pidx;
                /* TODO method 0xFF skip marker: treated like normal empty */
                if ((h->entries[zi].crc & 0xFFFFFFFFUL) != 0) {
                    g_sp_crc_err = 1;
                    return 1;
                }
                if (!g_sp_test_only) {
                    if (g_sp_open_cur() != 0) return 1;
                    if (*g_sp_pcur) { fclose(*g_sp_pcur); *g_sp_pcur = 0; }
                }
                (*g_sp_pidx)++;
            }
            if (*g_sp_pidx < h->nfiles) {
                /* TODO method 0xFF with size>0 fails fast at top of loop */
                *g_sp_prem = h->entries[*g_sp_pidx].size;
                if (g_sp_open_cur() != 0) return 1;
                if (g_sp_test_only) *g_sp_pcur = 0;
            } else {
                *g_sp_pcur = 0;
                *g_sp_prem = 0;
            }
        }
    }
    return 0;
}

int uharc06_extract(uharc06_t *h, const char *outdir,
                    int with_paths, int test_only)
{
    FILE *fp;
    unsigned char *packed = 0;
    unsigned long packed_len;
    size_t got;
    unsigned long idx = 0;
    unsigned long remaining = 0;
    FILE *cur = 0;
    unsigned long cur_crc = 0xFFFFFFFFUL;
    char curpath[2048];
    mem_reader mr;
    int drc;
    int rc = UHARC06_OK;
    unsigned long k;
    unsigned long *cum = 0;
    uhcrypt06_t *scr = 0;

    if (!h || !h->fp) return UHARC06_ERR_IO;
    if (!outdir) outdir = ".";
    fp = h->fp;

    /* wire globals to locals */
    g_sp_h = h;
    g_sp_outdir = outdir;
    g_sp_with_paths = with_paths;
    g_sp_test_only = test_only;
    g_sp_pidx = &idx;
    g_sp_prem = &remaining;
    g_sp_pcur = &cur;
    g_sp_pcrc = &cur_crc;
    g_sp_curpath = curpath;
    g_sp_curpath_cap = (unsigned)sizeof(curpath);
    g_sp_crc_err = 0;
    g_sp_filt = (h->hdr.flags2 & 1) ? 1 : 0;
    g_sp_fuse = 0;
    g_sp_ffill = 0;

    /* TODO skip marker method 0xFF: fail fast if size>0 */
    for (k = 0; k < h->nfiles; k++) {
        if (h->entries[k].method == 0xFF &&
            (h->entries[k].size & 0xFFFFFFFFUL) != 0) {
            rc = UHARC06_ERR_DATA;
            goto done;
        }
    }

    /* Container cumulative file-end table (dword_422934 / dword_45D128):
     * core5/core4 reset their models at these boundaries.  Built from the
     * same per-entry sizes the original uses for its progress table. */
    if (h->nfiles > 0) {
        unsigned long acc = 0;
        cum = (unsigned long *)malloc(sizeof(unsigned long) * h->nfiles);
        if (!cum) { rc = UHARC06_ERR_NOMEM; goto done; }
        for (k = 0; k < h->nfiles; k++) {
            acc = (acc + (h->entries[k].size & 0xFFFFFFFFUL)) & 0xFFFFFFFFUL;
            cum[k] = acc;
        }
        /* contract for the decoder: last entry is the total output size */
        if (cum[h->nfiles - 1] == 0 && h->hdr.total_size != 0) {
            cum[h->nfiles - 1] = h->hdr.total_size;
        }
    }

    /* skip leading empties: create empty file unless test, crc must be 0.
     * Do not consume input for them. */
    while (idx < h->nfiles &&
           (h->entries[idx].size & 0xFFFFFFFFUL) == 0) {
        if ((h->entries[idx].crc & 0xFFFFFFFFUL) != 0) {
            rc = UHARC06_ERR_CRC;
            goto done;
        }
        if (!test_only) {
            if (g_sp_open_cur() != 0) { rc = UHARC06_ERR_IO; goto done; }
            if (cur) { fclose(cur); cur = 0; }
        }
        idx++;
    }
    if (idx < h->nfiles) {
        remaining = h->entries[idx].size;
        if (g_sp_open_cur() != 0) { rc = UHARC06_ERR_IO; goto done; }
        if (test_only) cur = 0;
    } else {
        remaining = 0;
    }

    /* packed file-data range: 39 .. 39+hdr.data_off (data_off = packed len) */
    packed_len = h->hdr.data_off;
    if (fseek(fp, 39L, SEEK_SET) != 0) { rc = UHARC06_ERR_IO; goto done; }
    packed = (unsigned char *)malloc(packed_len ? packed_len : 1);
    if (!packed) { rc = UHARC06_ERR_NOMEM; goto done; }
    got = fread(packed, 1, packed_len, fp);
    if (got != packed_len) { rc = UHARC06_ERR_IO; goto done; }
    /* File data is NOT default-encrypted (unlike tables which use fresh
     * default S-box per table via sub_4017BE). With -pw the packed stream
     * uses a fresh password S-box instead (sub_4017BE(byte_422579),
     * flags1 bit 1); without it the stream is plain. */
    if ((h->hdr.flags1 & 2) && h->pwd) {
        scr = uhcrypt06_init_pw(h->pwd);
        if (!scr) { rc = UHARC06_ERR_NOMEM; goto done; }
        uhcrypt06_update(scr, packed, packed_len);
        uhcrypt06_free(scr);
        scr = 0;
    }

    /* TODO filters (sub_404DCE: exe E8E9, multimedia) gated by flags.
     * 06_m0 (flags 00/00) needs none; D3X (flags 00/01) needs exe filter.
     * Currently raw decompressed bytes are split as-is. */
    mr.buf = packed;
    mr.len = packed_len;
    mr.pos = 0;
    drc = uharc_dec_decode_b((int)h->hdr.method, 0, 6, cum, h->nfiles,
                             mr_read, &mr, uharc06_split_write, 0,
                             h->hdr.total_size);
    if (g_sp_crc_err) rc = UHARC06_ERR_CRC;
    else if (drc == 2) rc = UHARC06_ERR_NOMEM;
    else if (drc != 0) rc = UHARC06_ERR_DATA;
    else rc = UHARC06_OK;
#ifdef SPLITDBG
    fprintf(stderr, "SPLITDBG drc=%d idx=%u nfiles=%u curpath=%s\n",
        drc, idx, h->nfiles, g_sp_curpath);
#endif
    if (rc != UHARC06_OK) goto done;
    /* all files must be accounted for */
    if (idx != h->nfiles) { rc = UHARC06_ERR_DATA; goto done; }

done:
    if (packed) free(packed);
    if (cum) free(cum);
    free(g_sp_fbuf);
    g_sp_fbuf = 0;
    g_sp_fcap = 0;
    if (cur) { fclose(cur); cur = 0; }
    g_sp_h = 0;
    return rc;
}
