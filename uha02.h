/* UHARC 0.2 archive unpacker - public API (C89 portable)
 * Reverse engineered from Uharc02.c / Uharc02.lst (UHARC 0.2, DOS32).
 * Original 0.2 executable is DOS32 and not runnable here; this port is
 * based on static analysis plus the test archive uha02.UHA.
 * Magic "UHA\x02", 48-byte header, 4 tables (STORE/Huffman), solid stream.
 * File names use uha02* to avoid clashing with Uharc02.* on
 * case-insensitive filesystems.
 */
#ifndef UHA02_H
#define UHA02_H

#include "uha_types.h"

#define UHA02_OK 0
#define UHA02_ERR_IO 1
#define UHA02_ERR_NOMEM 2
#define UHA02_ERR_MAGIC 3
#define UHA02_ERR_VERSION 4
#define UHA02_ERR_CRC 5
#define UHA02_ERR_METHOD 6
#define UHA02_ERR_DATA 7

typedef struct uha02_entry_s {
    unsigned dir_index;
    unsigned char method;
    uint32_t size;
    uint32_t crc;
    unsigned date;
    unsigned time;
    char *dir;
    char *name;
    char *path;
    uint32_t offset;
} uha02_entry;

typedef struct uha02_header_s {
    uint32_t magic;
    unsigned dostime;
    unsigned dosdate;
    unsigned char reserved;
    unsigned char method;
    unsigned ndirs;
    uint32_t nfiles;
    uint32_t total_size;
    uint32_t data_len;
    uint32_t names_len;
    uint32_t cmp_sz1;
    uint32_t cmp_sz2;
    uint32_t cmp_sz3;
    uint32_t contents_crc;
    uint32_t hdr_crc;
} uha02_header;

typedef struct uha02_s uha02_t;

typedef int (*uha02_write_cb)(void *ctx, const unsigned char *buf,
                              unsigned len);

int uha02_open(const char *path, uha02_t **ph);
uint32_t uha02_nfiles(uha02_t *h);
const uha02_entry *uha02_entry_get(uha02_t *h, uint32_t idx);
const uha02_header *uha02_header_get(uha02_t *h);
int uha02_extract(uha02_t *h, const char *outdir,
                  int with_paths, int test_only);
void uha02_close(uha02_t *h);
const char *uha02_strerror(int code);

uint32_t uha02_crc32(const unsigned char *buf, unsigned len,
                     uint32_t crc);
void uha02_header_decrypt(unsigned char hdr[48]);
int uha02_header_parse(const unsigned char raw[48],
                       uha02_header *h);

#endif
