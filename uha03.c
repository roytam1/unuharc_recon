/* UHARC 0.3 container: header, directory, per-file extraction - C89
 * Reverse engineered from uharc_u.c / uharc_u.lst (UHARC 0.3, full archiver).
 * Magic "UHA\x03", 44-byte header, 4 tables (STORE/ALZ/Huffman), per-file. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uha03.h"
#include "uharc_dec.h"
#include "uharc_split.h"
#include "uharc_filter.h"
/* Crypt reuses uhcrypt04 (byte-identical to uhcrypt06 per test;
 * 0.3 assumed same - TODO verify via directory CRC once 03 cores work;
 * split to uhcrypt03.* if divergence found). */
#include "uhcrypt04.h"

static unsigned long crc_tab[256];
static int crc_init = 0;

static void crc_make(void)
{
    unsigned i, j;
    unsigned long c;
    if (crc_init) return;
    for (i = 0; i < 256; i++) {
        c = i;
        for (j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320UL ^ (c >> 1);
            else c >>= 1;
        }
        crc_tab[i] = c;
    }
    crc_init = 1;
}

unsigned long uha03_crc32(const unsigned char *buf, unsigned len,
                          unsigned long crc)
{
    unsigned i;
    crc_make();
    for (i = 0; i < len; i++)
        crc = crc_tab[(unsigned char)(crc ^ buf[i])] ^ (crc >> 8);
    return crc;
}

void uha03_header_decrypt(unsigned char hdr[44])
{
    unsigned char key;
    unsigned i;
    key = hdr[42];
    for (i = 0; i < 36; i++) {
        hdr[4 + i] ^= (unsigned char)((key + i * i) & 0xFF);
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

static unsigned long rd32le(const unsigned char *p)
{
    return (unsigned long)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

int uha03_header_parse(const unsigned char raw[44],
                       uha03_header *h)
{
    unsigned char b[44];
    unsigned long crc;
    unsigned i;
    memcpy(b, raw, 44);
    if (!(b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x41 && b[3] == 0x03))
        return UHA03_ERR_MAGIC;
    uha03_header_decrypt(b);
    crc_make();
    crc = 0xFFFFFFFFUL;
    for (i = 4; i < 4 + 36; i++)
        crc = crc_tab[(unsigned char)(crc ^ b[i])] ^ (crc >> 8);
    if (((unsigned)(~crc & 0xFFFFUL)) != rd16le(b + 42))
        return UHA03_ERR_CRC;
    h->magic = 0x03414855UL;
    h->dostime = rd16le(b + 4);
    h->dosdate = rd16le(b + 6);
    h->flags_pw = b[8];
    h->flags_filt = b[9];
    h->unk_dfa = b[10];
    h->ndirs = rd16le(b + 11);
    h->unk_dfd = rd32le(b + 13);
    h->base_e01 = rd32le(b + 17);
    h->nfiles = rd24le(b + 21);
    h->names_len = rd24le(b + 24);
    h->cmp_sz1 = rd24le(b + 27);
    h->cmp_sz2 = rd24le(b + 30);
    h->cmp_sz3 = rd24le(b + 33);
    h->crc1 = rd16le(b + 36);
    h->crc2 = rd16le(b + 38);
    h->crc3 = rd16le(b + 40);
    h->hdr_crc = rd16le(b + 42);
    return UHA03_OK;
}

struct uha03_s {
    FILE *fp;
    char *path;
    uha03_header hdr;
    unsigned long nfiles;
    uha03_entry *entries;
    char *strpool;
    long data_start;
    long tab0_off;
    long tab1_off;
    long tab2_off;
    long tab3_off;
    char *pwd; /* heap copy of -pw password, NULL when none given */
};

const char *uha03_strerror(int code)
{
    switch (code) {
    case UHA03_OK: return "OK";
    case UHA03_ERR_IO: return "I/O error";
    case UHA03_ERR_NOMEM: return "Out of memory";
    case UHA03_ERR_MAGIC: return "No UHARC 0.3 archive";
    case UHA03_ERR_VERSION: return "Unsupported version";
    case UHA03_ERR_CRC: return "CRC failure";
    case UHA03_ERR_METHOD: return "Unknown method";
    case UHA03_ERR_DATA: return "Data error";
    case UHA03_ERR_PASSWORD: return "Password required";
    default: return "Unknown error";
    }
}

typedef struct fr_s {
    FILE *fp;
    unsigned long left;
} fr_t;

static int fr_read(void *ctx, unsigned char *buf, unsigned want)
{
    fr_t *f = (fr_t *)ctx;
    size_t n;
    if (f->left == 0) return 0;
    if (want > f->left) want = (unsigned)f->left;
    n = fread(buf, 1, want, f->fp);
    f->left -= (unsigned long)n;
    return (int)n;
}

typedef struct mw_s {
    unsigned char *buf;
    unsigned long cap;
    unsigned long pos;
    unsigned long crc;
} mw_t;

static int mw_write(void *ctx, const unsigned char *buf, unsigned len)
{
    mw_t *m = (mw_t *)ctx;
    if (m->pos + len > m->cap) return 1;
    memcpy(m->buf + m->pos, buf, len);
    m->crc = uha03_crc32(buf, len, m->crc);
    m->pos += len;
    return 0;
}

typedef struct mr_s {
    const unsigned char *buf;
    unsigned long len;
    unsigned long pos;
} mr_t;

static int mr_read(void *ctx, unsigned char *buf, unsigned want)
{
    mr_t *m = (mr_t *)ctx;
    unsigned long avail = m->len - m->pos;
    if (avail == 0) return 0;
    if (want > avail) want = (unsigned)avail;
    memcpy(buf, m->buf + m->pos, want);
    m->pos += want;
    return (int)want;
}

/* crc_init: incoming CRC. 0.3 tab1_crc is cumulative over tab0+tab1
 * (verified: 03_m0 tab0+tab1 -> BD63 == crc1); tab2/tab3 are standalone. */
static int decode_table(FILE *fp, long off, unsigned long cmp_len,
                        int method, unsigned char *out,
                        unsigned long dec_len, unsigned long crc_seed,
                        unsigned long *crc_out, const char *pwd)
{
    mr_t mr;
    mw_t mw;
    unsigned char *cmp = 0;
    unsigned long clen;
    uhcrypt04_t *cr = 0;
    int rc;
    size_t got;
    if (fseek(fp, off, SEEK_SET) != 0) return UHA03_ERR_IO;
    clen = (method == 0) ? dec_len : cmp_len;
    if (clen == 0 && dec_len == 0) {
        if (crc_out) *crc_out = crc_seed;
        return UHA03_OK;
    }
    cmp = (unsigned char *)malloc(clen ? clen : 1);
    if (!cmp) return UHA03_ERR_NOMEM;
    got = fread(cmp, 1, clen, fp);
    if (got != clen) {
        free(cmp);
        return UHA03_ERR_IO;
    }
    cr = pwd ? uhcrypt04_init_pw(pwd) : uhcrypt04_init();
    if (!cr) {
        free(cmp);
        return UHA03_ERR_NOMEM;
    }
    uhcrypt04_update(cr, cmp, clen);
    uhcrypt04_free(cr);
    mr.buf = cmp;
    mr.len = clen;
    mr.pos = 0;
    mw.buf = out;
    mw.cap = dec_len;
    mw.pos = 0;
    mw.crc = crc_seed;
    rc = uharc_dec_decode_v(method, 1, 3, mr_read, &mr, mw_write, &mw, dec_len);
    free(cmp);
    if (rc != 0) {
        if (rc == 2) return UHA03_ERR_NOMEM;
        return UHA03_ERR_DATA;
    }
    if (mw.pos != dec_len) return UHA03_ERR_DATA;
    if (crc_out) *crc_out = mw.crc;
    return UHA03_OK;
}

unsigned long uha03_nfiles(uha03_t *h)
{
    return h ? h->nfiles : 0;
}

const uha03_entry *uha03_entry_get(uha03_t *h, unsigned long idx)
{
    if (!h || idx >= h->nfiles) return 0;
    return &h->entries[idx];
}

const uha03_header *uha03_header_get(uha03_t *h)
{
    return h ? &h->hdr : 0;
}

int uha03_open(const char *path, uha03_t **ph)
{
    return uha03_open_pw(path, ph, 0);
}

int uha03_open_pw(const char *path, uha03_t **ph, const char *pwd)
{
    FILE *fp;
    unsigned char raw[44];
    uha03_header hdr;
    uha03_t *h;
    int rc;
    long tab0_off, tab1_off, tab2_off, tab3_off;
    unsigned long nfiles, ndirs;
    unsigned char *tab0 = 0, *tab1 = 0, *tab2 = 0, *tab3 = 0;
    unsigned long crc;
    unsigned long i;
    int have_pw;
    int use_pw;
    const char *tpwd;

    if (!path || !ph) return UHA03_ERR_IO;
    *ph = 0;
    fp = fopen(path, "rb");
    if (!fp) return UHA03_ERR_IO;
    if (fread(raw, 1, 44, fp) != 44) {
        fclose(fp);
        return UHA03_ERR_IO;
    }
    rc = uha03_header_parse(raw, &hdr);
    if (rc != UHA03_OK) {
        fclose(fp);
        return rc;
    }
    nfiles = hdr.nfiles;
    ndirs = hdr.ndirs;
    if (nfiles > 1000000UL || ndirs > 100000UL) {
        fclose(fp);
        return UHA03_ERR_DATA;
    }
    /* -pw protection (flags_pw bit 1 = encrypted tables and stream).
     * A protected archive without a password cannot be opened. */
    have_pw = (pwd && *pwd) ? 1 : 0;
    use_pw = (hdr.flags_pw & 2) ? 1 : 0;
    tpwd = (have_pw && use_pw) ? pwd : 0;
    if (use_pw && !have_pw) {
        fclose(fp);
        return UHA03_ERR_PASSWORD;
    }
    h = (uha03_t *)calloc(1, sizeof(*h));
    if (!h) {
        fclose(fp);
        return UHA03_ERR_NOMEM;
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
            uha03_close(h);
            return UHA03_ERR_NOMEM;
        }
        strcpy(h->pwd, pwd);
    }

    /* layout: file data at 44..base_e01, tables at base_e01+44..end
     * tab0 STORE 4*N, tab1 method17 names_len, tab2 method2 4*N,
     * tab3 method7 7*N. See sub_4060D0 / sub_4058E0. */
    tab0_off = (long)(hdr.base_e01 + 44);
    tab1_off = tab0_off + (long)(4 * nfiles);
    tab2_off = tab1_off + (long)hdr.cmp_sz1;
    tab3_off = tab2_off + (long)hdr.cmp_sz2;
    h->data_start = 44;
    h->tab0_off = tab0_off;
    h->tab1_off = tab1_off;
    h->tab2_off = tab2_off;
    h->tab3_off = tab3_off;

    if (nfiles == 0) {
        *ph = h;
        return UHA03_OK;
    }

    tab0 = (unsigned char *)malloc(4 * (nfiles ? nfiles : 1));
    tab1 = (unsigned char *)malloc(hdr.names_len ? hdr.names_len : 1);
    tab2 = (unsigned char *)malloc(4 * (nfiles ? nfiles : 1));
    tab3 = (unsigned char *)malloc(7 * (nfiles ? nfiles : 1));
    if (!tab0 || !tab1 || !tab2 || !tab3) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return UHA03_ERR_NOMEM;
    }

    rc = decode_table(fp, tab0_off, 4 * nfiles, 0,
                      tab0, 4 * nfiles, 0xFFFFFFFFUL, &crc, tpwd);
    if (rc != UHA03_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return rc;
    }
    /* crc1 is cumulative over tab0+tab1 */
    rc = decode_table(fp, tab1_off, hdr.cmp_sz1, 17,
                      tab1, hdr.names_len, crc, &crc, tpwd);
    if (rc != UHA03_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return rc;
    }
    if (((unsigned)(~crc & 0xFFFFUL)) != (hdr.crc1 & 0xFFFFU)) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return UHA03_ERR_CRC;
    }
    rc = decode_table(fp, tab2_off, hdr.cmp_sz2, 2,
                      tab2, 4 * nfiles, 0xFFFFFFFFUL, &crc, tpwd);
    if (rc != UHA03_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return rc;
    }
    if (((unsigned)(~crc & 0xFFFFUL)) != (hdr.crc2 & 0xFFFFU)) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return UHA03_ERR_CRC;
    }
    rc = decode_table(fp, tab3_off, hdr.cmp_sz3, 7,
                      tab3, 7 * nfiles, 0xFFFFFFFFUL, &crc, tpwd);
    if (rc != UHA03_OK) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return rc;
    }
    if (((unsigned)(~crc & 0xFFFFUL)) != (hdr.crc3 & 0xFFFFU)) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return UHA03_ERR_CRC;
    }

    /* rebuild entries: tab1 = ndirs+N pascal strings,
     * tab2 = N u16 time + N u16 date,
     * tab3 = 2*N dir bytes + 4*N size bytes + 1*N method bytes.
     * tab0 = N u32 file CRCs. Entry layout matches 0.6 (19 bytes). */
    h->entries = (uha03_entry *)calloc(nfiles ? nfiles : 1,
                                      sizeof(uha03_entry));
    if (!h->entries) {
        free(tab0); free(tab1); free(tab2); free(tab3);
        uha03_close(h);
        return UHA03_ERR_NOMEM;
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
        h->strpool = (char *)malloc(dirs_total + strs_total + nfiles * 260 + 1024);
        if (!h->strpool) {
            free(tab0); free(tab1); free(tab2); free(tab3);
            uha03_close(h);
            return UHA03_ERR_NOMEM;
        }
        dir_offs = (unsigned long *)malloc(sizeof(unsigned long) * (ndirs ? ndirs : 1));
        if (!dir_offs && ndirs) {
            free(tab0); free(tab1); free(tab2); free(tab3);
            uha03_close(h);
            return UHA03_ERR_NOMEM;
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
            uha03_entry *e = &h->entries[i];
            unsigned dd, tt, w0;
            unsigned char m;
            unsigned long sz, c32;
            /* NOTE 0.3 tab2 plane order is dates-first (unlike 0.6
             * time-first). First N u16 = dosdate, next N u16 = dostime.
             * TODO verify against decoded tables once core2 works
             * (all test-corpus files share one date/time so order
             * is not observable there; use uha03.UHA with mixed
             * 2001..2010 dates). */
            dd = (unsigned)(tab2[2 * i] | (tab2[2 * i + 1] << 8));
            tt = (unsigned)(tab2[2 * nfiles + 2 * i] |
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
            /* raw var (negative cumulative, like 0.6 sub_403C28).
             * Converted to sizes below via differences. */
            e->size = sz & 0xFFFFFFFFUL; e->crc = c32;
            e->offset = 0;
            e->dir = ""; e->name = ""; e->path = "";
        }
        /* entry+3 on disk is delta (prev_size - curr_size, like 0.6).
         * Recover sizes iteratively: size_0 = -var_0,
         * size_i = size_{i-1} - var_i (32-bit wrap). */
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
                uha03_entry *e = &h->entries[kk];
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
        /* offsets already set during size conversion above. */
        free(dir_offs);
    }

    free(tab0); free(tab1); free(tab2); free(tab3);
    *ph = h;
    return UHA03_OK;
}

void uha03_close(uha03_t *h)
{
    if (!h) return;
    if (h->fp) fclose(h->fp);
    free(h->path);
    free(h->pwd);
    free(h->entries);
    free(h->strpool);
    free(h);
}

typedef struct u3_mr_s {
    const unsigned char *b;
    unsigned long n, p;
} u3_mr;

static int u3_read(void *ctx, unsigned char *buf, unsigned want)
{
    u3_mr *m = (u3_mr *)ctx;
    unsigned long avail = m->n - m->p;
    if (!avail) return 0;
    if (want > avail) want = (unsigned)avail;
    memcpy(buf, m->b + m->p, want);
    m->p += want;
    return (int)want;
}

/* header +9 flags_filt bit0 is the EXE filter switch (original:
 * byte_436DE1 & 1 -> byte_428B8A); the splitter additionally requires
 * each file to be at least 0x400 bytes, and the filter checks MZ/ZM. */
static void uha03_unfilter(unsigned char *buf, unsigned long len)
{
    uharc_filter_exe(buf, len, 3, 0);
}

int uha03_extract(uha03_t *h, const char *outdir,
                  int with_paths, int test_only)
{
    unsigned char *packed = 0;
    uharc_split_entry *se = 0;
    u3_mr mr;
    unsigned long i, total = 0;
    size_t got;
    int rc;
    uhcrypt04_t *scr = 0;

    if (!h || !h->fp) return UHA03_ERR_IO;
    if (!h->nfiles) return UHA03_OK;

    packed = (unsigned char *)malloc(h->hdr.base_e01 ? h->hdr.base_e01 : 1);
    se = (uharc_split_entry *)calloc(h->nfiles, sizeof(*se));
    if (!packed || !se) {
        free(packed); free(se);
        return UHA03_ERR_NOMEM;
    }
    if (fseek(h->fp, 44L, SEEK_SET) != 0) {
        free(packed); free(se);
        return UHA03_ERR_IO;
    }
    got = fread(packed, 1, h->hdr.base_e01, h->fp);
    if (got != h->hdr.base_e01) {
        free(packed); free(se);
        return UHA03_ERR_IO;
    }
    /* -pw archives (flags_pw bit 1) transform the packed stream with a
     * fresh password S-box, like the tables. */
    if ((h->hdr.flags_pw & 2) && h->pwd) {
        scr = uhcrypt04_init_pw(h->pwd);
        if (!scr) {
            free(packed); free(se);
            return UHA03_ERR_NOMEM;
        }
        uhcrypt04_update(scr, packed, h->hdr.base_e01);
        uhcrypt04_free(scr);
        scr = 0;
    }
    for (i = 0; i < h->nfiles; i++) {
        se[i].name = h->entries[i].name;
        se[i].path = h->entries[i].path;
        se[i].size = h->entries[i].size & 0xFFFFFFFFUL;
        se[i].crc = h->entries[i].crc & 0xFFFFFFFFUL;
        total += se[i].size;
    }
    mr.b = packed; mr.n = h->hdr.base_e01; mr.p = 0;
    rc = uharc_split_run(outdir, with_paths, test_only, se, h->nfiles,
                         (int)h->hdr.unk_dfa, 3,
                         (h->hdr.flags_filt & 1) ? uha03_unfilter : 0,
                         u3_read, &mr, total);
    free(packed);
    free(se);
    switch (rc) {
    case USPLIT_OK: return UHA03_OK;
    case USPLIT_ERR_NOMEM: return UHA03_ERR_NOMEM;
    case USPLIT_ERR_CRC: return UHA03_ERR_CRC;
    case USPLIT_ERR_IO: return UHA03_ERR_IO;
    default: return UHA03_ERR_DATA;
    }
}
