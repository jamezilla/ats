/* ATScsound Ugens, adapted by Alex Norman (2003) from the phase vocoder csound code by Richard Karpen
*  uses noise generations fucntions from atsh
*  the following needs to be put in entry.c

#include "ugnorman.h"

void    atsreadset(void*), atsread(void*);
void    atsreadnzset(void*), atsreadnz(void*);
void    atsaddset(void*), atsadd(void*);

taken out for now:
void    atsaddnzset(void*), atsaddnz(void*);
void    atsbufreadset(void*), atsbufread(void*);
void    atscrossset(void*), atscross(void*);
void    atsbufreadnzset(void*), atsbufreadnz(void*);
void    atscrossnzset(void*), atscrossnz(void*);

{ "atsread", S(ATSREAD), 3, "kk", "kSi", atsreadset, atsread, NULL},
{ "atsreadnz", S(ATSREADNZ), 3, "k", "kSi", atsreadnzset, atsreadnz, NULL},
{ "atsadd",    S(ATSADD),       5,     "a", "kkSiiopo", atsaddset,      NULL,   atsadd},

taken out for now:
{ "atsaddnz",    S(ATSADDNZ),   5,     "a", "kSiop", atsaddnzset,     NULL,   atsaddnz},
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

kfreq, kamp     atsread     ktimepnt, ifile, ipartial
kenergy atsreadnz    ktimepnt, ifile, iband

add functions:

ar      atsadd      ktimepnt, kfmod, iatsfile, ifn, ipartials[, ipartialoffset, ipartialincr, igatefn]
ar      atsaddnz    ktimepnt, iatsfile, ibands[, ibandoffset, ibandincr]

buf/cross functions:

        atsbufread      ktimepnt, kfmod, iatsfile, ipartials[, ipartialoffset, ipartialincr]
ar      atsadd      ktimepnt, kfmod, iatsfile, ifn, kmyamp, kbufamp, ipartials[, ipartialoffset, ipartialincr]

        atsbufreadnz    ktimepnt, iatsfile
ar      atscrossnz      ktimepnt, iatsfile, ifn, kmyamp, kbufamp, ibands[, ibandoffset, ibandincr]

*/


/* PROBLEM? for some reason on my system fopen causes seg fault if the file does not exist */

#include "cs.h"
// Use the below instead of the above if using as a plugin
//#include "csdl.h"
#include "ugnorman.h"
//#include "oload.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


#define costabsz 4096
#define ATSA_NOISE_VARIANCE 0.04


//extern	float	esr;	//the sampleing rate (i hope)
//extern	int	ksmps;
//extern	char	errmsg[];
//extern  int     odebug;

//static	int	pdebug = 0;

// static variables used for atsbufread and atsbufreadnz
static	ATSBUFREAD	*atsbufreadaddr;
static	ATSBUFREADNZ	*atsbufreadaddrnz;

int readdatafile(const char *, ATSFILEDATA *);

void myiniterror(char *s)
{
	fprintf(stderr,"INIT ERROR: %s\n", s);
	exit(1);
}


/************************************************************/
/*********  ATSREAD       ***********************************/
/************************************************************/

void FetchPartial(
	ATS_DATA_LOC **input,
	float	*buf,
	float	position
	)
{
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	
	frame = (int)position;
	frac = position - frame;
	
	buf[0] = (float)(input[frame]->amp + frac * (input[frame + 1]->amp - input[frame]->amp));	// calc amplitude
	buf[1] = (float)(input[frame]->freq + frac * (input[frame + 1]->freq - input[frame]->freq)); // calc freq
}

void atsreadset(ATSREAD *p){
	char atsfilname[MAXNAME];
	ATSSTRUCT * atsh;
	int i, frmInc, partialloc;
	double * temppnt;
	
	/* copy in ats file name */
	if (*p->ifileno == SSTRCOD)
	{
		strcpy(atsfilname, p->STRARG);
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else 
		sprintf(atsfilname,"ats.%d", (int)*p->ifileno);
	
	p->atsmemfile = ldmemfile(atsfilname);
	
	// allocate space so we can store the data for later use
	if(p->auxch.auxp == NULL || strcmp(p->filename, atsfilname) != 0)
	{
		// copy in the file name
		if(p->filename != NULL)
		{
			mfree(p->filename);
			p->filename = NULL;
		}
		p->filename = (char *)mmalloc(sizeof(char) * strlen(atsfilname));
		strcpy(p->filename, atsfilname);
		
		//point the header pointer at the header data
		atsh = (ATSSTRUCT *)p->atsmemfile->beginp;
	        
		//make sure that this is an ats file
		if (atsh->magic != 123)
		{
        	        sprintf(errmsg, "ATSREAD: either %s is not an ATS file or the byte endianness is wrong", atsfilname);
                	initerror(errmsg);
                	return;
        	}

		auxalloc(((int)(atsh->nfrms) * sizeof(ATS_DATA_LOC *)), &p->auxch);
		p->datap = (ATS_DATA_LOC **) (p->auxch.auxp);
		
		p->maxFr = atsh->nfrms - 1;
		p->timefrmInc = atsh->nfrms / atsh->dur;
	}
	else
	{
		//point the header pointer at the header data
		atsh = (ATSSTRUCT *)p->atsmemfile->beginp;
	}
		
	// check to see if partial is valid
	if( (int)(*p->ipartial) > (int)(atsh->npartials) || (int)(*p->ipartial) <= 0)
	{
		sprintf(errmsg, "ATSREAD: partial %i out of range, max allowed is %i", (int)(*p->ipartial), (int)(atsh->npartials));
		initerror(errmsg);
		return;
	}

	// point the data pointer to the correct partial
	temppnt = (double *)(p->atsmemfile->beginp + sizeof(ATSSTRUCT));

        switch ( (int)(atsh->type))
        {

                case 1 :        partialloc = 1 + 2 * (*p->ipartial - 1);
                                frmInc = (int)(atsh->npartials * 2 + 1);
                                break;

                case 2 :        partialloc = 1 + 3 * (*p->ipartial - 1);
                                frmInc = (int)(atsh->npartials * 3 + 1);
                                break;

                case 3 :        partialloc = 1 + 2 * (*p->ipartial - 1);
                                frmInc = (int)(atsh->npartials * 2 + 26);
                                break;

                case 4 :        partialloc = 1 + 3 * (*p->ipartial - 1);
                                frmInc = (int)(atsh->npartials * 3 + 26);
                                break;
                default:        sprintf(errmsg, "Type not implemented");
                                initerror(errmsg);
                                return;
        }
	for(i = 0; i < (int)(atsh->nfrms); i++)
	{
		p->datap[i] = (ATS_DATA_LOC *)(temppnt + partialloc);
		temppnt = temppnt + frmInc;
	}
	
	p->prFlg = 1;	// true

	return;
}

void atsread(ATSREAD *p){
	float frIndx;
	float buf[2];
	
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		frIndx = 0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			fprintf(stderr,"ATSREAD: only positive time pointer values allowed, setting to zero\n");
		}
		return;
	}
	else if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			fprintf(stderr,"ATSREAD: timepointer out of range, truncated to last frame\n");
		}
	}
	else
		p->prFlg = 1;
	
	FetchPartial(p->datap, buf, frIndx);
	*p->kamp = buf[0];
	*p->kfreq = buf[1];
}

/*
 * ATSREADNOISE
 */
void FetchNzBand(
	double *input,
	float	*buf,
	float	position
	)
{
	float	frac;		// the distance in time we are between frames
	int	frame;		// the time of the first frame
	
	frame = (int)position;
	frac = position - frame;

	*buf = (float)(input[frame] + frac * (input[frame + 1] - input[frame]));	// calc energy
}

void atsreadnzset(ATSREADNZ *p){
	char atsfilname[MAXNAME];
	ATSSTRUCT * atsh = NULL;
	int i;
	p->filedata.atsdata = NULL;
	p->filedata.atsdatanoise = NULL;

	/* copy in ats file name */
	if (*p->ifileno == SSTRCOD)
	{
		strcpy(atsfilname, p->STRARG);
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else 
		sprintf(atsfilname,"ats.%d", (int)*p->ifileno);
	
	
	if (readdatafile(atsfilname, &(p->filedata)) == 1)
	{
		sprintf(errmsg, "ATSREADNZ: FILE NOT READ\n");
		myiniterror(errmsg);
		return;
	}
	
	//test to see if the file actually contains noise data
	if (p->filedata.atsdatanoise == NULL)
	{
		sprintf(errmsg, "ATSREADNZ: This ATS file does not contain Noise Data\n");
		myiniterror(errmsg);
		if(p->filedata.atsdata != NULL)
		{
			for(i = 0; i < (int)(atsh->nfrms); i++)
				free(p->filedata.atsdata[i]);
			free(p->filedata.atsdata);
		}
		return;
	}

	//assign a local pointer to the header data so it's easy to deal with
	atsh = &(p->filedata.atshead);
	// allocate space so we can store the data for later use
	auxalloc(((int)(atsh->nfrms) * sizeof(double)), &p->auxch);
	p->datap = (double *) p->auxch.auxp;
	
	/* test */
	//p->datap = (double *) malloc(sizeof(double) * (int)(atsh->nfrms));
	
	// copy the data from the data read in to the newly allocated space
	for(i = 0; i < atsh->nfrms; i++)
	{
		p->datap[i] = (p->filedata.atsdatanoise)[i][(int)(*p->inzbin) - 1];
	}
	//free the space used when we read in the data
	if(p->filedata.atsdatanoise != NULL)
	{
		for(i = 0; i < (int)(atsh->nfrms); i++)
			free(p->filedata.atsdatanoise[i]);
		free(p->filedata.atsdatanoise);
	}
	if(p->filedata.atsdata != NULL)
	{
		for(i = 0; i < (int)(atsh->nfrms); i++)
			free(p->filedata.atsdata[i]);
		free(p->filedata.atsdata);
	}
		
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;

	// check to see if band is valid
	if( (int)(*p->inzbin) > 25 || (int)(*p->inzbin) <= 0)
	{
		sprintf(errmsg, "ATSREADNZ: band %i out of range, 1-25 are the valid band values\n", (int)(*p->inzbin));
		myiniterror(errmsg);
		return;
	}
	
	return;
}

void atsreadnz(ATSREADNZ *p){
	float frIndx;
	float buf;
	
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
	{
		frIndx = 0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			fprintf(stderr,"ATSREADNZ: only positive time pointer values allowed, setting to zero\n");
		}
		return;
	}
	else if (frIndx >= p->maxFr)	// if we're trying to get frames past where we have data
	{
		frIndx = (float)p->maxFr - 1.0;
		if (p->prFlg)
		{
			p->prFlg = 0;	// set to false
			fprintf(stderr,"ATSREADNZ: timepointer out of range, truncated to last frame\n");
		}
	}
	else
		p->prFlg = 1;
	FetchNzBand(p->datap, &buf, frIndx);
	*p->kenergy = buf;
}

/*
 * ATSADD
 */
void FetchADDPartials(ATS_DATA_LOC *, ATS_DATA_LOC *, float, int);
void AtsAmpGate(ATS_DATA_LOC *, int, FUNC *, double);

void atsaddset(ATSADD *p){
	char atsfilname[MAXNAME];
	ATSSTRUCT * atsh = NULL;
        FUNC *ftp, *AmpGateFunc; 
	int i, j;
	p->filedata.atsdata = NULL;
	p->filedata.atsdatanoise = NULL;
	
	/* copy in ats file name */

	if (*p->ifileno == SSTRCOD)
	{
		strcpy(atsfilname, p->STRARG);
	}
    	else if ((long)*p->ifileno < strsmax && strsets != NULL && strsets[(long)*p->ifileno])
		strcpy(atsfilname, strsets[(long)*p->ifileno]);
	else 
		sprintf(atsfilname,"ats.%d", (int)*p->ifileno);
	
	// read in data file
	if (readdatafile(atsfilname, &(p->filedata)) == 1)
	{
		sprintf(errmsg, "ATSADD: FILE NOT READ\n");
		myiniterror(errmsg);
		return;
	}
	
        // set up function table for synthesis
        if ((ftp = ftfind(p->ifn)) == NULL){
		sprintf(errmsg, "ATSADD: Function table number for synthesis waveform not valid\n");
		myiniterror(errmsg);
                return;
	}
        p->ftp = ftp;

	// set up gate function table
        if(*p->igatefun > 0){
                if ((AmpGateFunc = ftfind(p->igatefun)) == NULL)
		{
			sprintf(errmsg, "ATSADD: Gate Function table number not valid\n");
			myiniterror(errmsg);
                        return;
		}
                else
                        p->AmpGateFunc = AmpGateFunc;
        }
	
	//assign a local pointer to the header data so it's easy to deal with
	atsh = &(p->filedata.atshead);
	
	// make sure partials are in range
        if( (int)(*p->iptloffset + *p->iptls * *p->iptlincr)  > (int)(atsh->npartials) || (int)(*p->iptloffset) < 0)
        {
                sprintf(errmsg, "ATSADD: Partial(s) out of range, max partial allowed is %i", (int)atsh->npartials);
                initerror(errmsg);
		//free the space used when we read in the data
		if(p->filedata.atsdatanoise != NULL)
		{
			for(i = 0; i < (int)(atsh->nfrms); i++)
				free(p->filedata.atsdatanoise[i]);
			free(p->filedata.atsdatanoise);
		}
		if(p->filedata.atsdata != NULL)
		{
			for(i = 0; i < (int)(atsh->nfrms); i++)
				free(p->filedata.atsdata[i]);
			free(p->filedata.atsdata);
		}
                return;
        }
	
	// allocate space so we can store the data for later use
        if (p->auxch.auxp == NULL)
		auxalloc(((1 + (int)(atsh->nfrms)) * (int)(*p->iptls) * sizeof(ATS_DATA_LOC) + (int)(*p->iptls) * sizeof(double)), &p->auxch);
	
	p->datap = (ATS_DATA_LOC *) p->auxch.auxp;
	p->buf = (p->datap + (int)(atsh->nfrms)*(int)(*p->iptls));	//get past the last partial in the last frame of the data pointer;
	p->oscphase = (double *)(p->buf + (int)(*p->iptls));
	
	/* test */
	/*
	p->datap = (ATS_DATA_LOC *) malloc(sizeof(ATS_DATA_LOC) * (int)(*p->iptls) * (int)(atsh->nfrms));
	p->buf = (ATS_DATA_LOC *) malloc(sizeof(ATS_DATA_LOC) * (int)(*p->iptls));
	p->oscphase = (double *) malloc(sizeof(double) * (int)(*p->iptls));
	*/
	
	// initilize the phase of the oscilators
	for(i = 0; i < (int)(*p->iptls); i++)
		(p->oscphase)[i] = 0.0;
	
	// copy the data from the data read in to the newly allocated space and initilize the phase of the oscilators
	for(i = 0; i < atsh->nfrms; i++)
	{
		for(j = 0; j < (int)(*p->iptls); j++)
		{
			p->datap[j + i * (int)(*p->iptls)].amp = (p->filedata.atsdata)[i][(int)(*p->iptloffset - 1 + j * *p->iptlincr)].amp;
			p->datap[j + i * (int)(*p->iptls)].freq = (p->filedata.atsdata)[i][(int)(*p->iptloffset - 1 + j * *p->iptlincr)].freq;
		}
	}
	
	//free the space used when we read in the data
	if(p->filedata.atsdatanoise != NULL)
	{
		for(i = 0; i < (int)(atsh->nfrms); i++)
			free(p->filedata.atsdatanoise[i]);
		free(p->filedata.atsdatanoise);
	}
	if(p->filedata.atsdata != NULL)
	{
		for(i = 0; i < (int)(atsh->nfrms); i++)
			free(p->filedata.atsdata[i]);
		free(p->filedata.atsdata);
	}
		
	p->prFlg = 1;	// true
	p->maxFr = atsh->nfrms - 1;
	p->timefrmInc = atsh->nfrms / atsh->dur;
        p->MaxAmp = atsh->ampmax;        // store the maxium amplitude
	
	return;
}

void atsadd(ATSADD *p){

        float	frIndx;
        float	* ar, amp, fract, v1, *ftab;
        FUNC    *ftp;
        long    lobits, phase, inc;
        double *oscphase;
        int     i, nsmps = ksmps;
        int     numpartials = (int)*p->iptls;
	ATS_DATA_LOC * buf;
	buf = p->buf;

        if (p->auxch.auxp == NULL)
        {
		myiniterror("ATSADD: not intialized");
                return;
        }

        ftp = p->ftp;           // ftp is a poiter to the ftable
        if (ftp == NULL)
        {
                myiniterror("ATSADD: not intialized");
                return;
        }
	
	// make sure time pointer is within range
	if ( (frIndx = *(p->ktimpnt) * p->timefrmInc) < 0 )
        {
		frIndx = 0;
		if (p->prFlg)
		{
                	p->prFlg = 0;
			sprintf(errmsg, "ATSADD: only positive time pointer values are allowed, setting to zero\n");
			myiniterror(errmsg);
		}
        }
        else if (frIndx >= p->maxFr) // if we're trying to get frames past where we have data
        {
                frIndx = (float)p->maxFr - 1.0;
                if (p->prFlg)
                {
                        p->prFlg = 0;   // set to false
			sprintf(errmsg, "ATSADD: time pointer out of range, truncating to last frame\n");
			myiniterror(errmsg);
                }
        }
	else
		p->prFlg = 1;
        FetchADDPartials(p->datap, buf, frIndx, (int)*p->iptls);
        oscphase = p->oscphase;
        // initialize output to zero
        ar = p->aoutput;
        for (i = 0; i < nsmps; i++)
                ar[i] = 0.0f;

        if(*p->igatefun > 0)
            AtsAmpGate(buf, *p->iptls , p->AmpGateFunc, p->MaxAmp);

        for (i = 0; i < numpartials; i++)
        {
                lobits = ftp->lobits;
                amp = (float)p->buf[i].amp;
                phase = (long)*oscphase;
                ar = p->aoutput;        // ar is a pointer to the audio output
                nsmps = ksmps;
                inc = (long)(p->buf[i].freq *  sicvt * *p->kfmod);  // put in * kfmod
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
                *oscphase = (double)phase;
                oscphase++;
        }
}

void FetchADDPartials(
        ATS_DATA_LOC	*input,
        ATS_DATA_LOC	*buf,
        float	position,
        int	npartials
        )
{
        float   frac;           // the distance in time we are between frames
        int     frame1, frame2;
        int     i;      // for the for loop

        frame1 = (int)position;
        frac = position - frame1;
	frame1 = frame1 * npartials;	//the actual index of the array (a 1d array faking a 2d array)
	frame2 = frame1 + npartials;	//get to the next frame
	
        for(i = 0; i < npartials; i++)
        {
                buf[i].amp = input[frame1 + i].amp + frac * (input[frame2 + i].amp - input[frame1 + i].amp); // calc amplitude
                buf[i].freq = input[frame1 + i].freq + frac * (input[frame2 + i].freq - input[frame1 + i].freq); // calc freq
        }
}

void AtsAmpGate(                // adaption of PvAmpGate by Richard Karpen
        ATS_DATA_LOC   *buf,       /* where to get our mag/freq pairs */
        int    npartials,      /* number of partials we're working with */
        FUNC    *ampfunc,
        double    MaxAmpInData
        )
{
        int    j;
        long    funclen, mapPoint;
        funclen = ampfunc->flen;

        for (j=0; j < npartials; ++j)   
        {
                /* use normalized amp as index into table for amp scaling */
                mapPoint = (long)((buf[j].amp / MaxAmpInData) * funclen);
                buf[j].amp *= (double)*(ampfunc->ftable + mapPoint);
	}
}


/*
 *
 * readdatafile
 * reads data out of atsfile "filename"
 */
int readdatafile(const char * filename, ATSFILEDATA *p)
{
	FILE * fin;
	double temp;
	int i, j;	//index for loops
	ATSSTRUCT * atshead = &(p->atshead);
	fin = NULL;

	
	if( (fin = fopen(filename,"rb")) == NULL){
        	return 1;
	}
	
        // read the header
	fread(atshead,sizeof(ATSSTRUCT),1,fin);

        //make sure this is an ats file
        if(atshead->magic != 123){
		sprintf(errmsg, "ATS file %s - The Magic Number is not correct, either this is not an ATS file or it byte endian-ness is wrong\n", filename);
		myiniterror(errmsg);
                return 1;
	}
        if((int)atshead->type > 4 || (int)atshead->type < 1){
		sprintf(errmsg, "ATS file %s - %i type Ats files not supported yet\n", filename, (int)atshead->type);
		myiniterror(errmsg);
		return 1;
        }


	//allocate memory for a new data table
        p->atsdata = (ATS_DATA_LOC **)malloc( (int)atshead->nfrms * sizeof(ATS_DATA_LOC *) );
	
        for(i = 0; i < (int)atshead->nfrms; i++)
        	p->atsdata[i] = (ATS_DATA_LOC *)malloc( (int)atshead->npartials * sizeof(ATS_DATA_LOC) );

	// store the data into a table
        switch ( (int)atshead->type)
        {
		case 1 :
                	for(i = 0; i < (int)atshead->nfrms; i++)
                        {
                        	//eat time data
                                fread(&temp,sizeof(double),1,fin);
                                //get a whole frame of partial data
                                fread(&p->atsdata[i][0], sizeof(ATS_DATA_LOC),(int)atshead->npartials,fin);
			}
                        break;
		case 2 :
                	for(i = 0; i < (int)atshead->nfrms; i++)
                        {
                        	//eat time data
                                fread(&temp,sizeof(double),1,fin);
                                //get partial data
                                for(j = 0; j < (int)atshead->npartials; j++)
                               	{
                               		fread(&p->atsdata[i][j], sizeof(ATS_DATA_LOC),1,fin);
                                        //eat phase info
                                        fread(&temp, sizeof(double),1,fin);
                                }
			}
                        break;
		case 3 :
                	//allocate mem for the noise data
                        p->atsdatanoise = (double **)malloc((int)atshead->nfrms * sizeof(double *));
                        for(i = 0; i < (int)atshead->nfrms; i++)
                        {
                        	//allocate this row
                                p->atsdatanoise[i] = (double *)malloc(sizeof(double) * 25);
                                //eat time data
                                fread(&temp,sizeof(double),1,fin);
                                //get a whole frame of partial data
                                fread(&p->atsdata[i][0], sizeof(ATS_DATA_LOC),(int)atshead->npartials,fin);
                                //get the noise
                                fread(&p->atsdatanoise[i][0], sizeof(double),25,fin);
			}
                        break;
		case 4 :
               		//allocate mem for the noise data
                        p->atsdatanoise = (double **)malloc((int)atshead->nfrms * sizeof(double *));
                        for(i = 0; i < (int)atshead->nfrms; i++)
                        {
                        	//allocate noise for this row
                                p->atsdatanoise[i] = (double *)malloc(sizeof(double) * 25);
                                //eat time data
                                fread(&temp,sizeof(double),1,fin);
                               	//get partial data
                                for(j = 0; j < (int)atshead->npartials; j++)
                                {
                                	fread(&p->atsdata[i][j], sizeof(ATS_DATA_LOC),1,fin);
                                        //eat phase info
                                        fread(&temp, sizeof(double),1,fin);
				}
                                //get the noise
                                fread(&(p->atsdatanoise[i][0]), sizeof(double),25,fin);
			}
                     	break;
	}
                
	//close the input file
        fclose(fin);
        return 0;
}


void mytest(ATSREADNZ *p)
{
	FILE * fout;
	if((fout=fopen("Atstest","w")) != NULL)
	{
		fprintf(fout,"mytest");
		fclose(fout);
	}
		
}



/* the below is to allow this to be a plugin */

GLOBALS *pcglob;
#define S(x)       sizeof(x)

static OENTRY localops[] = {
  { "atsread", S(ATSREAD),  3, "kk", "kSi", atsreadset, atsread, NULL, (SUBR)mytest},
  { "atsreadnz", S(ATSREADNZ),  3, "k", "kSi", atsreadnzset, atsreadnz, NULL}
  //{ "atsadd",    S(ATSADD),	5,     "a", "kkSiiopo", atsaddset,      NULL,   atsadd}
};

long opcode_size(void)
{
    return sizeof(localops);
}

OENTRY *opcode_init(GLOBALS *xx)
{
    pcglob = xx;
    return localops;
}

