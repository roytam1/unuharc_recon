/* A test that the fixed-region fill cannot write past the one-byte gap
 * stand-in.
 *
 * The port's fill (sub_40C9C0 in 0.3) addresses the fixed region, the
 * I/O buffers and the model blocks.  The arena's zero gap - the space
 * between the globals and the model blocks - is stood in for by a
 * single byte, so a fill that ran into the gap used to hand its whole
 * remaining length to memset() and write past that byte: the context's
 * own members were overwritten and the heap was corrupted.
 *
 * A fill whose destination is in the gap has to write the gap byte and
 * stop there.  The arena's zero gap is one long run of zeros, so
 * writing it once is the same as writing it many times.
 *
 * Build:  gcc -std=c89 -Wall -Wextra -O2 -I. test_gapfill.c \
 *             uha03_dec4_dg.o -o test_gapfill.exe
 */
#include "uha03_dec4_clean.c"

int main(void)
{
    void *ctx = NULL;
    dec3w_ctx *c;
    void *io_before;
    void *model_before;
    void *fixed_before;
    int bad = 0;

    if (uharc_dec4uc_init(&ctx, 0) != 0) {
        printf("FAIL init\n");
        return 1;
    }
    c = (dec3w_ctx *)ctx;

    io_before = (void *)c->io_mem;
    model_before = (void *)c->model_mem;
    fixed_before = (void *)c->fixed_mem;

    /* a fill that starts in the gap and runs to the end of the address
     * space must write the gap byte and stop there */
    d3uc_fill(c, 0x200000UL, 0x468000UL, 0x5AUL);

    if ((void *)c->io_mem != io_before) {
        printf("FAIL the fill wrote past the gap byte (io_mem changed)\n");
        bad = 1;
    }
    if ((void *)c->model_mem != model_before) {
        printf("FAIL the fill wrote past the gap byte (model_mem changed)\n");
        bad = 1;
    }
    if ((void *)c->fixed_mem != fixed_before) {
        printf("FAIL the fill wrote past the gap byte (fixed_mem changed)\n");
        bad = 1;
    }

    uharc_dec4uc_free(ctx);
    if (bad)
        return 1;
    printf("the fill stops at the gap byte\n");
    return 0;
}
