CC=gcc
CFLAGS=-std=c89 -Wall -Wextra -O2
OBJS=uharc06_main.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o

all: uharc06_unpack.exe hdrtest.exe test_tab.exe test_dec3.exe

uharc06_unpack.exe: $(OBJS)
	$(CC) -o $@ $(OBJS)

hdrtest.exe: hdrtest.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o
	$(CC) -o $@ hdrtest.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o

test_tab.exe: test_tab.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o
	$(CC) -o $@ test_tab.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o

test_dec3.exe: test_dec3.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o
	$(CC) -o $@ test_dec3.o uharc06.o uharc_dec0.o uharc_dec2.o uharc_dec3.o uharc_dec4.o uharc_dec5.o

clean:
	del *.o *.exe
