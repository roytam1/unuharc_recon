/* Fixed-region layout of the 0.4 core-4 decoder.
 *
 * The ranges below are the tables the decoder addresses as
 * arrays; each becomes a byte array of the typed state and the
 * named cells inside it become indices into that array.  The
 * ranges come from the address map's cell boundaries, the
 * init's d4v_fill / d4vc_fill16 plane lengths and the extent
 * of every named cell a table covers - see DEC4_CLEAN_PLAN.md.
 *
 * Regenerate uha04_dec4_state.h with:
 *   python tools/gen_dec4_state.py uha04_dec4_addr.h uha04_dec4.c \
 *       uha04_dec4_state.h d4v uha04_dec4_layout.h
 */
0x44D920 0x451928
0x45193C 0x451B3C
0x451B3C 0x451D58
0x452154 0x453560
0x453560 0x455F60
0x455F60 0x456F78
0x456F78 0x457360
0x457360 0x458778
0x458778 0x458BA0
0x458F94 0x459BA0
0x459BA0 0x459BAD
0x459E38 0x459E58
0x459E58 0x45A25D
0x45A281 0x45AA7A
0x45AA7A 0x45AE84
0x45AF00 0x45AF04
0x45AF10 0x45AF14
0x45AF14 0x45AF28
0x45AF50 0x45AF58
0x45AFAC 0x45AFB8
0x45BA58 0x45C695
