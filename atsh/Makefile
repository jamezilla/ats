CC = gcc
DEFS = -DLINUX
CFLAGS = -I. -g -O2
SRC = atsh.c popup.c my_curve.c atsh-graphics.c ats-header.c about.c atsh-undo.c atsh-files.c synth-funcs.c get_sparams.c list_view.c atsh-curves.c atsh-misc.c progress.c atsh-sel.c atsh-analysis.c
GTK_LIBS = `gtk-config --cflags` `gtk-config --libs`
LIB = -lm ./atsa/atsa.a

all: 	makeatsa atsh

makeatsa:
	cd atsa ; make

atsh:   
	$(CC) $(GTK_LIBS) $(DEFS) $(SRC) -o atsh $(CFLAGS) $(LIB)

clean: 
	rm -f atsh atsa/*.o



