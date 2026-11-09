/* UHARC multimedia (BMP/WAV/VOC) inverse filter - C89.
 * See uha02_mm.c for the algorithm and its provenance. Applies to
 * 0.3/0.4/0.6 archives with header +9 bit1 set (flags_filt & 2 /
 * flags2 & 2) and file size >= 0x400; the 0.2 decoder must not call
 * it (0.2 uses an in-stream predictor inside core3 instead).
 */
#ifndef UHA02_MM_H
#define UHA02_MM_H

void uha02_mm_unfilter(unsigned char *buf, unsigned long len);

#endif
