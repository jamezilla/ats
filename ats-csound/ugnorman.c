/* ATScsound Ugens, adapted by Alex Norman (2003) from the phase vocoder csound code by Richard Karpen
*  uses noise generations fucntions from atsh
*  the following needs to be put in entry.c

#include "ugnorman.h"

void    atsreadset(void*), atsread(void*);
void    atsreadnzset(void*), atsreadnz(void*);
void    atsaddset(void*), atsadd(void*);
void    atsaddnzset(void*), atsaddnz(void*);

taken out for now:
void    atsbufreadset(void*), atsbufread(void*);
void    atscrossset(void*), atscross(void*);
void    atsbufreadnzset(void*), atsbufreadnz(void*);
void    atscrossnzset(void*), atscrossnz(void*);

{ "atsread", S(ATSREAD), 3, "kk", "kSi", atsreadset, atsread, NULL},
{ "atsreadnz", S(ATSREADNZ), 3, "k", "kSi", atsreadnzset, atsreadnz, NULL},
{ "atsadd",    S(ATSADD),       5,     "a", "kkSiiopo", atsaddset,      NULL,   atsadd},
{ "atsaddnz",    S(ATSADDNZ),   5,     "a", "kSiop", atsaddnzset,     NULL,   atsaddnz},

taken out for now:
{ "atsbufread", S(ATSBUFREAD),  3,      "", "kkSiop", atsbufreadset, atsbufread, NULL},
{ "atscross", S(ATSCROSS),      5,      "a", "kkSikkiop", atscrossset,  NULL,   atscross},
{ "atsbufreadnz", S(ATSBUFREADNZ), 3,   "", "kS", atsbufreadnzset, atsbufreadnz, NULL},
{ "atscrossnz", S(ATSCROSSNZ),  5,      "a", "kSikkiop", atscrossnzset, NULL, atscrossnz}

put ugnorman.c ugnorman.h and ugnorman.o in the correct place in your make file as well.

function usage
see:
http://students.washington.edu/~anorman/atsadd.html
and
http://students.washington.edu/~anorman/atsread.html


read functions:

kfreq, kamp	atsread     ktimepnt, ifile, ipartial
kenergy	atsreadnz    ktimepnt, ifile, iband

add functions:

ar      atsadd      ktimepnt, kfmod, iatsfile, ifn, ipartials[, ipartialoffset, ipartialincr, igatefn]
ar      atsaddnz    ktimepnt, iatsfile, ibands[, ibandoffset, ibandincr]

ifn for atsaddnz and atscrossnz needs to be 1 cycle of a cosine wave for now, this will change in future implementations

buf/cross functions:

        atsbufread      ktimepnt, kfmod, iatsfile, ipartials[, ipartialoffset, ipartialincr]
ar      atsadd      ktimepnt, kfmod, iatsfile, ifn, kmyamp, kbufamp, ipartials[, ipartialoffset, ipartialincr]

	atsbufreadnz	ktimepnt, iatsfile
ar	atscrossnz	ktimepnt, iatsfile, ifn, kmyamp, kbufamp, ibands[, ibandoffset, ibandincr]

ifn for atsaddnz and atscrossnz needs to be 1 cycle of a cosine wave for now, this will change in future implementations

*/


#include "cs.h"
#include "ugnorman.h"
#include "oload.h"
#include <math.h>
#include <stdlib.h>

#include "ftgen.h"


#define costabsz 4096
#define ATSA_NOISE_VARIANCE 0.04

extern	float	esr;	//the sampleing rate (i hope)
//extern	int	ksmps;
extern	char	errmsg[];
//extern  int     odebug;

//static	int	pdebug = 0;

// static variables used for atsbufread and atsbufreadnz
static	ATSBUFREAD	*atsbufreadaddr;
static	ATSBUFREADNZ	*atsbufreadaddrnz;


/************************************************************/
/*********  ATSREAD       ***********************************/
/************************************************************/

void FetchPartial(
	double	*input,
	float	*buf,
	float	position,
	int	frameinc,
	int	partialloc
	)
{
	double	*frm0, *frm1;	// 2 frames for interpolation
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	
	frame = (int)position;
	frac = position - frame;
	
	frm0 = input + (frame * frameinc);
	frm1 = frm0 + (int)frameinc;

	buf[0] = frm0[partialloc] + frac * (frm1[partialloc] - frm0[partialloc]);	// calc amplitude
	buf[1] = frm0[partialloc + 1] + frac * (frm1[partialloc + 1 ] - frm0[partialloc + 1]); // calc freq
}

void atsreadset(ATSREAD *p){
	char atsfilname[64];
	MEMFIL *mfp, *ldmemfile(char*);
	ATSSTRUCT *atsh;
	
	
	if (*p->ifileno == sstrcod){
		strcpy(atsfilname, unquote(p->STRARG));
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else sprintf(atsfilname,"ats.%d", (int)*p->ifileno); /* else ats.filnum   */
			   

	if ( (mfp = ldmemfile(atsfilname)) == NULL)
	{
		sprintf(errmsg, "ATSREAD NOT READ");
		initerror(errmsg);
		return;
	}
	atsh = (ATSSTRUCT *)mfp->beginp;
	
	if ( atsh->magic != 123){
		sprintf(errmsg, "Not an ATS file");
		initerror(errmsg);
		return;
	}
	
	// get past the header to the data, point frptr at time 0
	p->frPtr = (double *)atsh + 10;
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;
	
	// check to see if partial is valid
	if( (int)(*p->ipartial) > (int)(atsh->npartials) || (int)(*p->ipartial) <= 0)
	{
		sprintf(errmsg, "PARTIAL OUT OF RANGE");
		initerror(errmsg);
		return;
	}

	// set up partial locations and frame increments
	
	switch ( (int)(atsh->type))
	{
	
		case 1 : 	p->partialloc = 1 + 2 * (*p->ipartial - 1);
				p->frmInc = (int)(atsh->npartials * 2 + 1);
				break;
		
		case 2 : 	p->partialloc = 1 + 3 * (*p->ipartial - 1);
				p->frmInc = (int)(atsh->npartials * 3 + 1);
				break;
				
		case 3 : 	p->partialloc = 1 + 2 * (*p->ipartial - 1);
				p->frmInc = (int)(atsh->npartials * 2 + 26);
				break;
		
		case 4 : 	p->partialloc = 1 + 3 * (*p->ipartial - 1);
				p->frmInc = (int)(atsh->npartials * 3 + 26);
				break;
		default:	sprintf(errmsg, "Type not implemented");
				initerror(errmsg);
				return;
	}
	
	return;
}

void atsread(ATSREAD *p){
	float frIndx;
	float buf[2];
	
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		printf("\n ERROR \n");
		return;
	}
	if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			warning("ATS ktimpnt truncated to last frame");
		}
	}
	FetchPartial(p->frPtr, buf, frIndx, p->frmInc, p->partialloc);
	*p->kamp = buf[0];
	*p->kfreq = buf[1];
}


/************************************************************/
/*********  ATSREADNZ     ***********************************/
/************************************************************/


void FetchNoisebin(
	double	*input,
	float	buf,
	float	position,
	int	frameinc,
	int	nzbinloc
	)
{
	double	*frm0, *frm1;	// 2 frames for interpolation
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	
	frame = (int)position;
	frac = position - frame;
	
	frm0 = input + (frame * frameinc);
	frm1 = frm0 + (int)frameinc;

	buf = (frm0[nzbinloc] + frac * (frm1[nzbinloc] - frm0[nzbinloc]));
}

void atsreadnzset(ATSREADNZ *p){
	char atsfilname[64];
	MEMFIL *mfp, *ldmemfile(char*);
	ATSSTRUCT *atsh;
	
	
	if (*p->ifileno == sstrcod){
		strcpy(atsfilname, unquote(p->STRARG));
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else sprintf(atsfilname,"ats.%d", (int)*p->ifileno); /* else ats.filnum   */
			   

	if ( (mfp = ldmemfile(atsfilname)) == NULL)
	{
		sprintf(errmsg, "ATSREAD NOT READ");
		initerror(errmsg);
		return;
	}
	atsh = (ATSSTRUCT *)mfp->beginp;
	
	if ( atsh->magic != 123){
		sprintf(errmsg, "Not an ATS file");
		initerror(errmsg);
		return;
	}
	
	// get past the header to the data, point frptr at time 0
	p->frPtr = (double *)atsh + 10;
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;
	
	// check to see if partial is valid
	if( (int)(*p->inzbin) > 25 || (int)(*p->inzbin) <= 0)
	{
		sprintf(errmsg, "NOISE BIN OUT OF RANGE");
		initerror(errmsg);
		return;
	}

	// set up noise locations and frame increments
	
	switch ( (int)(atsh->type))
	{
		case 3 : 	p->nzbinloc =  2 * atsh->npartials + *p->inzbin;	//get past the partial data to the noise
				p->frmInc = (int)(atsh->npartials * 2 + 26);
				break;
		
		case 4 : 	p->nzbinloc = 3 * atsh->npartials + *p->inzbin;
				p->frmInc = (int)(atsh->npartials * 3 + 26);
				break;
		default:	sprintf(errmsg, "Type not implemented");
				initerror(errmsg);
                		return;


	}
	
	return;
}

void atsreadnz(ATSREADNZ *p){
	float frIndx;
	float buf;
	
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		printf("\n ERROR \n");
		return;
	}
	if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			warning("ATS ktimpnt truncated to last frame");
		}
	}
	FetchNoisebin(p->frPtr, buf, frIndx, p->frmInc, p->nzbinloc);
	*p->kenergy = buf;
}


/************************************************************/
/*********  ATSADD        ***********************************/
/************************************************************/

void AtsAmpGate(float *, long, FUNC *, float);

void FetchADDPartials(
	double	*input,
	float	*buf,
	float	position,
	int	frameinc,
	int	firstpartial,
	int	partialinc,
	int	npartials
	)
{
	double	*frm0, *frm1;	// 2 frames for interpolation
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	int	partialloc = firstpartial;
	int	i;	// for the for loop
	
	frame = (int)position;
	frac = position - frame;
	
	frm0 = input + (int)(frame * frameinc);
	frm1 = frm0 + (int)frameinc;

	for(i = 0; i < npartials; i++)
	{
		*buf = frm0[partialloc] + frac * (frm1[partialloc] - frm0[partialloc]);	// calc amplitude
		buf++;
		*buf = frm0[partialloc + 1] + frac * (frm1[partialloc + 1 ] - frm0[partialloc + 1]); // calc freq
		buf++;
		partialloc += partialinc;	// get to the next partial
	}
}

void atsaddset(ATSADD *p){
	char atsfilname[64];
	MEMFIL *mfp, *ldmemfile(char*);
	float	*oscphase;
	ATSSTRUCT *atsh;
	FUNC *ftp, *AmpGateFunc;
	int i;	// for indexing for loop
	int memsize;		// memory size for buffer
	    
	if(*p->ifn > 0)
		if ((ftp = ftfind(p->ifn)) == NULL)
			return;
	p->ftp = ftp;
				      
	if(*p->igatefun > 0)
        	if ((AmpGateFunc = ftfind(p->igatefun)) == NULL)
	        	return;
		else
			p->AmpGateFunc = AmpGateFunc;
			       
	
	if (*p->ifileno == sstrcod){
		strcpy(atsfilname, unquote(p->STRARG));
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else sprintf(atsfilname,"ats.%d", (int)*p->ifileno); /* else ats.filnum   */
			   

	if ( (mfp = ldmemfile(atsfilname)) == NULL)
	{
		sprintf(errmsg, "ATS file not read");
		initerror(errmsg);
		return;

	}
	atsh = (ATSSTRUCT *)mfp->beginp;
	
	if ( atsh->magic != 123){
		sprintf(errmsg, "Not an ATS file");
		initerror(errmsg);
		return;
	}
	
	// get past the header to the data, point frptr at time 0
	p->frPtr = (double *)atsh + 10;
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;
	p->MaxAmp = (float)atsh->ampmax;	// store the maxium amplitude
	
		memsize = (int)((*p->iptls) * 3);	// need the number of partials * 3 for freq, amp and phase
	
	if (p->auxch.auxp == NULL || memsize != p->mems) 
	{
		register float *fltp;
		auxalloc((memsize * sizeof(float)), &p->auxch);
		fltp = (float *) p->auxch.auxp;
		p->buf = fltp;
		fltp += (int)((*p->iptls) * 2);
		p->oscphase = fltp;
		
	}
        p->mems=memsize;
		
	// initialize phases of osciallators to zero
	oscphase = p->oscphase;
	for( i = 0; i < *p->iptls; i++)
		*oscphase++ = 0.0f;
	// check to see if partial is valid
	if( (int)(*p->iptloffset + *p->iptls * *p->iptlincr)  > (int)(atsh->npartials) || (int)(*p->iptloffset) < 0)
	{
		sprintf(errmsg, "Partial out of range, max partial is %i", (int)atsh->npartials);
		initerror(errmsg);
		return;

	}

	// set up partial locations and frame increments
	
	switch ( (int)(atsh->type))
	{
	
		case 1 : 	p->firstpartial = 1 + 2 * (*p->iptloffset);
				p->partialinc = 2;
				p->frmInc = (int)(atsh->npartials * 2 + 1);
				break;
		
		case 2 : 	p->firstpartial = 1 + 3 * (*p->iptloffset);
				p->partialinc = 3;
				p->frmInc = (int)(atsh->npartials * 3 + 1);
				break;
				
		case 3 : 	p->firstpartial = 1 + 2 * (*p->iptloffset);
				p->partialinc = 2;
				p->frmInc = (int)(atsh->npartials * 2 + 26);
				break;
		
		case 4 : 	p->firstpartial = 1 + 3 * (*p->iptloffset);
				p->partialinc = 3;
				p->frmInc = (int)(atsh->npartials * 3 + 26);
				break;
		
		default:	sprintf(errmsg, "Type not implemented");
				initerror(errmsg);
				return;
	}
	
	return;
}

void atsadd(ATSADD *p){

	float frIndx;
	float * buf, * ar, amp, cpsp, fract, v1, *ftab;
	FUNC	*ftp;
	long	lobits, phase, inc;
	float *oscphase;
	int	i, nsmps;
	int	numpartials = (int)*p->iptls;
	
	if (p->auxch.auxp == NULL)
	{
		initerror("atsadd: not intialized");
		return;
	}
	
	ftp = p->ftp;		// ftp is a poiter to the ftable
	if (ftp == NULL)
	{
		initerror("atsadd: not intialized");
		return;
	}
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		printf("\n ERROR frame index less than zero\n");
		return;
	}
	if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			warning("ATS ktimpnt truncated to last frame");
		}
	}
	buf = (float *)p->buf;
	
	FetchADDPartials(p->frPtr, buf, frIndx, p->frmInc, p->firstpartial, p->partialinc, (int)*p->iptls);
	
	oscphase = p->oscphase;
	// initialize output to zero
	ar = p->aoutput;
	for (i = 0; i < ksmps; i++)
		*ar++ = 0.0f;
    	
	if(*p->igatefun > 0)
            AtsAmpGate(p->buf, *p->iptls , p->AmpGateFunc, p->MaxAmp);
	    
	for (i = 0; i < numpartials; i++)
	{
		lobits = ftp->lobits;
		amp = (float)p->buf[i * 2];
		phase = (long)*oscphase;
		ar = p->aoutput;	// ar is a pointer to the audio output
		nsmps = ksmps;
		inc = (long)(p->buf[i * 2 + 1] *  sicvt * *p->kfmod);  // put in * kfmod
		do {
			ftab = ftp->ftable + (phase >> lobits);
			v1 = *ftab++;
			fract = (float) PFRAC(phase);
			*ar += (v1 + fract * (*ftab - v1)) * amp;
			ar++;
			phase += inc;
			phase &= PHMASK;
		}
		while (--nsmps);
		*oscphase = (float)phase;
		oscphase++;
	}

}

void AtsAmpGate(		// adaption of PvAmpGate by Richard Karpen
	float   *buf,       /* where to get our mag/freq pairs */
        long    npartials,      /* number of partials we're working with */
	FUNC    *ampfunc,
	float    MaxAmpInData
	)
{
	long    j;
	long    ampindex, funclen, mapPoint;
	funclen = ampfunc->flen;

	for (j=0; j < npartials; ++j) 
	{
		/* use normalized amp as index into table for amp scaling */
		mapPoint = (long)((buf[j*2] / MaxAmpInData) * funclen);
		buf[j*2] *= *(ampfunc->ftable + mapPoint);
		}
}
	
	
/************************************************************/
/*********  ATSADDNZ      ***********************************/
/************************************************************/

// copied directly from atsh synth-funcs.c with names changed so as not to conflict with csound
///////////////////////////////////////////////////////////////////
//randi output random numbers in the range of 1,-1
//getting a new number at frequency freq and interpolating
//the intermediate values.
void randiats_setup(float sr, float freq, RANDIATS *radat)
{
  long int first, second;

  first =random();
  second=random();

  radat->size= (int) (sr / freq) - 1;
  radat->a1  = first;
  radat->a2  = second;
  radat->cnt = 0;

  return;
}
///////////////////////////////////////////////////////////////////
float randiats(RANDIATS *radat)
{
  long int second; 
  float output;
 
  if(radat->cnt == radat->size) { //get a new random value
    radat->a1  = radat->a2; 
    second=random();
    radat->a2  = second;
    radat->cnt = 0;
  }

  output=(float)(((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1;
  radat->cnt++;
  return(1. - ((float)(output /(long int)RAND_MAX) * 2.));
}
///////////////////////////////////////////////////////////////////
float randifats(RANDIATS *radat, float freq)
{
  long int second;
  float output;

  if(radat->cnt == radat->size) { //get a new random value
    radat->a1  = radat->a2;
    second=random();

    radat->a2  = second;
    radat->cnt = 0;
    radat->size= (int) (esr / freq) - 1;
  }

  output=(float)(((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1;
  radat->cnt++;

  return(1. - ((float)(output /(long int)RAND_MAX) * 2.));
}


void FetchNoise(
	double	*input,
	double	*buf,
	float	position,
	int	frameinc,
	int	firstband
	)
{
	double	*frm0, *frm1;	// 2 frames for interpolation
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	int	firstbandloc = firstband;
	int	i;	// for the for loop
	
	frame = (int)position;
	frac = position - frame;
	
	frm0 = input + (int)(frame * frameinc);
	frm1 = frm0 + (int)frameinc;

	for(i = 0; i < 25; i++)
	{
		buf[i] = (frm0[firstbandloc + i] + frac * (frm1[firstbandloc + i] - frm0[firstbandloc + i]));	// calc amplitude
	}
}

void atsaddnzset(ATSADDNZ *p){
	char atsfilname[64];
	MEMFIL *mfp, *ldmemfile(char*);
	ATSSTRUCT *atsh;
	register atsnzAUX *fltp;
	int i;	// for indexing for loop
	short rand;
	
	
	if (*p->ifileno == sstrcod){
		strcpy(atsfilname, unquote(p->STRARG));
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else sprintf(atsfilname,"ats.%d", (int)*p->ifileno); /* else ats.filnum   */
			   

	if ( (mfp = ldmemfile(atsfilname)) == NULL)
	{
		sprintf(errmsg, "ATSREAD NOT READ");
		initerror(errmsg);
		return;
	}
	atsh = (ATSSTRUCT *)mfp->beginp;
	
	if ( atsh->magic != 123){
		sprintf(errmsg, "Not an ATS file");
		initerror(errmsg);
		return;
	}
	
	
	// get past the header to the data, point frptr at time 0
	p->frPtr = (double *)atsh + 10;
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;
	//p->MaxAmp = (float)atsh->ampmax;	// store the maxium amplitude
	p->winsize = (float)atsh->winsz;
	
	if (p->auxch.auxp == NULL ) 
	{
		auxalloc((long)sizeof(atsnzAUX), &(p->auxch));
		fltp = (atsnzAUX *)(p->auxch.auxp);
		p->buf = fltp->buf;
		p->phaseinc = fltp->phaseinc;
		p->nfreq = fltp->nfreq;
		p->randinoise = fltp->randinoise;
	}

	
	// save bandwidths for creating noise bands
	p->nfreq[0] = 100;
	p->nfreq[1] = 100;
	p->nfreq[2] = 100;
	p->nfreq[3] = 100;
	p->nfreq[4] = 110;
	p->nfreq[5] = 120;
	p->nfreq[6] = 140;
	p->nfreq[7] = 150;
	p->nfreq[8] = 160;
	p->nfreq[9] = 190;
	p->nfreq[10] = 210;
	p->nfreq[11] = 240;
	p->nfreq[12] = 280;
	p->nfreq[13] = 320;
	p->nfreq[14] = 380;
	p->nfreq[15] = 450;
	p->nfreq[16] = 550;
	p->nfreq[17] = 700;
	p->nfreq[18] = 900;
	p->nfreq[19] = 1100;
	p->nfreq[20] = 1300;
	p->nfreq[21] = 1800;
	p->nfreq[22] = 2500;
	p->nfreq[23] = 3500;
	p->nfreq[24] = 4500;

	// initialize frequencies to modulate noise by
	p->phaseinc[0] = TWOPI * 50.0 / esr;
	p->phaseinc[1] = TWOPI * 150.0/ esr;
	p->phaseinc[2] = TWOPI * 250.0/ esr;
	p->phaseinc[3] = TWOPI * 350.0/ esr;
	p->phaseinc[4] = TWOPI * 455.0/ esr;
	p->phaseinc[5] = TWOPI * 570.0/ esr;
	p->phaseinc[6] = TWOPI * 700.0/ esr;
	p->phaseinc[7] = TWOPI * 845.0/ esr;
	p->phaseinc[8] = TWOPI * 1000.0/ esr;
	p->phaseinc[9] = TWOPI * 1175.0/ esr;
	p->phaseinc[10] = TWOPI * 1375.0/ esr;
	p->phaseinc[11] = TWOPI * 1600.0/ esr;
	p->phaseinc[12] = TWOPI * 1860.0/ esr;
	p->phaseinc[13] = TWOPI * 2160.0/ esr;
	p->phaseinc[14] = TWOPI * 2510.0/ esr;
	p->phaseinc[15] = TWOPI * 2925.0/ esr;
	p->phaseinc[16] = TWOPI * 3425.0/ esr;
	p->phaseinc[17] = TWOPI * 4050.0/ esr;
	p->phaseinc[18] = TWOPI * 4850.0/ esr;
	p->phaseinc[19] = TWOPI * 5850.0/ esr;
	p->phaseinc[20] = TWOPI * 7050.0/ esr;
	p->phaseinc[21] = TWOPI * 8600.0/ esr;
	p->phaseinc[22] = TWOPI * 10750.0/ esr;
	p->phaseinc[23] = TWOPI * 13750.0/ esr;
	p->phaseinc[24] = TWOPI * 17750.0/ esr;

	// initialize band limited noise parameters
	for(i = 0; i < 25; i++)
	{
		randiats_setup(esr, p->nfreq[i], &(p->randinoise[i]));
	}
	// check to see if bands are valid
	if( (int)(*p->ibandoffset + *p->ibands * *p->ibandincr)  > 25 || (int)(*p->ibandoffset) < 0)
	{
		sprintf(errmsg, "Bands out of range, the are only 25 bands");
		initerror(errmsg);
		return;
	}
	// set up partial locations and frame increments
	
	switch ( (int)(atsh->type))
	{
		case 3 : 	p->firstband = 1 + 2 * (int)(atsh->npartials);
				p->frmInc = (int)(atsh->npartials * 2 + 26);
				break;
		
		case 4 : 	p->firstband = 1 + 3 * (int)(atsh->npartials);
				p->frmInc = (int)(atsh->npartials * 3 + 26);
				break;
		
		default:	sprintf(errmsg, "Type either has no noise or is not implemented (only type 3 and 4 work now)");
				initerror(errmsg);
				return;
	}
	return;
}

void atsaddnz(ATSADDNZ *p){
	float frIndx;
	float *ar, amp, fract, v1, *ftab;
	double * buf;
	int phase;
	float inc;	// a value to increment a phase index of the cosine by
	int	i, nsmps;
	int	numbands = (int)*p->ibands;
	int synthme;
	int nsynthed;
		
	if (p->auxch.auxp == NULL)
	{
		initerror("atsadd: not intialized");
		return;
	}
	
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		initerror("ERROR ATS frame index less than zero");
		return;
	}
	if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			warning("ATS ktimpnt truncated to last frame");
		}
	}
	buf = (double *)p->buf;
	
	FetchNoise(p->frPtr, buf, frIndx, p->frmInc, p->firstband);
	
	// initialize output to zero
	ar = p->aoutput;

	for (i = 0; i < ksmps; i++)
		*ar++ = 0.0f;
   	
	synthme = (int)*p->ibandoffset;
	nsynthed = 0;
	for (i = 0; i < 25; i++)
	{	
		// do we even have to synthesize it?
		if(i == synthme && nsynthed < (int)*p->ibands)
		{
			// synthesize cosine
			amp = (float)sqrt((p->buf[i] / ((p->winsize) * (float)ATSA_NOISE_VARIANCE) ) );
			phase = p->oscphase;
			ar = p->aoutput;
			nsmps = ksmps;
			inc = p->phaseinc[i];
			do
			{
				// lineraly interpolate the cosine
				*ar += cos(inc * phase) * amp * randiats(&(p->randinoise[i]));
				phase += 1;
				ar++;
			}
			while(--nsmps);
			// make sure that the phase doesn't over flow
			/*
			while (phase >= costabsz)
				phase = phase - costabsz;
			*/
			nsynthed++;
			synthme += (int)*p->ibandincr;
		}	
	}
	p->oscphase = phase;
}


