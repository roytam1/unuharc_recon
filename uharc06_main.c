/* uharc06_main - UHARC 0.6 unpacker tool (C89, portable)
 * Matches UNUHARC06.EXE CLI subset: x/e/t
 *   x archive[.uha] [outdir] : extract with paths
 *   e archive[.uha] [outdir] : extract without paths (flat)
 *   t archive[.uha]          : test (decode without writing)
 * NOTE: named uharc06_main.c to avoid clashing with UNUHARC06.c
 * on case-insensitive filesystems.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uharc06.h"

static void banner(void)
{
    printf("UnUHARC 0.6b (C port) ---- high compression multimedia archiver ----\n");
    printf("Portable C89 unpacker, derived from UNUHARC06.EXE reverse engineering.\n");
}

static void usage(void)
{
    printf("Usage: uharc06_main <x|e|t> archive[.uha] [outdir]\n");
    printf("  x = extract with directory names (default)\n");
    printf("  e = extract without directory names (flat)\n");
    printf("  t = test archive integrity (no output)\n");
}

int main(int argc, char **argv)
{
    const char *cmd;
    const char *arc;
    const char *outdir;
    char arcbuf[1024];
    uharc06_t *h;
    int rc, i;
    int with_paths;
    int test_only;

    banner();
    if (argc < 3) {
        usage();
        return 255;
    }
    cmd = argv[1];
    arc = argv[2];
    outdir = (argc >= 4) ? argv[3] : ".";

    if ((cmd[0] == 'x' || cmd[0] == 'X') && cmd[1] == 0) {
        with_paths = 1; test_only = 0;
    } else if ((cmd[0] == 'e' || cmd[0] == 'E') && cmd[1] == 0) {
        with_paths = 0; test_only = 0;
    } else if ((cmd[0] == 't' || cmd[0] == 'T') && cmd[1] == 0) {
        with_paths = 1; test_only = 1;
    } else {
        printf("ERROR: Invalid command \"%c\"\n", cmd[0]);
        printf("Process aborted.\n");
        return 255;
    }

    h = 0;
    rc = uharc06_open(arc, &h);
    if (rc != UHARC06_OK) {
        int rc2;
        if (rc == UHARC06_ERR_IO && strlen(arc) + 5 < sizeof(arcbuf)) {
            sprintf(arcbuf, "%s.uha", arc);
            rc2 = uharc06_open(arcbuf, &h);
            if (rc2 == UHARC06_OK) {
                arc = arcbuf;
                rc = rc2;
            }
        }
    }
    if (rc != UHARC06_OK) {
        printf("ERROR: %s (%s)\n", uharc06_strerror(rc), arc);
        printf("Process aborted.\n");
        if (rc == UHARC06_ERR_METHOD || rc == UHARC06_ERR_DATA
            || rc == UHARC06_ERR_CRC) {
            printf("Note: directory tables need core3 ALZ (method 17).\n");
            printf("See README, FORMAT.md and test_dec3 for status.\n");
        }
        return 255;
    }

    {
        const uharc06_header *hh = uharc06_header_get(h);
        unsigned long n = uharc06_nfiles(h);
        printf("Processing archive \"%s\" (method %u, %lu files, %lu bytes).\n",
            arc, hh->method, n, hh->total_size);
    }

    if (test_only) {
        printf("Testing %lu files...\n", uharc06_nfiles(h));
    } else {
        printf("Extracting %lu files...\n", uharc06_nfiles(h));
    }

    for (i = 0; (unsigned long)i < uharc06_nfiles(h); i++) {
        const uharc06_entry *e = uharc06_entry_get(h, (unsigned long)i);
        if (e) {
            printf("  %s (%lu bytes)\n", e->path, e->size);
        }
    }

    rc = uharc06_extract(h, outdir, with_paths, test_only);
    uharc06_close(h);
    if (rc != UHARC06_OK) {
        printf("ERROR: %s\n", uharc06_strerror(rc));
        printf("Process aborted.\n");
        if (rc == UHARC06_ERR_METHOD || rc == UHARC06_ERR_DATA) {
            printf("Note: method 17/34 decompressors (ALZ/PPM) are not yet bit-exact.\n");
            printf("See README and test_tab/test_dec3 for current status.\n");
        }
        return 255;
    }
    printf("Completed successfully. All files OK\n");
    return 0;
}
