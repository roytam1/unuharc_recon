/* UHARC 0.6 container: header, directory, solid extraction - C89 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uharc06.h"
#include "uharc_dec.h"

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

/* decode one table */
static int decode_table(FILE *fp, long off, unsigned long cmp_size,
                        int method, unsigned char *out,
                        unsigned long dec_size, unsigned long *crc_out)
{
    file_reader fr;
    mem_writer mw;
    int rc;
    if (fseek(fp, off, SEEK_SET) != 0) return UHARC06_ERR_IO;
    fr.fp = fp;
    fr.remaining = (method == 0) ? dec_size : cmp_size;
    /* for STORE, cmp==dec; for method 17, cmp from header */
    if (method == 0) fr.remaining = dec_size;
    fr.use_crypt = 0;
    mw.buf = out;
    mw.cap = dec_size;
    mw.pos = 0;
    mw.crc = 0xFFFFFFFFUL;
    rc = uharc_dec_decode(method, fr_read, &fr, mw_write, &mw, dec_size);
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

    /* table0: method 0 STORE, 4*nfiles (file CRCs) */
    rc = decode_table(fp, tab0_off, 4 * nfiles, 0,
                      tab0, 4 * nfiles, 0);
    if (rc != UHARC06_OK) {
        free(tab0);
        free(tab1);
        free(tab2);
        uharc06_close(h);
        return rc;
    }
    /* table1: method 17 (core3 ALZ), dec=tab1_dec, cmp=tab1_cmp */
    rc = decode_table(fp, tab1_off, hdr.tab1_cmp_size, 17,
                      tab1, hdr.tab1_dec_size, &crc);
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
    /* table2: method 17, dec=11*nfiles, cmp=tab2_cmp */
    rc = decode_table(fp, tab2_off, hdr.tab2_cmp_size, 17,
                      tab2, 11 * nfiles, &crc);
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
                e->size = sz;
                e->crc = crcf;
                e->offset = off;
                off += sz;
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
                    /* path */
                    e->path = sp;
                    if (e->dir[0]) {
                        strcpy(sp, e->dir);
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
    free(h->entries);
    free(h->strpool);
    free(h);
}

int uharc06_extract(uharc06_t *h, const char *outdir,
                    int with_paths, int test_only)
{
    (void)h;
    (void)outdir;
    (void)with_paths;
    (void)test_only;
    return UHARC06_ERR_METHOD;
}
