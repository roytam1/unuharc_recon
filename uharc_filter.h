/* UHARC 0.3/0.4 executable (E8/E9) post-filter - C89.
 * See uharc_filter.c for what the filter does and its original gate.
 */
#ifndef UHARC_FILTER_H
#define UHARC_FILTER_H

/* In-place inverse EXE filter for a whole file.
 * ver: 3 (UHARC 0.3), 4 (UHARC 0.4) or 6 (UHARC 0.6).  0.3/0.4 share one
 * algorithm; 0.6 adds a byte_433E36 pre-transform for methods 24..39
 * (byte_42251F[method]==4 in the original), enabled here via f6.
 * No-op unless len >= 0x400 and the buffer starts with an MZ/ZM/MQ
 * executable header (the original also accepts ELF and "FBOV"). */
void uharc_filter_exe(unsigned char *buf, unsigned long len, int ver, int f6);

#endif
