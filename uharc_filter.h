/* UHARC 0.3/0.4 executable (E8/E9) post-filter - C89.
 * See uharc_filter.c for what the filter does and its original gate.
 */
#ifndef UHARC_FILTER_H
#define UHARC_FILTER_H

/* In-place inverse EXE filter for a whole file.
 * ver: 3 (UHARC 0.3) or 4 (UHARC 0.4).  Both use the same algorithm.
 * No-op unless len >= 0x400 and the buffer starts with an MZ/ZM/MQ
 * executable header (the original also accepts ELF and "FBOV"). */
void uharc_filter_exe(unsigned char *buf, unsigned long len, int ver);

#endif
