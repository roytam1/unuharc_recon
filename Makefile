CC=gcc
CFLAGS=-std=c89 -Wall -Wextra -O2
OBJS=uharc06_main.o uharc06.o uha04.o uha03.o uha02.o uhcrypt06.o uhcrypt04.o uharc_dec0.o uharc_dec2.o uharc_dec2_lsb.o uharc_split.o uharc_filter.o uha02_mm.o uharc_dec3.o uha03_dec3.o uha02_dec3.o uha04_dec3.o uha03_dec5.o uha04_dec5.o uha04_dec4.o uha04_dec4_dg.o uha04_dec4_clean.o uha03_dec4.o uha03_dec4_dg.o uha03_dec4_clean.o uharc_dec4.o uharc_dec4_clean.o uharc_dec5.o
DECOBJS=uharc06.o uha04.o uha03.o uha02.o uhcrypt06.o uhcrypt04.o uharc_dec0.o uharc_dec2.o uharc_dec2_lsb.o uharc_split.o uharc_filter.o uha02_mm.o uharc_dec3.o uha03_dec3.o uha02_dec3.o uha04_dec3.o uha03_dec5.o uha04_dec5.o uha04_dec4.o uha04_dec4_dg.o uha04_dec4_clean.o uha03_dec4.o uha03_dec4_dg.o uha03_dec4_clean.o uharc_dec4.o uharc_dec4_clean.o uharc_dec5.o

# Define your targets in a variable so they can be shared
TARGETS=uharc06_unpack.exe hdrtest.exe hdrtest04.exe hdrtest03.exe hdrtest02.exe test_tab.exe test_dec3.exe test_dec4state.exe test_gapfill.exe

all: $(TARGETS)

# Explicit pattern rule: tells make how to compile any .o file from its corresponding .c file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# The tests include the port sources directly, so they have to be rebuilt
# when a port or a state header changes - the pattern rule above only
# knows about the .c file of the target.
test_gapfill.o: uha03_dec4_clean.c uha04_dec4_clean.c uha03_dec4_state.h uha04_dec4_state.h uha03_dec4_addr.h uha04_dec4_addr.h
test_dec4state.o: uha03_dec4_state.h uha04_dec4_state.h

uharc06_unpack.exe: $(OBJS)
	$(CC) -o $@ $(OBJS)

hdrtest.exe: hdrtest.o $(DECOBJS)
	$(CC) -o $@ hdrtest.o $(DECOBJS)

hdrtest04.exe: hdrtest04.o $(DECOBJS)
	$(CC) -o $@ hdrtest04.o $(DECOBJS)

hdrtest03.exe: hdrtest03.o $(DECOBJS)
	$(CC) -o $@ hdrtest03.o $(DECOBJS)

hdrtest02.exe: hdrtest02.o $(DECOBJS)
	$(CC) -o $@ hdrtest02.o $(DECOBJS)

test_tab.exe: test_tab.o $(DECOBJS)
	$(CC) -o $@ test_tab.o $(DECOBJS)

test_dec3.exe: test_dec3.o $(DECOBJS)
	$(CC) -o $@ test_dec3.o $(DECOBJS)

# Self-test for the generated typed core-4 state headers (0.3 / 0.4).
test_dec4state.exe: test_dec4state.o
	$(CC) -o $@ test_dec4state.o

# Self-test for the core-4 fills: they must stop at the stand-in for
# the arena's zero gap (0.3 and 0.4).
test_gapfill.exe: test_gapfill.o uha03_dec4_dg.o uha04_dec4_dg.o
	$(CC) -o $@ test_gapfill.o uha03_dec4_dg.o uha04_dec4_dg.o

clean:
	del *.o $(TARGETS)
