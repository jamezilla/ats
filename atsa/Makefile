CC = gcc
LD = ld
CFLAGS = -I. -ggdb -O2 -Wall

#you might need to put -lm here as well
LIBS = ./atsa-sndlib/sndlib.a #-lfftw3
#LIBS = ./atsa-sndlib/sndlib.a /usr/local/lib/libfftw3.a

#if you have fftw installed, then -DFFTW
DEFS = -DLINUX #-DFFTW

#if you have/use gls you need to also include -lgsl -lgslcblas in this line
NOGUI_LIBS = -lm atsa.a
#if you have fftw, you need to list the directory of libfftw3.a after -L
LD_FLAGS = -r -L/usr/local/lib

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
	rm -f *.o atsa.a atsa

sndlibclean:
	cd atsa-sndlib ; make clean
