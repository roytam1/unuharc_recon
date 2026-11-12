/* A test that the fill cannot write past the stand-in for the arena's
 * zero gap.
 *
 * The ports' fills (sub_40C9C0 in 0.3, sub_40D570 in 0.4) address the
 * globals, the I/O buffers and the model blocks.  The arena's zero gap -
 * the space between the globals and the model blocks - is stood in for
 * by a few bytes, so a fill that ran into the gap used to hand its whole
 * remaining length to memset() and write past them: the context's own
 * members were overwritten and the heap was corrupted.
 *
 * A fill whose destination is in the gap has to write the gap bytes and
 * stop there.  The arena's zero gap is one long run of zeros, so writing
 * it once is the same as writing it many times.
 *
 * The test covers both core-4 ports: the 0.3 one (whose globals end at
 * 0x445100) and the 0.4 one (whose globals end at 0x45D0C8).
 *
 * Build:  gcc -std=c89 -Wall -Wextra -O2 -I. test_gapfill.c \
 *             uha03_dec4_dg.o uha04_dec4_dg.o -o test_gapfill.exe
 */
#include "uha03_dec4_clean.c"
#include "uha04_dec4_clean.c"

static int check_gap3(dec3w_ctx *c, unsigned long dst, const char *what)
{
    void *io_before = (void *)c->io_mem;
    void *model_before = (void *)c->model_mem;
    unsigned char pad_before[8];
    int i;

    for (i = 0; i < 8; i++)
        pad_before[i] = c->fx_pad[i];
    d3uc_fill(c, 0x200000UL, dst, 0x5AUL);
    if ((void *)c->io_mem != io_before) {
        printf("FAIL 0.3: the fill at %s wrote past the gap (io_mem changed)\n", what);
        return 1;
    }
    if ((void *)c->model_mem != model_before) {
        printf("FAIL 0.3: the fill at %s wrote past the gap (model_mem changed)\n", what);
        return 1;
    }
    for (i = 0; i < 8; i++) {
        if (c->fx_pad[i] != pad_before[i]) {
            printf("FAIL 0.3: the fill at %s wrote past the gap (pad changed)\n", what);
            return 1;
        }
    }
    return 0;
}

static int check_gap4(dec4v_ctx *c, unsigned long dst, const char *what)
{
    void *io_before = (void *)c->io_mem;
    void *model_before = (void *)c->model_mem;
    unsigned char pad_before[8];
    int i;

    for (i = 0; i < 8; i++)
        pad_before[i] = c->fx_pad[i];
    d4v_fill(c, 0x200000UL, dst, 0x5AUL);
    if ((void *)c->io_mem != io_before) {
        printf("FAIL 0.4: the fill at %s wrote past the gap (io_mem changed)\n", what);
        return 1;
    }
    if ((void *)c->model_mem != model_before) {
        printf("FAIL 0.4: the fill at %s wrote past the gap (model_mem changed)\n", what);
        return 1;
    }
    for (i = 0; i < 8; i++) {
        if (c->fx_pad[i] != pad_before[i]) {
            printf("FAIL 0.4: the fill at %s wrote past the gap (pad changed)\n", what);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    void *ctx = NULL;
    dec3w_ctx *c3;
    dec4v_ctx *c4;
    int bad = 0;

    if (uharc_dec4uc_init(&ctx, 0) != 0) {
        printf("FAIL 0.3 init\n");
        return 1;
    }
    c3 = (dec3w_ctx *)ctx;
    /* Three addresses are tried: just past the globals, the gap between
     * the globals and the model blocks, and one past the end of the
     * model blocks. */
    bad |= check_gap3(c3, 0x445100UL, "0x445100");
    bad |= check_gap3(c3, 0x460000UL, "0x460000");
    bad |= check_gap3(c3, 0x4600000UL, "0x4600000");
    uharc_dec4uc_free(ctx);

    ctx = NULL;
    if (uharc_dec4vc_init(&ctx, 24) != 0) {
        printf("FAIL 0.4 init\n");
        return 1;
    }
    c4 = (dec4v_ctx *)ctx;
    bad |= check_gap4(c4, 0x45D0C8UL, "0x45D0C8");
    bad |= check_gap4(c4, 0x500000UL, "0x500000");
    bad |= check_gap4(c4, 0x4600000UL, "0x4600000");
    uharc_dec4vc_free(ctx);

    if (bad)
        return 1;
    printf("the fill stops at the gap\n");
    return 0;
}
