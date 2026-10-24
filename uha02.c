/* UHARC 0.2 container: header, directory, solid extraction - C89.
 * Reverse engineered from Uharc02.c / Uharc02.lst (UHARC 0.2, DOS32).
 * Solid stream like 0.6 (NOT per-file like 0.3/0.4): file data is one
 * compressed block at offset 48, split across files on output.
 * 48-byte header, magic "UHA\x02". 4 directory tables follow the data:
 * tab0 STORE 4*N (file CRCs), tab1 method 7 (names_len bytes),
 * tab2 method 2 (4*N dates/times), tab3 method 7 (7*N dir/size/method).
 * Each table is decrypted with a fresh 0.2 default-password S-box
 * BEFORE decode (static cipher below; verified against Uharc02.c
 * sub_10840/sub_108C8/sub_12AE0 and Uharc02.lst - NOT byte-identical
 * to the 0.4/0.6 cipher, so uhcrypt04.h is deliberately not used).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include "uha02.h"
#include "uharc_dec.h"
/* 0.2 table cipher (Uharc02.c sub_10840/sub_108C8/sub_12AE0).
 * NOTE: this is NOT byte-identical to the 0.4/0.6 cipher, so
 * uhcrypt04.h is deliberately NOT used here (verified against the
 * 0.2 binary and real vectors: no table decodes with it):
 *  - S-box fill: all 256 bytes cycle the 22 password bytes
 *    ("UHARC Default Password", no length-prefix byte), whereas
 *    0.4/0.6 put the length in sb[0] and cycle from sb[1].
 *  - key schedule shape is the same (32 x CRC32-IEEE over the
 *    whole box, XOR crc / ~crc into 8-byte chunks, LE order).
 *  - stream (sub_12AE0) is simpler: a = S[pos++]; S[pos] += a;
 *    out = S[pos] ^ in (no second/mix index, pos wraps at 256).
 * Kept static in this file: no new crypt files. */

/* ---------- CRC32 (IEEE) ---------- */
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

unsigned long uha02_crc32(const unsigned char *buf, unsigned len,
                          unsigned long crc)
{
    unsigned i;
    crc_init_tab();
    for (i = 0; i < len; i++)
        crc = crc_tab[(unsigned char)(crc ^ buf[i])] ^ (crc >> 8);
    return crc;
}

/* ---------- 0.2 table cipher (static, see note at top) ---------- */
typedef struct uha02_crypt_s {
    unsigned char s[256];
    unsigned char pos;
} uha02_crypt;

static void uha02_crypt_init(uha02_crypt *c)
{
    /* strlen("UHARC Default Password") == 22 */
    static const char pwd[] = "UHARC Default Password";
    unsigned char sb[256];
    unsigned i, off, k;
    unsigned long crc;
    for (i = 0; i < 256; i++)
        sb[i] = (unsigned char)pwd[i % 22];
    for (off = 0; off < 256; off += 8) {
        crc = 0xFFFFFFFFUL;
        for (i = 0; i < 256; i++)
            crc = crc_tab[(unsigned char)(crc ^ sb[i])] ^ (crc >> 8);
        for (k = 0; k < 4; k++) {
            sb[off + k] ^= (unsigned char)((crc >> (8 * k)) & 0xFF);
            sb[off + 4 + k] ^= (unsigned char)(((~crc) >> (8 * k)) & 0xFF);
        }
    }
    memcpy(c->s, sb, 256);
    memset(sb, 0, sizeof(sb));
    c->pos = 0;
}

static void uha02_crypt_update(uha02_crypt *c, unsigned char *data,
                               unsigned long len)
{
    unsigned long i;
    for (i = 0; i < len; i++) {
        unsigned char a = c->s[c->pos];
        c->pos = (unsigned char)(c->pos + 1);
        c->s[c->pos] = (unsigned char)(c->s[c->pos] + a);
        data[i] ^= c->s[c->pos];
    }
}

void uha02_header_decrypt(unsigned char hdr[48])
{
    unsigned char key;
    unsigned i;
    key = hdr[44];
    for (i = 0; i < 40; i++) {
        hdr[4 + i] ^= (unsigned char)((key + i * i) & 0xFF);
    }
}

static unsigned rd16le(const unsigned char *p)
{
    return (unsigned)(p[0] | (p[1] << 8));
}

static unsigned long rd32le(const unsigned char *p)
{
    return (unsigned long)(p[0] | (p[1] << 8) |
                           (p[2] << 16) | (p[3] << 24));
}

int uha02_header_parse(const unsigned char raw[48],
                       uha02_header *h)
{
    unsigned char b[48];
    unsigned long crc;
    unsigned i;
    memcpy(b, raw, 48);
    if (!(b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x41 && b[3] == 0x02))
        return UHA02_ERR_MAGIC;
    uha02_header_decrypt(b);
    crc_init_tab();
    crc = 0xFFFFFFFFUL;
    for (i = 4; i < 4 + 40; i++)
        crc = crc_tab[(unsigned char)(crc ^ b[i])] ^ (crc >> 8);
    if ((~crc & 0xFFFFFFFFUL) != rd32le(b + 44))
        return UHA02_ERR_CRC;
    h->magic = 0x02414855UL;
    h->dostime = rd16le(b + 4);
    h->dosdate = rd16le(b + 6);
    h->reserved = b[8];
    h->method = b[9];
    h->ndirs = rd16le(b + 10);
    h->nfiles = rd32le(b + 12);
    h->total_size = rd32le(b + 16);
    h->data_len = rd32le(b + 20);
    h->names_len = rd32le(b + 24);
    h->cmp_sz1 = rd32le(b + 28);
    h->cmp_sz2 = rd32le(b + 32);
    h->cmp_sz3 = rd32le(b + 36);
    h->contents_crc = rd32le(b + 40);
    h->hdr_crc = rd32le(b + 44);
    return UHA02_OK;
}

/* ---------- internal handle ---------- */
struct uha02_s {
    FILE *fp;
    char *path;
    uha02_header hdr;
    unsigned long nfiles;
    uha02_entry *entries;
    char *strpool; /* dir + name strings */
    long tab0_off;
    long tab1_off;
    long tab2_off;
    long tab3_off;
    long data_off;
};

const char *uha02_strerror(int code)
{
    switch (code) {
    case UHA02_OK: return "OK";
    case UHA02_ERR_IO: return "I/O error";
    case UHA02_ERR_NOMEM: return "Out of memory";
    case UHA02_ERR_MAGIC: return "No UHARC 0.2 archive";
    case UHA02_ERR_VERSION: return "Unsupported version";
    case UHA02_ERR_CRC: return "CRC failure";
    case UHA02_ERR_METHOD: return "Unknown method";
    case UHA02_ERR_DATA: return "Data error";
    default: return "Unknown error";
    }
}

/* ---------- I/O helpers for decoder ---------- */
typedef struct file_reader_s {
    FILE *fp;
    unsigned long remaining;
} file_reader;

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
    mw->crc = uha02_crc32(buf, len, mw->crc);
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

/* decode one table: read cmp_len bytes at off, decrypt with a fresh
 * 0.2 default-password S-box, then decode from memory. crc_init is the
 * incoming cumulative contents CRC (FFFFFFFF before the first table);
 * crc_out receives the chained value. Single cumulative contents CRC
 * over all four decoded tables concatenated: dword_3BAEC is reset to
 * -1 once before tab0 and checked (~crc == contents) after tab3. */
static int decode_table(FILE *fp, long off, unsigned long cmp_size,
                        int method, unsigned char *out,
                        unsigned long dec_size, unsigned long crc_init,
                        unsigned long *crc_out)
{
    mem_reader mr;
    mem_writer mw;
    unsigned char *cmp = 0;
    unsigned long cmp_len;
    uha02_crypt cr;
    int rc;
    size_t got;
    if (fseek(fp, off, SEEK_SET) != 0) return UHA02_ERR_IO;
    cmp_len = (method == 0) ? dec_size : cmp_size;
    if (cmp_len == 0 && dec_size == 0) {
        if (crc_out) *crc_out = crc_init;
        return UHA02_OK;
    }
    cmp = (unsigned char *)malloc(cmp_len ? cmp_len : 1);
    if (!cmp) return UHA02_ERR_NOMEM;
    got = fread(cmp, 1, cmp_len, fp);
    if (got != cmp_len) {
        free(cmp);
        return UHA02_ERR_IO;
    }
    /* fresh 0.2 default-password cipher per table (sub_108C8(1)),
     * in place over the compressed bytes */
    uha02_crypt_init(&cr);
    uha02_crypt_update(&cr, cmp, cmp_len);
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
        if (rc == 2) return UHA02_ERR_NOMEM;
        return UHA02_ERR_DATA;
    }
    if (mw.pos != dec_size) return UHA02_ERR_DATA;
    if (crc_out) *crc_out = mw.crc;
    return UHA02_OK;
}

unsigned long uha02_nfiles(uha02_t *h)
{
    return h ? h->nfiles : 0;
}

const uha02_entry *uha02_entry_get(uha02_t *h, unsigned long idx)
{
    if (!h || idx >= h->nfiles) return 0;
    return &h->entries[idx];
}

const uha02_header *uha02_header_get(uha02_t *h)
{
    return h ? &h->hdr : 0;
}

int uha02_open(const char *path, uha02_t **ph)
{
    FILE *fp;
    unsigned char raw[48];
    uha02_header hdr;
    uha02_t *h;
    int rc;
    long tab0_off, tab1_off, tab2_off, tab3_off;
    unsigned long nfiles, ndirs;
    unsigned char *tab0 = 0, *tab1 = 0, *tab2 = 0, *tab3 = 0;
    unsigned long crc;
    unsigned long i;

    if (!path || !ph) return UHA02_ERR_IO;
    *ph = 0;
    fp = fopen(path, "rb");
    if (!fp) return UHA02_ERR_IO;
    if (fread(raw, 1, 48, fp) != 48) {
        fclose(fp);
        return UHA02_ERR_IO;
    }
    rc = uha02_header_parse(raw, &hdr);
    if (rc != UHA02_OK) {
        fclose(fp);
        return rc;
    }
    nfiles = hdr.nfiles;
    ndirs = hdr.ndirs;
    if (nfiles > 1000000UL || ndirs > 100000UL) {
        fclose(fp);
        return UHA02_ERR_DATA;
    }
    h = (uha02_t *)calloc(1, sizeof(*h));
    if (!h) {
        fclose(fp);
        return UHA02_ERR_NOMEM;
    }
    h->fp = fp;
    h->hdr = hdr;
    h->nfiles = nfiles;
    h->path = (char *)malloc(strlen(path) + 1);
    if (h->path) strcpy(h->path, path);

    /* solid layout: file data at 48..48+data_len, then tables.
     * tab0 STORE 4*N, tab1 method 7 names_len, tab2 method 2 4*N,
     * tab3 method 7 7*N. Expected file size:
     * 48+data_len+4*N+cmp1+cmp2+cmp3 (sample: 48+11656+28+65+28+35). */
    h->data_off = 48L;
    tab0_off = 48L + (long)hdr.data_len;
    tab1_off = tab0_off + (long)(4 * nfiles);
    tab2_off = tab1_off + (long)hdr.cmp_sz1;
    tab3_off = tab2_off + (long)hdr.cmp_sz2;
    h->tab0_off = tab0_off;
    h->tab1_off = tab1_off;
    h->tab2_off = tab2_off;
    h->tab3_off = tab3_off;

    if (nfiles == 0) {
        *ph = h;
        return UHA02_OK;
    }

    tab0 = (unsigned char *)malloc(4 * (nfiles ? nfiles : 1));
    tab1 = (unsigned char *)malloc(hdr.names_len ? hdr.names_len : 1);
    tab2 = (unsigned char *)malloc(4 * (nfiles ? nfiles : 1));
    tab3 = (unsigned char *)malloc(7 * (nfiles ? nfiles : 1));
    if (!tab0 || !tab1 || !tab2 || !tab3) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return UHA02_ERR_NOMEM;
    }

    /* cumulative contents CRC chained across all four decoded tables */
    rc = decode_table(fp, tab0_off, 4 * nfiles, 0,
                      tab0, 4 * nfiles, 0xFFFFFFFFUL, &crc);
    if (rc != UHA02_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return rc;
    }
    rc = decode_table(fp, tab1_off, hdr.cmp_sz1, 7,
                      tab1, hdr.names_len, crc, &crc);
    if (rc != UHA02_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return rc;
    }
    rc = decode_table(fp, tab2_off, hdr.cmp_sz2, 2,
                      tab2, 4 * nfiles, crc, &crc);
    if (rc != UHA02_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return rc;
    }
    rc = decode_table(fp, tab3_off, hdr.cmp_sz3, 7,
                      tab3, 7 * nfiles, crc, &crc);
    if (rc != UHA02_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return rc;
    }
    if ((~crc & 0xFFFFFFFFUL) != (hdr.contents_crc & 0xFFFFFFFFUL)) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return UHA02_ERR_CRC;
    }

    /* rebuild 19-byte entries: tab1 = ndirs+N pascal strings (dirs
     * first, then files), tab2 = N u16 + N u16, tab3 = 2*N dir bytes
     * + 4*N size bytes + 1*N method bytes, tab0 = N u32 file CRCs. */
    h->entries = (uha02_entry *)calloc(nfiles ? nfiles : 1,
                                      sizeof(uha02_entry));
    if (!h->entries) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha02_close(h);
        return UHA02_ERR_NOMEM;
    }
    {
        unsigned long p1 = 0, p2 = 0;
        unsigned long dirs_total = 0, strs_total = 0;
        unsigned k;
        unsigned long *dir_offs = 0;
        char *sp = 0;

        for (k = 0; k < ndirs; k++) {
            unsigned l;
            if (p1 >= hdr.names_len) break;
            l = tab1[p1++];
            if (p1 + l > hdr.names_len) break;
            p1 += l;
            dirs_total += l + 1;
        }
        for (i = 0; i < nfiles; i++) {
            unsigned l;
            if (p1 >= hdr.names_len) break;
            l = tab1[p1++];
            if (p1 + l > hdr.names_len) break;
            p1 += l;
            strs_total += l + 1;
        }
        h->strpool = (char *)malloc(dirs_total + strs_total +
                                    nfiles * 260 + 1024);
        if (!h->strpool) {
            free(tab0); free(tab1); free(tab2); free(tab3);
            uha02_close(h);
            return UHA02_ERR_NOMEM;
        }
        dir_offs = (unsigned long *)malloc(sizeof(unsigned long) *
                                           (ndirs ? ndirs : 1));
        if (!dir_offs && ndirs) {
            free(tab0); free(tab1); free(tab2); free(tab3);
            uha02_close(h);
            return UHA02_ERR_NOMEM;
        }
        p1 = 0; p2 = 0;
        for (k = 0; k < ndirs; k++) {
            unsigned l = tab1[p1++];
            dir_offs[k] = p2;
            memcpy(h->strpool + p2, tab1 + p1, l);
            h->strpool[p2 + l] = 0;
            p1 += l; p2 += l + 1;
        }
        sp = h->strpool + dirs_total;
        for (i = 0; i < nfiles; i++) {
            uha02_entry *e = &h->entries[i];
            unsigned dd, tt, w0;
            unsigned char m;
            unsigned long sz, c32;
            /* tab2 plane order (verified in Uharc02.c: compress side
             * writes entry+13 first, entry+11 second; extract side
             * stores first plane to +13, second to +11; +13 is time
             * (dos_setftime arg order), +11 is date - same as 0.6). */
            tt = (unsigned)(tab2[2 * i] | (tab2[2 * i + 1] << 8));
            dd = (unsigned)(tab2[2 * nfiles + 2 * i] |
                            (tab2[2 * nfiles + 2 * i + 1] << 8));
            w0 = (unsigned)(tab3[i] | (tab3[nfiles + i] << 8));
            sz = (unsigned long)(tab3[2 * nfiles + i] |
                                 (tab3[3 * nfiles + i] << 8) |
                                 (tab3[4 * nfiles + i] << 16) |
                                 (tab3[5 * nfiles + i] << 24));
            m = tab3[6 * nfiles + i];
            c32 = (unsigned long)(tab0[4 * i] | (tab0[4 * i + 1] << 8) |
                                  (tab0[4 * i + 2] << 16) |
                                  (tab0[4 * i + 3] << 24));
            e->time = tt; e->date = dd;
            e->dir_index = w0; e->method = m;
            /* raw var (negative cumulative, like 0.6/0.4).
             * Converted to sizes below via differences. */
            e->size = sz & 0xFFFFFFFFUL; e->crc = c32;
            e->offset = 0;
            e->dir = ""; e->name = ""; e->path = "";
        }
        /* entry+3 on disk is delta (prev_size - curr_size, like
         * 0.6/0.4). Recover sizes iteratively: size_0 = -var_0,
         * size_i = size_{i-1} - var_i (32-bit wrap). Offsets follow
         * cumulatively. NOTE: sum need not equal total_size exactly
         * (filters/padding); open stays permissive on mismatch. */
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
        {
            unsigned long q = 0;
            unsigned kk;
            for (kk = 0; kk < ndirs; kk++) {
                unsigned l = tab1[q++];
                q += l;
            }
            for (kk = 0; kk < nfiles; kk++) {
                unsigned l = tab1[q++];
                uha02_entry *e = &h->entries[kk];
                memcpy(sp, tab1 + q, l);
                sp[l] = 0;
                q += l;
                e->name = sp;
                sp += l + 1;
                if ((e->dir_index & 0xFFFFU) < ndirs)
                    e->dir = h->strpool + dir_offs[e->dir_index & 0xFFFFU];
                else
                    e->dir = "";
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
        free(dir_offs);
    }

    free(tab0); free(tab1); free(tab2); free(tab3);
    *ph = h;
    return UHA02_OK;
}

void uha02_close(uha02_t *h)
{
    if (!h) return;
    if (h->fp) fclose(h->fp);
    free(h->path);
    free(h->entries);
    free(h->strpool);
    free(h);
}

/* ---- solid-stream splitter (mirrors sub_4051A2 / uharc06.c) ----
 * Single-threaded globals pointing at extract() locals. */
static uha02_t *g_sp_h;
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
    uha02_t *h = g_sp_h;
    unsigned long i = *g_sp_pidx;
    const uha02_entry *e = &h->entries[i];
    char *dst = g_sp_curpath;
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
    return 0;
}

/* returns 0 ok, 1 io/error (decoder maps non-zero to err 4; CRC flag distinguishes) */
static int uha02_split_write(void *ctx, const unsigned char *buf, unsigned len)
{
    uha02_t *h = g_sp_h;
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
            if (chunk > 0) {
                size_t w = fwrite(buf, 1, (size_t)chunk, *g_sp_pcur);
                if (w != (size_t)chunk) return 1;
                *g_sp_pcrc = uha02_crc32(buf, (unsigned)chunk, *g_sp_pcrc);
            }
        } else {
            if (chunk > 0)
                *g_sp_pcrc = uha02_crc32(buf, (unsigned)chunk, *g_sp_pcrc);
        }
        buf += chunk;
        len -= (unsigned)chunk;
        *g_sp_prem -= chunk;
        if (*g_sp_prem == 0) {
            ci = *g_sp_pidx;
            if (*g_sp_pcur) { fclose(*g_sp_pcur); *g_sp_pcur = 0; }
            if (((~(*g_sp_pcrc)) & 0xFFFFFFFFUL) !=
                (h->entries[ci].crc & 0xFFFFFFFFUL)) {
                g_sp_crc_err = 1;
                return 1;
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

int uha02_extract(uha02_t *h, const char *outdir,
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
    int rc = UHA02_OK;
    unsigned long k;

    if (!h || !h->fp) return UHA02_ERR_IO;
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

    /* TODO skip marker method 0xFF: fail fast if size>0 */
    for (k = 0; k < h->nfiles; k++) {
        if (h->entries[k].method == 0xFF &&
            (h->entries[k].size & 0xFFFFFFFFUL) != 0) {
            rc = UHA02_ERR_DATA;
            goto done;
        }
    }

    /* skip leading empties: create empty file unless test, crc must be 0.
     * Do not consume input for them. */
    while (idx < h->nfiles &&
           (h->entries[idx].size & 0xFFFFFFFFUL) == 0) {
        if ((h->entries[idx].crc & 0xFFFFFFFFUL) != 0) {
            rc = UHA02_ERR_CRC;
            goto done;
        }
        if (!test_only) {
            if (g_sp_open_cur() != 0) { rc = UHA02_ERR_IO; goto done; }
            if (cur) { fclose(cur); cur = 0; }
        }
        idx++;
    }
    if (idx < h->nfiles) {
        remaining = h->entries[idx].size;
        if (g_sp_open_cur() != 0) { rc = UHA02_ERR_IO; goto done; }
        if (test_only) cur = 0;
    } else {
        remaining = 0;
    }

    /* packed file-data range: 48 .. 48+data_len.
     * File data is NOT default-encrypted (like 0.6: the file stream
     * uses plain input when no -pw password is set).
     * TODO password archives: decrypt with user password when set. */
    packed_len = h->hdr.data_len;
    if (fseek(fp, 48L, SEEK_SET) != 0) { rc = UHA02_ERR_IO; goto done; }
    packed = (unsigned char *)malloc(packed_len ? packed_len : 1);
    if (!packed) { rc = UHA02_ERR_NOMEM; goto done; }
    got = fread(packed, 1, packed_len, fp);
    if (got != packed_len) { rc = UHA02_ERR_IO; goto done; }

    /* TODO filters (0.2 exe filter sub_150A4, multimedia sub_15AD0,
     * gated by flags). 0.2 vectors need analysis; currently raw
     * decompressed bytes are split as-is. */
    mr.buf = packed;
    mr.len = packed_len;
    mr.pos = 0;
    drc = uharc_dec_decode((int)h->hdr.method, mr_read, &mr,
                           uha02_split_write, 0, h->hdr.total_size);
    if (g_sp_crc_err) rc = UHA02_ERR_CRC;
    else if (drc == 2) rc = UHA02_ERR_NOMEM;
    else if (drc != 0) rc = UHA02_ERR_DATA;
    else rc = UHA02_OK;
    if (rc != UHA02_OK) goto done;
    /* all files must be accounted for */
    if (idx != h->nfiles) { rc = UHA02_ERR_DATA; goto done; }

done:
    if (packed) free(packed);
    if (cur) { fclose(cur); cur = 0; }
    g_sp_h = 0;
    return rc;
}
