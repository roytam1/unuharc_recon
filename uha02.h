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
    unsigned long size;
    unsigned long crc;
    unsigned date;
    unsigned time;
    char *dir;
    char *name;
    char *path;
    unsigned long offset;
} uha02_entry;

typedef struct uha02_header_s {
    unsigned long magic;
    unsigned dostime;
    unsigned dosdate;
    unsigned char reserved;
    unsigned char method;
    unsigned ndirs;
    unsigned long nfiles;
    unsigned long total_size;
    unsigned long data_len;
    unsigned long names_len;
    unsigned long cmp_sz1;
    unsigned long cmp_sz2;
    unsigned long cmp_sz3;
    unsigned long contents_crc;
    unsigned long hdr_crc;
} uha02_header;

typedef struct uha02_s uha02_t;

typedef int (*uha02_write_cb)(void *ctx, const unsigned char *buf,
                              unsigned len);

int uha02_open(const char *path, uha02_t **ph);
unsigned long uha02_nfiles(uha02_t *h);
const uha02_entry *uha02_entry_get(uha02_t *h, unsigned long idx);
const uha02_header *uha02_header_get(uha02_t *h);
int uha02_extract(uha02_t *h, const char *outdir,
                  int with_paths, int test_only);
void uha02_close(uha02_t *h);
const char *uha02_strerror(int code);

unsigned long uha02_crc32(const unsigned char *buf, unsigned len,
                          unsigned long crc);
void uha02_header_decrypt(unsigned char hdr[48]);
int uha02_header_parse(const unsigned char raw[48],
                       uha02_header *h);

#endif
