/* UHARC solid-stream splitter - shared implementation (C89).
 * See uharc_split.h. No globals: all state lives in one struct.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uharc_split.h"
#include "uharc06.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

typedef struct split_ctx_s {
    const uharc_split_entry *ents;
    unsigned long nfiles;
    unsigned long idx;
    unsigned long remaining;
    const char *outdir;
    int with_paths;
    int test_only;
    FILE *cur;
    unsigned long crc;
    char path[2048];
    int err;
} split_ctx;

static unsigned long crc32b(const unsigned char *p, unsigned long n,
                            unsigned long c)
{
    return uharc06_crc32(p, (unsigned)n, c);
}

static void mkdir_parents(char *path)
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

/* build split_ctx->path for entry i */
static int build_path(split_ctx *s, unsigned long i)
{
    const uharc_split_entry *e = &s->ents[i];
    const char *rel = (s->with_paths && e->path && e->path[0]) ? e->path
                                                              : e->name;
    size_t od = strlen(s->outdir);
    size_t need = od + 1 + strlen(rel) + 1;
    if (need > sizeof(s->path)) return 0;
    memcpy(s->path, s->outdir, od);
    s->path[od] = '/';
    strcpy(s->path + od + 1, rel);
    return 1;
}

static int open_cur(split_ctx *s)
{
    if (!build_path(s, s->idx)) return 0;
    if (!s->test_only) {
        mkdir_parents(s->path);
        s->cur = fopen(s->path, "wb");
        if (!s->cur) return 0;
    } else {
        s->cur = 0;
    }
    s->crc = 0xFFFFFFFFUL;
    return 1;
}

/* create a zero-length file (or check its CRC in test mode) */
static int emit_empty(split_ctx *s)
{
    const uharc_split_entry *e = &s->ents[s->idx];
    if ((e->crc & 0xFFFFFFFFUL) != 0) {
        s->err = USPLIT_ERR_CRC;
        return 0;
    }
    if (!s->test_only) {
        if (!build_path(s, s->idx)) return 0;
        mkdir_parents(s->path);
        {
            FILE *f = fopen(s->path, "wb");
            if (!f) return 0;
            fclose(f);
        }
    }
    return 1;
}

static int split_write(void *ctx, const unsigned char *buf, unsigned len)
{
    split_ctx *s = (split_ctx *)ctx;
    while (len > 0) {
        unsigned long chunk;
        if (s->idx >= s->nfiles) {
            s->err = USPLIT_ERR_DATA;
            return 1;
        }
        if (s->remaining == 0) {
            s->err = USPLIT_ERR_DATA;
            return 1;
        }
        chunk = len;
        if (chunk > s->remaining) chunk = (unsigned) s->remaining;
        if (!s->test_only && !s->cur) {
            if (!open_cur(s)) return 1;
        }
        if (chunk > 0) {
            if (!s->test_only) {
                size_t w = fwrite(buf, 1, (size_t)chunk, s->cur);
                if (w != (size_t)chunk) {
                    s->err = USPLIT_ERR_IO;
                    return 1;
                }
            }
            s->crc = crc32b(buf, chunk, s->crc);
        }
        buf += chunk;
        len -= (unsigned)chunk;
        s->remaining -= chunk;
        if (s->remaining == 0) {
            if (s->cur) { fclose(s->cur); s->cur = 0; }
            if (((~s->crc) & 0xFFFFFFFFUL) !=
                (s->ents[s->idx].crc & 0xFFFFFFFFUL)) {
                s->err = USPLIT_ERR_CRC;
                return 1;
            }
            /* move to the next entry, creating any empty files */
            s->idx++;
            while (s->idx < s->nfiles && s->ents[s->idx].size == 0) {
                if (!emit_empty(s)) return 1;
                s->idx++;
            }
            if (s->idx < s->nfiles) {
                s->remaining = s->ents[s->idx].size;
                if (!open_cur(s)) return 1;
            }
        }
    }
    return 0;
}

int uharc_split_run(const char *outdir, int with_paths, int test_only,
                    const uharc_split_entry *ents, unsigned long nfiles,
                    int method, int ver,
                    uharc_dec_read_cb rcb, void *rctx,
                    unsigned long out_expected)
{
    split_ctx s;
    int rc;
    memset(&s, 0, sizeof(s));
    s.ents = ents;
    s.nfiles = nfiles;
    s.outdir = outdir ? outdir : ".";
    s.with_paths = with_paths;
    s.test_only = test_only;
    s.crc = 0xFFFFFFFFUL;

    /* leading empty files */
    while (s.idx < s.nfiles && s.ents[s.idx].size == 0) {
        if (!emit_empty(&s)) return s.err ? s.err : USPLIT_ERR_IO;
        s.idx++;
    }
    if (s.idx < s.nfiles) {
        s.remaining = s.ents[s.idx].size;
        if (!open_cur(&s)) return USPLIT_ERR_IO;
    } else {
        s.remaining = 0;
    }

    rc = uharc_dec_decode_v(method, 1, ver, rcb, rctx, split_write, &s,
                            out_expected);
    if (s.cur) { fclose(s.cur); s.cur = 0; }
    if (s.err) return s.err;
    if (rc == 2) return USPLIT_ERR_NOMEM;
    if (rc != 0) return USPLIT_ERR_DATA;
    /* all files must have been consumed: a completed last file leaves
     * idx past the end (it was advanced when remaining hit zero) */
    if (s.idx < s.nfiles) {
        if (!(s.idx + 1 == s.nfiles && s.remaining == 0)) return USPLIT_ERR_DATA;
    }
    return USPLIT_OK;
}
