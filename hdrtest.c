/* header test: parse D3X.UHA header, no decompression */
#include <stdio.h>
#include "uharc06.h"

int main(int argc, char **argv)
{
    const char *path = "D3X.UHA";
    FILE *fp;
    unsigned char raw[39];
    uharc06_header h;
    int rc;
    if (argc > 1) path = argv[1];
    fp = fopen(path, "rb");
    if (!fp) { printf("cannot open %s\n", path); return 1; }
    if (fread(raw, 1, 39, fp) != 39) { printf("short read\n"); return 1; }
    fclose(fp);
    printf("raw magic: %02X %02X %02X %02X\n", raw[0], raw[1], raw[2], raw[3]);
    rc = uharc06_header_parse(raw, &h);
    printf("parse rc=%d (%s)\n", rc, uharc06_strerror(rc));
    if (rc != 0) {
        printf("decrypted bytes 4..38: ");
        {
            unsigned char b[39];
            unsigned i;
            for (i = 0; i < 39; i++) b[i] = raw[i];
            uharc06_header_decrypt(b);
            for (i = 0; i < 39; i++) printf("%02X ", b[i]);
            printf("\n");
        }
        return 1;
    }
    printf("method=%u ndirs=%lu nfiles=%lu total=%lu data_off=%lu\n",
        h.method, h.ndirs, h.nfiles, h.total_size, h.data_off);
    printf("tab1 dec=%lu cmp=%lu crc=%04X\n",
        h.tab1_dec_size, h.tab1_cmp_size, h.tab1_crc);
    printf("tab2 cmp=%lu crc=%04X hdr_crc=%04X\n",
        h.tab2_cmp_size, h.tab2_crc, h.hdr_crc);
    printf("dostime=%04X dosdate=%04X flags1=%02X flags2=%02X\n",
        h.dostime, h.dosdate, h.flags1, h.flags2);
    return 0;
}
