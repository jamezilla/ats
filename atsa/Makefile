CC = gcc
DEFS = -DLINUX
CFLAGS = -I. -g -O2
LIBS = ./atsa-sndlib/sndlib.a
NOGUI_LIBS = -lm atsa.a
LD = ld
LD_FLAGS = -r

.c.o:
	$(CC) -c $(DEFS) $(CFLAGS) $<

HEADERS = atsa.h
OBJ = atsa.o critical-bands.o other-utils.o peak-detection.o peak-tracking.o save-load-sound.o tracker.o utilities.o residual.o residual-analysis.o

all: sndlib atsa

atsa: $(HEADERS) $(OBJ)
	$(LD) $(LD_FLAGS) $(OBJ) -o atsa.a $(LIBS)

nogui:	atsa
	$(CC) $(DEFS) atsa-nogui.c -o atsa $(NOGUI_LIBS)

sndlib:
	cd atsa-sndlib ; ./configure --with-doubles ; make

clean:
	rm -f *.o atsa.a atsa ; cd atsa-sndlib ; make clean
