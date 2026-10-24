/* UHARC multimedia (audio/image) inverse filter - C89.
 *
 * WHAT THIS FILE IS
 * -----------------
 * uha02_mm_unfilter() inverts UHARC's per-file multimedia pre-filter: the
 * delta + sign-fold transform that UHARC 0.3 and later apply to a whole
 * file *before* compression when the file is recognised as BMP/WAV/
 * Creative-VOC (sub_407D44 -> sub_407B44 on the encode side, sub_4090A0 ->
 * sub_408EB8 on the decode side, from uharc_u.c / uharc_u.lst; the same
 * pair appears at 0.6 sub_404665/404DCE and 0.4 sub_4085A0/408D20).
 *
 * IMPORTANT - UHARC 0.2 DOES NOT USE THIS FILTER
 * ----------------------------------------------
 * UHARC 0.2 (Uharc02.exe, 21 Dec 1997) has no per-file multimedia filter
 * at all:
 *   - its binary contains no BMP/WAV/VOC signature tables (byte_4288C8 /
 *     byte_4288EC / byte_428910 exist only from 0.3 on), and
 *   - the only filters in 0.2 are the E8/E9 executable pair (sub_150A4 /
 *     sub_15AD0, gated on MZ/ZM + len >= 0x400).
 * 0.2's "-mm+"/"-mm-" switch only sets byte_3220F (Uharc02.c:1842-1876),
 * which is used once, in the *encoder* (Uharc02.c:5034: model parameter
 * v5 = -1 instead of 31). Nothing about it is stored in the archive and
 * the decoder never looks at it. 0.2 gets its multimedia gain from a
 * predictor that lives *inside* core3 (methods 8..43): an in-stream
 * control token (0..5) selects a "mode"; modes 1..4 switch on a delta
 * predictor whose byte distance is ((mode-1)&3)+1 (mode 5 behaves like 1)
 * and every literal is coded as fold(~sym) plus that predictor byte, taken
 * from a 4-byte window of already-reconstructed output
 * (Uharc02.c:10626-10628, 9405-9413, 9438-9450, 9632-9655). That is a
 * codec-internal transform - it cannot be inverted after decompression,
 * because only literals are transformed while match bytes are copied
 * verbatim from already-reconstructed history.
 *
 * Verified 2026-10-24: UH02MMP.UHA (made with "a -mm+", method 27, 9
 * files, 113011 bytes) extracts byte-exactly with uha02.c + uha02_dec3.c
 * and NO post-decode filter - all 9 tab0 CRCs match, including
 * B261024.WAV (69222, DDBE2A33) and BUNTITE.BMP (12342, D8A04411).
 * => Do not call uha02_mm_unfilter() from uha02.c; uha02_dec3.c already
 *    produces the original bytes. Calling it on 0.2 output would corrupt
 *    exactly the two files it recognises.
 *
 * ALGORITHM (encode side, per file, chunk = whole file here)
 * ----------------------------------------------------------
 *   if len <= 15: untouched.
 *   Detect the format in the first min(36,len) bytes with the three
 *   36-byte signature tables below ('?' = 0x3F = wildcard). A candidate
 *   that mismatches is dropped; if all three are dropped the file is not
 *   filtered. Those first 36 bytes are *not* transformed.
 *   Period (v3) is derived from the detected format, exactly as 0.3 does:
 *     - buf[22] seeds it (WAV channel count); if the WAV signature is
 *       alive it must be 1 or 2, BMP/VOC ignore this check.
 *     - at offset 34: BMP -> 3; VOC -> 1; WAV -> keep the seed, doubled
 *       when buf[34] (bits per sample) is 16 (8 leaves it alone, any
 *       other value means "not audio" -> no filter).
 *   Reachable periods: 1 (8-bit mono WAV/VOC), 2 (16-bit mono or 8-bit
 *   stereo WAV), 3 (24bpp BMP), 4 (16-bit stereo WAV).
 *   shift = (8 * period - 8): the byte distance the predictor is taken
 *   from, as a 32-bit window of already-reconstructed output bytes.
 *   predictor window v4 = 0, fold[0..period-1] = 0.
 *   for i in [36, len):
 *       S = buf[i]
 *       r = i % period                        (absolute file offset)
 *       F = fold[r] ? -S : S                  (byte arithmetic)
 *       if (S & 0x80) fold[r] ^= 1            (uses the *stored* byte)
 *       O = F + ((v4 >> shift) & 0xFF)        (mod 256)
 *       buf[i] = O
 *       v4 = (v4 << 8) | O                    (32-bit)
 * The transform is invertible in isolation: given the same untouched
 * 36-byte header the period/shift/fold state are derived identically and
 * each step is undone by construction (the fold toggle depends only on
 * the stored byte, which the decoder also sees).
 *
 * Not modelled: the original processes a file in chunks and keeps v4/fold/
 * period in globals across calls; a whole-buffer call is equivalent (the
 * original never re-runs detection after offset 0). The shift is masked
 * with 31 to mirror x86's shift-count masking, which matters only if a
 * period above 4 could ever be produced; with the signature tables above
 * it cannot.
 *
 * VERIFIED IN ISOLATION (2026-10-24, scratch harness in the build dir)
 * -------------------------------------------------------------------
 *  - B261024.WAV (69222, real 0.2 ground truth): encoder-side filter
 *    (faithful sub_407B44 port, WAV -> period 2) then uha02_mm_unfilter
 *    -> byte-exact; 45018 of 69222 bytes actually change.
 *  - BUNTITE.BMP (12342, real 0.2 ground truth): period 3 -> byte-exact;
 *    12285 bytes change.
 *  - synthetic 8-bit mono WAV (period 1), 16-bit stereo WAV (period 4),
 *    Creative VOC (period 1): byte-exact.
 *  - 35-byte and 15-byte buffers: untouched (detection-only / below the
 *    16-byte gate), and non-media files (TXT/EXE/PRV) pass through
 *    unchanged.
 * There is no real 0.3/0.4/0.6 filtered stream available here to validate
 * against (the 0.3 core4 method is not decoded by this repo yet), so the
 * evidence is encoder-inverse round-trip plus a byte-level reading of
 * sub_408EB8 / sub_4090A0.
 */

#define MM_WILD 0x3F

/* BMP: "BM", planes=1 at 26, bpp=24 at 28, compression=0 at 30. */
static const unsigned char mm_sig_bmp[36] = {
    0x42, 0x4D, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F
};

/* WAV: "RIFF" size "WAVE" "fmt " size, PCM(1) channels, ..., bits[3..2]
 * zero (byte 35); byte 34 (bits/sample low) selects the period. */
static const unsigned char mm_sig_wav[36] = {
    0x52, 0x49, 0x46, 0x46, 0x3F, 0x3F, 0x3F, 0x3F, 0x57, 0x41, 0x56, 0x45,
    0x66, 0x6D, 0x74, 0x20, 0x3F, 0x3F, 0x3F, 0x3F, 0x01, 0x00, 0x3F, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00
};

/* Creative Voice File: header text, 0x1A, 0x1A, data offset, version 1.0A,
 * checksum, then the codec byte at 34 (8 or 16). */
static const unsigned char mm_sig_voc[36] = {
    0x43, 0x72, 0x65, 0x61, 0x74, 0x69, 0x76, 0x65, 0x20, 0x56, 0x6F, 0x69,
    0x63, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x1A, 0x1A, 0x00, 0x0A, 0x01,
    0x29, 0x11, 0x01, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x3F, 0x3F, 0x3F, 0x3F
};

void uha02_mm_unfilter(unsigned char *buf, unsigned long len)
{
    unsigned long i;
    unsigned long v4;
    unsigned period = 0;
    unsigned shift = 0;
    unsigned char fold[8];
    unsigned is_bmp = 1, is_voc = 1, is_wav = 1;
    unsigned k;

    if (!buf || len < 16) return;   /* original needs > 15 bytes */

    for (k = 0; k < 8; k++) fold[k] = 0;
    v4 = 0;

    /* --- header signature detection: first 36 bytes stay untouched --- */
    for (i = 0; i < 36 && i < len; i++) {
        unsigned char c = buf[i];
        if (is_bmp && mm_sig_bmp[i] != MM_WILD && mm_sig_bmp[i] != c)
            is_bmp = 0;
        if (is_voc && mm_sig_voc[i] != MM_WILD && mm_sig_voc[i] != c)
            is_voc = 0;
        if (is_wav && mm_sig_wav[i] != MM_WILD && mm_sig_wav[i] != c)
            is_wav = 0;
        if (!is_bmp && !is_voc && !is_wav) return;  /* not a media file */

        if (i == 22) {
            period = c;
            v4 = 0;
            for (k = 0; k < 8; k++) fold[k] = 0;
            /* WAV candidate: byte 22 is the channel count (1 or 2) and
             * seeds the period; the 16-bit doubling happens at 34. */
            if (is_wav && c != 1 && c != 2) return;
        }
        if (i == 34) {
            if (is_bmp) period = 3;       /* 24 bpp: R,G,B triples */
            if (is_voc) period = 1;
            if (is_wav) {                 /* byte 34 = bits per sample */
                if (c == 16) period *= 2;
                else if (c != 8) return;
            }
            shift = (8 * period - 8) & 31;   /* x86 shift-count masking */
        }
    }

    if (period == 0) return;    /* nothing recognised / degenerate period */

    /* --- delta + sign-fold inverse from the first untransformed byte --- */
    for (; i < len; i++) {
        unsigned s = buf[i];
        unsigned r = (unsigned)(i % period);
        unsigned f = fold[r] ? (unsigned)((0u - s) & 0xFFu) : s;
        unsigned o;
        if (s >= 0x80u) fold[r] = (unsigned char)(fold[r] ^ 1);
        o = (unsigned)(f + ((v4 >> shift) & 0xFFUL)) & 0xFFu;
        buf[i] = (unsigned char)o;
        v4 = ((v4 << 8) | (unsigned long)o) & 0xFFFFFFFFUL;
    }
}
