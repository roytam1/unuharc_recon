/* header test for UHARC 0.3: parse uharc_u.uha header */
#include <stdio.h>
#include "uha03.h"

int main(int argc, char **argv)
{
    const char *path = "uharc_u.uha";
    FILE *fp;
    unsigned char raw[44];
    uha03_header h;
    int rc;
    if (argc > 1) path = argv[1];
    fp = fopen(path, "rb");
    if (!fp) { printf("cannot open %s\n", path); return 1; }
    if (fread(raw, 1, 44, fp) != 44) { printf("short read\n"); return 1; }
    fclose(fp);
    printf("raw magic: %02X %02X %02X %02X\n", raw[0], raw[1], raw[2], raw[3]);
    rc = uha03_header_parse(raw, &h);
    printf("parse rc=%d (%s)\n", rc, uha03_strerror(rc));
    if (rc != 0) return 1;
    printf("flags_pw=%02X flags_filt=%02X unk_dfa=%02X ndirs=%lu unk_dfd=%lu\n",
        h.flags_pw, h.flags_filt, h.unk_dfa, (unsigned long)h.ndirs,
        (unsigned long)h.unk_dfd);
    printf("base_e01=%lu nfiles=%lu names_len=%lu\n",
        (unsigned long)h.base_e01, (unsigned long)h.nfiles,
        (unsigned long)h.names_len);
    printf("cmp1=%lu cmp2=%lu cmp3=%lu crc1=%04X crc2=%04X crc3=%04X hdr=%04X\n",
        (unsigned long)h.cmp_sz1, (unsigned long)h.cmp_sz2,
        (unsigned long)h.cmp_sz3,
        h.crc1, h.crc2, h.crc3, h.hdr_crc);
    printf("time=%04X date=%04X\n", h.dostime, h.dosdate);
    return 0;
}
