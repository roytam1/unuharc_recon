/* test directory table decompression (method 17, core3) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uharc06.h"
#include "uharc_dec.h"

typedef struct mem_reader_s {
    const unsigned char *p;
    unsigned long len;
    unsigned long pos;
} mem_reader;

static int mr_read(void *ctx, unsigned char *buf, unsigned want)
{
    mem_reader *m = (mem_reader *)ctx;
    unsigned long avail = m->len - m->pos;
    if (avail == 0) return 0;
    if (want > avail) want = (unsigned)avail;
    memcpy(buf, m->p + m->pos, want);
    m->pos += want;
    return (int)want;
}

typedef struct mem_writer_s {
    unsigned char *p;
    unsigned long len;
    unsigned long pos;
    unsigned long crc;
} mem_writer;

static int mw_write(void *ctx, const unsigned char *buf, unsigned len)
{
    mem_writer *m = (mem_writer *)ctx;
    if (m->pos + len > m->len) return 1;
    memcpy(m->p + m->pos, buf, len);
    m->crc = uharc06_crc32(buf, len, m->crc);
    m->pos += len;
    return 0;
}

int main(void)
{
    FILE *fp;
    unsigned char raw[39];
    uharc06_header h;
    int rc;
    unsigned char *cmp1, *dec1, *cmp2, *dec2;
    long tab1_off, tab2_off;
    mem_reader mr;
    mem_writer mw;

    fp = fopen("D3X.UHA", "rb");
    if (!fp) { printf("open fail\n"); return 1; }
    if (fread(raw, 1, 39, fp) != 39) { printf("read fail\n"); return 1; }
    rc = uharc06_header_parse(raw, &h);
    if (rc != 0) { printf("header parse %d\n", rc); return 1; }
    printf("header OK nfiles=%lu tab1 dec=%lu cmp=%lu crc=%04X tab2 cmp=%lu crc=%04X\n",
        h.nfiles, h.tab1_dec_size, h.tab1_cmp_size, h.tab1_crc,
        h.tab2_cmp_size, h.tab2_crc);

    tab1_off = (long)(h.data_off + 39 + 4 * h.nfiles);
    tab2_off = tab1_off + (long)h.tab1_cmp_size;

    cmp1 = (unsigned char *)malloc(h.tab1_cmp_size ? h.tab1_cmp_size : 1);
    dec1 = (unsigned char *)malloc(h.tab1_dec_size ? h.tab1_dec_size : 1);
    cmp2 = (unsigned char *)malloc(h.tab2_cmp_size ? h.tab2_cmp_size : 1);
    dec2 = (unsigned char *)malloc(11 * h.nfiles);
    if (!cmp1 || !dec1 || !cmp2 || !dec2) { printf("nomem\n"); return 1; }

    fseek(fp, tab1_off, SEEK_SET);
    if (fread(cmp1, 1, h.tab1_cmp_size, fp) != h.tab1_cmp_size) {
        printf("tab1 read fail\n"); return 1;
    }
    fseek(fp, tab2_off, SEEK_SET);
    if (fread(cmp2, 1, h.tab2_cmp_size, fp) != h.tab2_cmp_size) {
        printf("tab2 read fail\n"); return 1;
    }
    fclose(fp);

    /* decode tab1 method 17 */
    mr.p = cmp1; mr.len = h.tab1_cmp_size; mr.pos = 0;
    mw.p = dec1; mw.len = h.tab1_dec_size; mw.pos = 0; mw.crc = 0xFFFFFFFFUL;
    rc = uharc_dec_decode(17, mr_read, &mr, mw_write, &mw, h.tab1_dec_size);
    printf("tab1 decode rc=%d pos=%lu/%lu crc=%08lX expect_crc_low=%04X got_low=%04X\n",
        rc, mw.pos, h.tab1_dec_size, mw.crc,
        h.tab1_crc, (unsigned)(~mw.crc & 0xFFFFUL));
    if (rc == 0 && mw.pos == h.tab1_dec_size) {
        unsigned i;
        printf("tab1 first 64: ");
        for (i = 0; i < 64 && i < h.tab1_dec_size; i++) {
            unsigned char c = dec1[i];
            if (c >= 32 && c < 127) printf("%c", c);
            else printf("\\x%02X", c);
        }
        printf("\n");
    }

    mr.p = cmp2; mr.len = h.tab2_cmp_size; mr.pos = 0;
    mw.p = dec2; mw.len = 11 * h.nfiles; mw.pos = 0; mw.crc = 0xFFFFFFFFUL;
    rc = uharc_dec_decode(17, mr_read, &mr, mw_write, &mw, 11 * h.nfiles);
    printf("tab2 decode rc=%d pos=%lu/%lu crc=%08lX expect_low=%04X got_low=%04X\n",
        rc, mw.pos, 11UL * h.nfiles, mw.crc,
        h.tab2_crc, (unsigned)(~mw.crc & 0xFFFFUL));

    free(cmp1); free(dec1); free(cmp2); free(dec2);
    return 0;
}
