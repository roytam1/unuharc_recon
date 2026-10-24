/* uharc tool - UHARC 0.2/0.3/0.4/0.6 unpacker (C89, portable)
 * Auto-detects UHA\\x04 vs UHA\\x06 via magic, dispatches to uha04 / uharc06.
 * Matches UNUHARC06 x/e/t plus UHARC04 l (list) for both versions.
 * File named uharc06_main.c to avoid clashing with UNUHARC06.c
 * on case-insensitive filesystems (kept for history).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uharc06.h"
#include "uha04.h"
#include "uha03.h"
#include "uha02.h"

static void banner(void)
{
    printf("UHARC 0.2/0.3/0.4/0.6 (C port) ---- high compression multimedia archiver ----\n");
    printf("Portable C89 unpacker, derived from Uharc02/uharc_u/UHARC04/UNUHARC06 reverse engineering.\n");
}

static void usage(void)
{
    printf("Usage: uharc_tool <x|e|t|l> archive[.uha] [outdir]\n");
    printf("  x = extract with directory names\n");
    printf("  e = extract without directory names (flat)\n");
    printf("  t = test archive integrity (no output)\n");
    printf("  l = list files (directory only)\n");
}

static int detect_version(const char *path, unsigned *ver)
{
    FILE *fp;
    unsigned char m[4];
    fp = fopen(path, "rb");
    if (!fp) return 0;
    if (fread(m, 1, 4, fp) != 4) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    if (!(m[0] == 0x55 && m[1] == 0x48 && m[2] == 0x41)) return 0;
    *ver = m[3];
    return 1;
}

int main(int argc, char **argv)
{
    const char *cmd;
    const char *arc;
    const char *outdir;
    char arcbuf[1024];
    int with_paths;
    int test_only;
    int list_only;
    unsigned ver;
    int rc;

    banner();
    if (argc < 3) {
        usage();
        return 255;
    }
    cmd = argv[1];
    arc = argv[2];
    outdir = (argc >= 4) ? argv[3] : ".";

    with_paths = 1; test_only = 0; list_only = 0;
    if ((cmd[0] == 'x' || cmd[0] == 'X') && cmd[1] == 0) {
        with_paths = 1;
    } else if ((cmd[0] == 'e' || cmd[0] == 'E') && cmd[1] == 0) {
        with_paths = 0;
    } else if ((cmd[0] == 't' || cmd[0] == 'T') && cmd[1] == 0) {
        test_only = 1;
    } else if ((cmd[0] == 'l' || cmd[0] == 'L') && cmd[1] == 0) {
        list_only = 1;
    } else {
        printf("ERROR: Invalid command \"%c\"\n", cmd[0]);
        printf("Process aborted.\n");
        return 255;
    }

    if (!detect_version(arc, &ver)) {
        if (strlen(arc) + 5 < sizeof(arcbuf)) {
            sprintf(arcbuf, "%s.uha", arc);
            if (detect_version(arcbuf, &ver)) arc = arcbuf;
        }
    }
    if (!detect_version(arc, &ver)) {
        printf("ERROR: I/O error (%s)\n", arc);
        printf("Process aborted.\n");
        return 255;
    }

    if (ver == 0x06) {
        uharc06_t *h = 0;
        unsigned long i, n;
        const uharc06_header *hh;
        rc = uharc06_open(arc, &h);
        if (rc != UHARC06_OK && rc == UHARC06_ERR_IO) {
            if (strlen(arc) + 5 < sizeof(arcbuf)) {
                sprintf(arcbuf, "%s.uha", arc);
                if (detect_version(arcbuf, &ver) && ver == 0x06) {
                    rc = uharc06_open(arcbuf, &h);
                    if (rc == UHARC06_OK) arc = arcbuf;
                }
            }
        }
        if (rc != UHARC06_OK) {
            printf("ERROR: %s (%s)\n", uharc06_strerror(rc), arc);
            printf("Process aborted.\n");
            if (rc == UHARC06_ERR_METHOD || rc == UHARC06_ERR_DATA
                || rc == UHARC06_ERR_CRC) {
                printf("Note: 0.6 directory needs core3 ALZ (method 17).\n");
                printf("See README, FORMAT.md and test_dec3 for status.\n");
            }
            return 255;
        }
        hh = uharc06_header_get(h);
        n = uharc06_nfiles(h);
        printf("Processing archive \"%s\" (UHA 0.6, method %u, %lu files, %lu bytes).\n",
            arc, hh->method, n, hh->total_size);
        for (i = 0; i < n; i++) {
            const uharc06_entry *e = uharc06_entry_get(h, i);
            if (e) printf("  %s (%lu bytes)\n", e->path, e->size);
        }
        if (list_only) {
            uharc06_close(h);
            printf("Completed successfully. All files OK\n");
            return 0;
        }
        if (test_only) printf("Testing %lu files...\n", n);
        else printf("Extracting %lu files...\n", n);
        rc = uharc06_extract(h, outdir, with_paths, test_only);
        uharc06_close(h);
        if (rc != UHARC06_OK) {
            printf("ERROR: %s\n", uharc06_strerror(rc));
            printf("Process aborted.\n");
            return 255;
        }
        printf("Completed successfully. All files OK\n");
        return 0;
    } else if (ver == 0x04) {
        uha04_t *h = 0;
        unsigned long i, n;
        const uha04_header *hh;
        rc = uha04_open(arc, &h);
        if (rc != UHA04_OK) {
            printf("ERROR: %s (%s)\n", uha04_strerror(rc), arc);
            printf("Process aborted.\n");
            if (rc == UHA04_ERR_METHOD || rc == UHA04_ERR_DATA
                || rc == UHA04_ERR_CRC) {
                printf("Note: 0.4 directory needs cores 2/3 (methods 2/7/17).\n");
                printf("See README, FORMAT.md for status.\n");
            }
            return 255;
        }
        hh = uha04_header_get(h);
        n = uha04_nfiles(h);
        printf("Processing archive \"%s\" (UHA 0.4, %lu files).\n",
            arc, n);
        printf("header: ndirs=%u base=%lu names=%lu cmp=%lu/%lu/%lu\n",
            hh->ndirs, hh->base_e01, hh->names_len,
            hh->cmp_sz1, hh->cmp_sz2, hh->cmp_sz3);
        for (i = 0; i < n; i++) {
            const uha04_entry *e = uha04_entry_get(h, i);
            if (e) printf("  %s (%lu bytes, m=%u)\n", e->path, e->size, e->method);
        }
        if (list_only) {
            uha04_close(h);
            printf("Completed successfully. All files OK\n");
            return 0;
        }
        if (test_only) printf("Testing %lu files...\n", n);
        else printf("Extracting %lu files...\n", n);
        rc = uha04_extract(h, outdir, with_paths, test_only);
        uha04_close(h);
        if (rc != UHA04_OK) {
            printf("ERROR: %s\n", uha04_strerror(rc));
            printf("Process aborted.\n");
            return 255;
        }
        printf("Completed successfully. All files OK\n");
        return 0;
    } else if (ver == 0x03) {
        uha03_t *h = 0;
        unsigned long i, n;
        const uha03_header *hh;
        rc = uha03_open(arc, &h);
        if (rc != UHA03_OK) {
            printf("ERROR: %s (%s)\n", uha03_strerror(rc), arc);
            printf("Process aborted.\n");
            if (rc == UHA03_ERR_METHOD || rc == UHA03_ERR_DATA
                || rc == UHA03_ERR_CRC) {
                printf("Note: 0.3 directory needs 0.3-specific cores 2/3.\n");
                printf("See README, FORMAT.md for status.\n");
            }
            return 255;
        }
        hh = uha03_header_get(h);
        n = uha03_nfiles(h);
        printf("Processing archive \"%s\" (UHA 0.3, %lu files).\n",
            arc, n);
        printf("header: ndirs=%u base=%lu names=%lu cmp=%lu/%lu/%lu\n",
            hh->ndirs, hh->base_e01, hh->names_len,
            hh->cmp_sz1, hh->cmp_sz2, hh->cmp_sz3);
        for (i = 0; i < n; i++) {
            const uha03_entry *e = uha03_entry_get(h, i);
            if (e) printf("  %s (%lu bytes, m=%u)\n", e->path, e->size, e->method);
        }
        if (list_only) {
            uha03_close(h);
            printf("Completed successfully. All files OK\n");
            return 0;
        }
        if (test_only) printf("Testing %lu files...\n", n);
        else printf("Extracting %lu files...\n", n);
        rc = uha03_extract(h, outdir, with_paths, test_only);
        uha03_close(h);
        if (rc != UHA03_OK) {
            printf("ERROR: %s\n", uha03_strerror(rc));
            printf("Process aborted.\n");
            return 255;
        }
        printf("Completed successfully. All files OK\n");
        return 0;
    } else if (ver == 0x02) {
        uha02_t *h = 0;
        unsigned long i, n;
        const uha02_header *hh;
        rc = uha02_open(arc, &h);
        if (rc != UHA02_OK) {
            printf("ERROR: %s (%s)\n", uha02_strerror(rc), arc);
            printf("Process aborted.\n");
            if (rc == UHA02_ERR_METHOD || rc == UHA02_ERR_DATA
                || rc == UHA02_ERR_CRC) {
                printf("Note: 0.2 directory needs 0.2-specific cores 2/3.\n");
                printf("See README, FORMAT.md for status.\n");
            }
            return 255;
        }
        hh = uha02_header_get(h);
        n = uha02_nfiles(h);
        printf("Processing archive \"%s\" (UHA 0.2, %lu files).\n",
            arc, n);
        printf("header: ndirs=%u base=%lu names=%lu cmp=%lu/%lu/%lu\n",
            hh->ndirs, hh->data_len, hh->names_len,
            hh->cmp_sz1, hh->cmp_sz2, hh->cmp_sz3);
        for (i = 0; i < n; i++) {
            const uha02_entry *e = uha02_entry_get(h, i);
            if (e) printf("  %s (%lu bytes, m=%u)\n", e->path, e->size, e->method);
        }
        if (list_only) {
            uha02_close(h);
            printf("Completed successfully. All files OK\n");
            return 0;
        }
        if (test_only) printf("Testing %lu files...\n", n);
        else printf("Extracting %lu files...\n", n);
        rc = uha02_extract(h, outdir, with_paths, test_only);
        uha02_close(h);
        if (rc != UHA02_OK) {
            printf("ERROR: %s\n", uha02_strerror(rc));
            printf("Process aborted.\n");
            return 255;
        }
        printf("Completed successfully. All files OK\n");
        return 0;
    } else {
        printf("ERROR: Unsupported UHA version 0x%02X (%s)\n", ver, arc);
        printf("Process aborted.\n");
        return 255;
    }
}
