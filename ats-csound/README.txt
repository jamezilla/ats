Sat May  8 17:59:17 PDT 2004

ATScsound Ugens, adapted by Alex Norman (2003) from the phase vocoder csound code by Richard Karpen
This software uses files created by ATS: http://sourceforge.net/projects/atsa/
There is no warranty for this software.
If you find bugs contact me at alexnorman@users.sourceforge.net
There is HTML documentation in the docs directory

To install (pre Csound 5):

1) put ugnorman.c and ugnorman.h into your Csound directory
2) put ugnorman.c ugnorman.h and ugnorman.o in the correct place in your csound Makefile
3) put the following in the correct place in entry.c
  
-----------------------------------
  
#include "ugnorman.h"

void	atsreadset(void*), atsread(void*);
void	atsreadnzset(void*), atsreadnz(void*);
void	atsaddset(void*), atsadd(void*);
void	atsaddnzset(void*), atsaddnz(void*);
void	atssinnoiset(void*), atssinnoi(void*);
void	atsbufreadset(void*), atsbufread(void*);
void	atspartialtapset(void*), atspartialtap(void*);
void	atsinterpreadset(void*), atsinterpread(void*);
void	atscrossset(void*), atscross(void*);
void	atsinfo(void*);

{ "atsread", S(ATSREAD), 3, "kk", "kSi", atsreadset, atsread, NULL},
{ "atsreadnz", S(ATSREADNZ), 3, "k", "kSi", atsreadnzset, atsreadnz, NULL},
{ "atsadd",    S(ATSADD),       5,     "a", "kkSiiopo", atsaddset,      NULL,   atsadd},
{ "atsaddnz",    S(ATSADDNZ),   5,     "a", "kSiop", atsaddnzset,     NULL,   atsaddnz},
{ "atssinnoi",    S(ATSSINNOI),   5,     "a", "kkkkSiop", atssinnoiset,     NULL,   atssinnoi},
{ "atsbufread",    S(ATSBUFREAD),   3,     "", "kkSiop", atsbufreadset, atsbufread, NULL},
{ "atspartialtap",    S(ATSPARTIALTAP),   3,     "kk", "i", atspartialtapset, atspartialtap, NULL},
{ "atsinterpread",    S(ATSINTERPREAD),   3,     "k", "k", atsinterpreadset, atsinterpread, NULL},
{ "atscross",    S(ATSCROSS),   5,     "a", "kkSikkiopo", atscrossset, NULL, atscross},
{ "atsinfo",    S(ATSINFO),   1,     "i", "Si", atsinfo, NULL, NULL},

--------------------------------------
  
4) Then rebuild Csound
