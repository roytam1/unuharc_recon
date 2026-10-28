/* UHARC 0.4 archive unpacker - public API (C89 portable)
 * Reverse engineered from UHARC04.c / UHARC04.lst (UHARC 0.4 BETA).
 * Magic "UHA\x04", 44-byte header, 4 tables (STORE/ALZ/Huffman).
 * File names are distinct from UHARC04.* (case-insensitive FS safe).
 */
#ifndef UHA04_H
#define UHA04_H

#define UHA04_OK 0
#define UHA04_ERR_IO 1
#define UHA04_ERR_NOMEM 2
#define UHA04_ERR_MAGIC 3
#define UHA04_ERR_VERSION 4
#define UHA04_ERR_CRC 5
#define UHA04_ERR_METHOD 6
#define UHA04_ERR_DATA 7
#define UHA04_ERR_PASSWORD 8

typedef struct uha04_entry_s {
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
} uha04_entry;

typedef struct uha04_header_s {
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
} uha04_header;

typedef struct uha04_s uha04_t;

typedef int (*uha04_write_cb)(void *ctx, const unsigned char *buf,
                              unsigned len);

int uha04_open(const char *path, uha04_t **ph);

/* Same, with an explicit password for -pw archives (flags_pw bit 1 =
 * encrypted tables and packed stream). A NULL or empty password behaves
 * like uha04_open; a protected archive without a password fails with
 * UHA04_ERR_PASSWORD. */
int uha04_open_pw(const char *path, uha04_t **ph, const char *pwd);
unsigned long uha04_nfiles(uha04_t *h);
const uha04_entry *uha04_entry_get(uha04_t *h, unsigned long idx);
const uha04_header *uha04_header_get(uha04_t *h);
int uha04_extract(uha04_t *h, const char *outdir,
                  int with_paths, int test_only);
void uha04_close(uha04_t *h);
const char *uha04_strerror(int code);

unsigned long uha04_crc32(const unsigned char *buf, unsigned len,
                          unsigned long crc);
void uha04_header_decrypt(unsigned char hdr[44]);
int uha04_header_parse(const unsigned char raw[44],
                       uha04_header *h);

#endif
