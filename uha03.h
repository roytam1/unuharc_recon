/* UHARC 0.3 archive unpacker - public API (C89 portable)
 * Reverse engineered from uharc_u.c / uharc_u.lst (UHARC 0.3, full archiver).
 * Magic "UHA\x03", 44-byte header, 4 tables (STORE/ALZ/Huffman), per-file.
 * File names use uha03* to avoid clashing with uharc_u.* on
 * case-insensitive filesystems.
 */
#ifndef UHA03_H
#define UHA03_H

#define UHA03_OK 0
#define UHA03_ERR_IO 1
#define UHA03_ERR_NOMEM 2
#define UHA03_ERR_MAGIC 3
#define UHA03_ERR_VERSION 4
#define UHA03_ERR_CRC 5
#define UHA03_ERR_METHOD 6
#define UHA03_ERR_DATA 7

typedef struct uha03_entry_s {
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
} uha03_entry;

typedef struct uha03_header_s {
    unsigned long magic;
    unsigned dostime;
    unsigned dosdate;
    unsigned char flags_pw;
    unsigned char flags_filt;
    unsigned char unk_dfa;
    unsigned ndirs;
    unsigned long unk_dfd;
    unsigned long base_e01;
    unsigned long nfiles;
    unsigned long names_len;
    unsigned long cmp_sz1;
    unsigned long cmp_sz2;
    unsigned long cmp_sz3;
    unsigned crc1;
    unsigned crc2;
    unsigned crc3;
    unsigned hdr_crc;
} uha03_header;

typedef struct uha03_s uha03_t;

typedef int (*uha03_write_cb)(void *ctx, const unsigned char *buf,
                              unsigned len);

int uha03_open(const char *path, uha03_t **ph);
unsigned long uha03_nfiles(uha03_t *h);
const uha03_entry *uha03_entry_get(uha03_t *h, unsigned long idx);
const uha03_header *uha03_header_get(uha03_t *h);
int uha03_extract(uha03_t *h, const char *outdir,
                  int with_paths, int test_only);
void uha03_close(uha03_t *h);
const char *uha03_strerror(int code);

unsigned long uha03_crc32(const unsigned char *buf, unsigned len,
                          unsigned long crc);
void uha03_header_decrypt(unsigned char hdr[44]);
int uha03_header_parse(const unsigned char raw[44],
                       uha03_header *h);

#endif
