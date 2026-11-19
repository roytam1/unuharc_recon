/* header test for UHARC 0.2: parse uha02.UHA header */
#include <stdio.h>
#include "uha02.h"

int main(int argc, char **argv)
{
    const char *path = "uha02.UHA";
    FILE *fp;
    unsigned char raw[48];
    uha02_header h;
    int rc;
    if (argc > 1) path = argv[1];
    fp = fopen(path, "rb");
    if (!fp) { printf("cannot open %s\n", path); return 1; }
    if (fread(raw, 1, 48, fp) != 48) { printf("short read\n"); return 1; }
    fclose(fp);
    printf("raw magic: %02X %02X %02X %02X\n", raw[0], raw[1], raw[2], raw[3]);
    rc = uha02_header_parse(raw, &h);
    printf("parse rc=%d (%s)\n", rc, uha02_strerror(rc));
    if (rc != 0) return 1;
    printf("reserved=%02X method=%u ndirs=%lu\n",
        h.reserved, h.method, (unsigned long)h.ndirs);
    printf("nfiles=%lu total=%lu data_len=%lu names_len=%lu\n",
        (unsigned long)h.nfiles, (unsigned long)h.total_size,
        (unsigned long)h.data_len, (unsigned long)h.names_len);
    printf("cmp1=%lu cmp2=%lu cmp3=%lu contents=%08lX hdr=%08lX\n",
        (unsigned long)h.cmp_sz1, (unsigned long)h.cmp_sz2,
        (unsigned long)h.cmp_sz3,
        (unsigned long)h.contents_crc, (unsigned long)h.hdr_crc);
    printf("time=%04X date=%04X\n", h.dostime, h.dosdate);
    return 0;
}
