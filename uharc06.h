/* UHARC 0.6 archive unpacker - public API (C89 portable) */
#ifndef UHARC06_H
#define UHARC06_H

/* error codes */
#define UHARC06_OK 0
#define UHARC06_ERR_IO 1
#define UHARC06_ERR_NOMEM 2
#define UHARC06_ERR_MAGIC 3
#define UHARC06_ERR_VERSION 4
#define UHARC06_ERR_CRC 5
#define UHARC06_ERR_METHOD 6
#define UHARC06_ERR_DATA 7

/* compression method ids (raw byte values 0..87) */
#define UHARC06_METHOD_STORE 0

/* file entry (19-byte on-disk record decoded) */
typedef struct uharc06_entry_s {
    unsigned dir_index;      /* +0 word */
    unsigned char method;    /* +2 byte (0xFF = separator/skip) */
    unsigned long size;      /* +3 dword (unpacked size) */
    unsigned long crc;       /* +7 dword (stored file crc, from table0) */
    unsigned date;           /* +11 word DOS date */
    unsigned time;           /* +13 word DOS time */
    char *dir;               /* directory string (may be "") */
    char *name;              /* file name */
    char *path;              /* full path dir/name */
    unsigned long offset;    /* cumulative unpacked offset (for solid stream) */
} uharc06_entry;

/* archive header (39 bytes on disk, bytes 4..35 encrypted) */
typedef struct uharc06_header_s {
    unsigned long magic;        /* 0x06414855 "UHA\x06" */
    unsigned dostime;           /* DOS time */
    unsigned dosdate;           /* DOS date */
    unsigned char flags1;       /* byte_433E10 */
    unsigned char flags2;       /* byte_433E11 */
    unsigned char method;       /* byte_433E12 (solid stream method) */
    unsigned ndirs;             /* word_433E13 */
    unsigned long total_size;   /* dword_433E15 */
    unsigned long data_off;     /* dword_433E19 */
    unsigned long nfiles;       /* dword_433E1D & 0xFFFFFF */
    unsigned long tab1_dec_size;/* *(E1D+3) & 0xFFFFFF */
    unsigned long tab1_cmp_size;/* dword_433E23 & 0xFFFFFF */
    unsigned long tab2_cmp_size;/* *(E23+3) & 0xFFFFFF */
    unsigned tab1_crc;          /* word_433E29 (low 16 of crc32) */
    unsigned tab2_crc;          /* word_433E2B */
    unsigned hdr_crc;           /* word_433E2D */
} uharc06_header;

/* opaque archive handle */
typedef struct uharc06_s uharc06_t;

/* callbacks for decompressed output */
typedef int (*uharc06_write_cb)(void *ctx, const unsigned char *buf,
                                unsigned len);

/* open archive, parse header + directory (decompresses tables).
 * On success returns UHARC06_OK and *ph != 0.
 * err_detail optionally receives method/error detail.
 */
int uharc06_open(const char *path, uharc06_t **ph);

/* query */
unsigned long uharc06_nfiles(uharc06_t *h);
const uharc06_entry *uharc06_entry_get(uharc06_t *h, unsigned long idx);
const uharc06_header *uharc06_header_get(uharc06_t *h);

/* extract: if with_paths!=0 recreate dir/name, else flat names.
 * test_only!=0 decodes but writes nothing (like 't').
 * progress callback may be NULL.
 */
int uharc06_extract(uharc06_t *h, const char *outdir,
                    int with_paths, int test_only);

/* close */
void uharc06_close(uharc06_t *h);

/* error string */
const char *uharc06_strerror(int code);

/* low level helpers (exposed for tests) */
unsigned long uharc06_crc32(const unsigned char *buf, unsigned len,
                            unsigned long crc);
void uharc06_header_decrypt(unsigned char hdr[39]);
int uharc06_header_parse(const unsigned char raw[39],
                         uharc06_header *h);

#endif
