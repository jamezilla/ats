sr	=	44100 
kr	=	44100
ksmps	=	1
nchnls	=	1
instr 1
ktime	line  0, p3, 2.2
aout1	atsaddnz	ktime, "cl.ats", 25
out	aout1*100000
endin
