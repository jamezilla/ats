CC = gcc
DEFS = -DLINUX
CFLAGS = -I$(ATSA_LOC) -ggdb -O2 -Wall
GTK_LIBS = `gtk-config --libs`
GTK_CFLAGS = `gtk-config --cflags`
LIBS = -lm $(ATSA_LOC)/atsa.a
ATSA_LOC = ../atsa

.c.o:
	$(CC) -c $(DEFS) $(GTK_CFLAGS) $(CFLAGS) $<

HEADERS = $(ATSA_LOC)/atsa.h
OBJ = atsh.o popup.o my_curve.o atsh-graphics.o ats-header.o about.o atsh-undo.o atsh-files.o synth-funcs.o get_sparams.o list_view.o atsh-curves.o atsh-misc.o progress.o atsh-sel.o atsh-analysis.o

all: 	atsa atsh

atsa:
	cd $(ATSA_LOC) ; make

atsh: $(HEADERS) $(OBJ)
	$(CC) $(OBJ) -o atsh $(LIBS) $(GTK_LIBS)

clean: 
	rm -f atsh *.o

atsaclean:
	cd $(ATSA_LOC) ; make clean

