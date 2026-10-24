/* UHARC solid-stream splitter - shared by the 0.3/0.4 containers (C89).
 *
 * Decodes one solid stream (method from the archive header) and splits the
 * output across files by their unpacked sizes, verifying each file's
 * CRC32 against the archive's table0 value.  Mirrors the logic already
 * verified in uha02.c / uharc06.c.
 *
 * Not version-specific: the caller supplies the method, the decoder
 * version selector and the input callback.
 */
#ifndef UHARC_SPLIT_H
#define UHARC_SPLIT_H

#include "uharc_dec.h"

typedef struct uharc_split_entry_s {
    const char *name;      /* basename used for flat extraction */
    const char *path;      /* relative path used when with_paths */
    unsigned long size;    /* unpacked size */
    unsigned long crc;     /* expected CRC32 (full 32 bits) */
} uharc_split_entry;

/* out_expected: total unpacked size (sum of entries[i].size)
 * returns 0 ok, UHARC06-style error code otherwise */
int uharc_split_run(const char *outdir, int with_paths, int test_only,
                    const uharc_split_entry *ents, unsigned long nfiles,
                    int method, int ver,
                    uharc_dec_read_cb rcb, void *rctx,
                    unsigned long out_expected);

/* error codes (subset, mirrors containers) */
#define USPLIT_OK 0
#define USPLIT_ERR_IO 1
#define USPLIT_ERR_NOMEM 2
#define USPLIT_ERR_CRC 5
#define USPLIT_ERR_DATA 7

#endif
