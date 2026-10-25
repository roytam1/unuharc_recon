CC=gcc
CFLAGS=-std=c89 -Wall -Wextra -O2
OBJS=uharc06_main.o uharc06.o uha04.o uha03.o uha02.o uhcrypt06.o uhcrypt04.o uharc_dec0.o uharc_dec2.o uharc_dec2_lsb.o uharc_split.o uharc_filter.o uharc_dec3.o uha03_dec3.o uha02_dec3.o uha04_dec3.o uha03_dec5.o uha04_dec5.o uharc_dec4.o uharc_dec5.o
DECOBJS=uharc06.o uha04.o uha03.o uha02.o uhcrypt06.o uhcrypt04.o uharc_dec0.o uharc_dec2.o uharc_dec2_lsb.o uharc_split.o uharc_filter.o uharc_dec3.o uha03_dec3.o uha02_dec3.o uha04_dec3.o uha03_dec5.o uha04_dec5.o uharc_dec4.o uharc_dec5.o

# Define your targets in a variable so they can be shared
TARGETS=uharc06_unpack.exe hdrtest.exe hdrtest04.exe hdrtest03.exe hdrtest02.exe test_tab.exe test_dec3.exe

all: $(TARGETS)

# Explicit pattern rule: tells make how to compile any .o file from its corresponding .c file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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

clean:
	del *.o $(TARGETS)
