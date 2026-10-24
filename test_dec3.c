/* Test uharc_dec3 against D3X.UHA directory tables (method 17). C89. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uharc_dec.h"
#include "uharc06.h"

int uharc_dec3_init(void **pctx, int method);
int uharc_dec3_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);
void uharc_dec3_free(void *ctx);

typedef struct {
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

typedef struct {
    unsigned char *buf;
    unsigned long cap;
    unsigned long pos;
} mw_t;

static int mw_write(void *ctx, const unsigned char *buf, unsigned len)
{
    mw_t *m = (mw_t *)ctx;
    if (m->pos + len > m->cap) return 1;
    memcpy(m->buf + m->pos, buf, len);
    m->pos += len;
    return 0;
}

static unsigned crc16_of(const unsigned char *p, unsigned long n)
{
    unsigned long c = uharc06_crc32(p, (unsigned)n, 0xFFFFFFFFUL);
    return (unsigned)(c & 0xFFFFUL);
}

static int run_table(const char *path, long off, unsigned long cmp,
                     unsigned long dec, unsigned expect_crc, const char *nm)
{
    FILE *fp;
    fr_t fr;
    mw_t mw;
    void *dec3 = 0;
    int rc;
    int ret = 1;
    fp = fopen(path, "rb");
    if (!fp) {
        printf("%s: cannot open %s\n", nm, path);
        return 1;
    }
    if (fseek(fp, off, SEEK_SET) != 0) {
        printf("%s: seek fail\n", nm);
        fclose(fp);
        return 1;
    }
    mw.cap = dec + 65536;
    mw.buf = (unsigned char *)malloc(mw.cap);
    if (!mw.buf) {
        fclose(fp);
        return 1;
    }
    mw.pos = 0;
    fr.fp = fp;
    fr.left = cmp;
    rc = uharc_dec3_init(&dec3, 17);
    if (rc != 0) {
        printf("%s: init rc=%d\n", nm, rc);
        free(mw.buf);
        fclose(fp);
        return 1;
    }
    rc = uharc_dec3_decode(dec3, fr_read, &fr, mw_write, &mw, dec);
    {
        unsigned long c32 = uharc06_crc32(mw.buf, mw.pos, 0xFFFFFFFFUL);
        unsigned inv = (unsigned)(~c32 & 0xFFFFUL);
        unsigned rawc = (unsigned)(c32 & 0xFFFFUL);
        unsigned long i;
        unsigned char *invb;
        unsigned long invc;
        printf("%s: cmp=%lu dec=%lu rc=%d out=%lu left=%lu crc16(~)=%04X raw=%04X expect=%04X %s\n",
               nm, cmp, dec, rc, mw.pos, fr.left, inv, rawc, expect_crc,
               (rc == 0 && mw.pos == dec && inv == expect_crc) ? "OK" : "FAIL");
        if (rc == 0 && mw.pos == dec && inv == expect_crc) ret = 0;
        invb = (unsigned char *)malloc(mw.pos ? mw.pos : 1);
        if (invb) {
            for (i = 0; i < mw.pos; i++) invb[i] = (unsigned char)(mw.buf[i] ^ 0xFF);
            invc = uharc06_crc32(invb, mw.pos, 0xFFFFFFFFUL);
            printf("  inverted-bytes crc16(~)=%04X\n", (unsigned)(~invc & 0xFFFFUL));
            free(invb);
        }
        printf("  first 32 out bytes:");
        for (i = 0; i < 32 && i < mw.pos; i++) printf(" %02X", mw.buf[i]);
        printf("\n");
    }
    uharc_dec3_free(dec3);
    {
        /* dump raw output for inspection */
        FILE *fo;
        char fn[64];
        sprintf(fn, "%s.bin", nm);
        fo = fopen(fn, "wb");
        if (fo) {
            fwrite(mw.buf, 1, mw.pos, fo);
            fclose(fo);
        }
    }
    free(mw.buf);
    fclose(fp);
    return ret;
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "D3X.UHA";
    FILE *fp;
    unsigned char raw[39];
    uharc06_header h;
    int rc;
    long tab0, tab1, tab2;
    long delta = 0;
    int fails = 0;
    if (argc > 3) delta = atol(argv[3]);
    fp = fopen(path, "rb");
    if (!fp) {
        printf("cannot open %s\n", path);
        return 1;
    }
    if (fread(raw, 1, 39, fp) != 39) {
        printf("short header\n");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    rc = uharc06_header_parse(raw, &h);
    if (rc != 0) {
        printf("header parse rc=%d\n", rc);
        return 1;
    }
    printf("method=%u ndirs=%u total=%lu data_off=%lu nfiles=%lu\n",
           h.method, h.ndirs, h.total_size, h.data_off, h.nfiles);
    printf("tab1 dec=%lu cmp=%lu crc=%04X | tab2 cmp=%lu crc=%04X hdr_crc=%04X\n",
           h.tab1_dec_size, h.tab1_cmp_size, h.tab1_crc,
           h.tab2_cmp_size, h.tab2_crc, h.hdr_crc);
    tab0 = (long)(h.data_off + 39);
    tab1 = tab0 + (long)(4 * h.nfiles) + delta;
    tab2 = tab1 + (long)h.tab1_cmp_size;
    fails += run_table(path, tab1, h.tab1_cmp_size, h.tab1_dec_size,
                       h.tab1_crc, "tab1");
    /* tab2 dec size is not in header; task says 495 */
    fails += run_table(path, tab2, h.tab2_cmp_size, 495, h.tab2_crc, "tab2");
    if (fails == 0) printf("ALL OK\n");
    else printf("FAILURES: %d\n", fails);
    return fails ? 1 : 0;
}
