/* header file for all of the ATScsound functions by Alex Norman
*/

typedef struct atsdataloc
{
        double amp;
        double freq;
}       ATS_DATA_LOC;

typedef struct _atspinfo
{
        float   amp;
        float   freq;
}       atspinfo;               //a sturucture that holds the amp and freq of 1 partial

typedef struct _randiats
{ //the data for the randi UG
  int   size; //size of the frame in samples this should be sr/freq.
  float a1;   //first amplitude value
  float a2;   //next  amplitude value
  int   cnt;  //sample position counter
} RANDIATS;

typedef struct _atsnzaux
{
        double   buf[25];
        float   phaseinc[25];
        float   nfreq[25];
	RANDIATS randinoise[25];
}       atsnzAUX;

typedef struct atsstruct
{
        double  magic;  /* ats magic number */
        double  sampr;  /* sampling rate */
        double  frmsz;  /* frame size in samples */
        double  winsz;  /* window size in samples */
        double  npartials;      /* number of partials */
        double  nfrms;  /* number of frames */
        double  ampmax; /* max amplitude */
        double  freqmax;        // max frequency
        double  dur;    /* duration seconds */
        double  type;   /* Ats Frame type 1-4 */
}       ATSSTRUCT;

typedef struct _atsfiledata
{
	ATS_DATA_LOC ** atsdata; //temp data
	double ** atsdatanoise;  //temp data
	ATSSTRUCT atshead;	//the ATS file header
}	ATSFILEDATA;


/* structures to pass data to the opcodes */

typedef struct _atsread
{       OPDS    h;
        float   *kfreq, *kamp, *ktimpnt, *ifileno, *ipartial; //outputs (2) and inputs
        double  maxFr;
	int	prFlg;	//a flag used to indicate if we've steped out of the time range of the data, so we don't print too many warnings
	double  timefrmInc;
        char * filename;
	
	AUXCH   auxch;
        MEMFIL * atsmemfile;
	
	ATS_DATA_LOC ** datap;	// pointer to the data that we get from the ats file (freq and amp array)
}       ATSREAD;

typedef struct _atsreadnz
{       OPDS    h;
        float   *kenergy, *ktimpnt, *ifileno, *inzbin; //outputs (1) and inputs
        double  maxFr;
	int	prFlg;	//a flag used to indicate if we've steped out of the time range of the data, so we don't print too many warnings
        double  timefrmInc;
        char * filename;
        
        AUXCH   auxch;
	MEMFIL * atsmemfile;

	double ** datap;	// pointer to the data that we get from the ats file energy

}       ATSREADNZ;

typedef struct _atsadd
{       OPDS    h;
        float   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn, *iptls; // audio output and k & i inputs
        float   *iptloffset, *iptlincr, *igatefun; // optional arguments
        
	FUNC    *ftp, *AmpGateFunc;     // pointer to table with wave to synthesize sound
        AUXCH   auxch;
        MEMFIL * atsmemfile;
        
	double  maxFr;
	int	prFlg;	//a flag used to indicate if we've steped out of the time range of the data, so we don't print too many warnings
        double  timefrmInc;
        double	MaxAmp;         // maximum amplitude in anaylsis file
        char * filename;

	ATS_DATA_LOC ** datap;
        double  *oscphase;      // oscillator phase
	ATS_DATA_LOC * buf;

}       ATSADD;

typedef struct _atsaddnz
{       OPDS    h;
        float   *aoutput, *ktimpnt, *ifileno, *ibands; // audio output and k & i inputs
        float   *ibandoffset, *ibandincr; // optional arguments
        MEMFIL  *mfp;   // a pointer into the ATS file
        FUNC    *ftp, *AmpGateFunc;     // pointer to table with wave to synthesize sound
        AUXCH   auxch;
        double  *frPtr;         // pointer to the data (past the header)
        double  maxFr, prFlg;
        int     frmInc;         // amount to increment frame pointer to get to next frame
        double  timefrmInc;
        double  winsize;        // size of windows in analysis file, used to compute RMS amplitude from energy in noise band
        int     firstband;      // location of first wanted band in the frame
        int     bandinc;
        double   * buf;           // stores band information for passing data
        // maximum amplitude in anaylsis file
        float   * phaseinc;       // to create an array of noise
        RANDIATS * randinoise;	// a pointer to the interpolated random noise info
        int	oscphase;       //the phase of all the oscilators
	float   * nfreq;
	ATSSTRUCT atshead;
}       ATSADDNZ;

typedef struct _atsbufread
{
        OPDS    h;
        float   *ktimpnt, *kfmod, *ifilno, *iptls;
        float   *iptloffset, *iptlincr; // optional arguments
        MEMFIL  *mfp;
        double  maxFr, frSiz, prFlg;
        /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
        AUXCH   auxch;
        atspinfo *table;        //store freq and amp info for later use
        int     frmInc;         // amount to increment frame pointer to get to next frame
        int     firstpartial;   // location of first wanted partial in the frame
        int     partialinc;     // amount to increment pointer by to get at the next partial in a frame
        int     mems;           // memory size of aux channel
        double  timefrmInc;
        float   MaxAmp;         // maximum amplitude in anaylsis file
        double  *frPtr;         // pointer to the data (past the header)
	ATSSTRUCT atshead;
} ATSBUFREAD;

typedef struct _atscross
{       OPDS    h;
        float   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn, *kmyamp, *katsbufamp, *iptls; // audio output and k & i inputs
        float   *iptloffset, *iptlincr, *igatefun; // optional arguments
        ATSBUFREAD      *atsbufread;
        MEMFIL  *mfp;   // a pointer into the ATS file
        FUNC    *ftp, *AmpGateFunc;     // pointer to table with wave to synthesize sound
        AUXCH   auxch;
        double  *frPtr;         // pointer to the data (past the header)
        double  maxFr, prFlg;
        int     frmInc;         // amount to increment frame pointer to get to next frame
        double  timefrmInc;
        int     firstpartial;   // location of first wanted partial in the frame
        int     partialinc;     // amount to increment pointer by to get at the next partial in a frame
        float   *buf;           // stores partial information for passing data
        float   *oscphase;              // pointer to the phase information
        float   MaxAmp;         // maximum amplitude in anaylsis file
        int     mems;           // memory size of aux channel
	ATSSTRUCT atshead;

}       ATSCROSS;       //modified from atsadd

typedef struct _atsbufreadnz
{
        OPDS    h;
        float   *ktimpnt, *ifileno, *ibands; // required inputs
        float   *ibandoffset, *ibandincr; // optional arguments
        MEMFIL  *mfp;
        double  maxFr, frSiz, prFlg;
        /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
        AUXCH   auxch;
        float   *table; //store noise for later use
        int     frmInc;         // amount to increment frame pointer to get to next frame
        int     firstband;   // location of first wanted partial in the frame
        int     bandinc;     // amount to increment pointer by to get at the next partial in a frame
        int     mems;           // memory size of aux channel
        double  timefrmInc;
        float   MaxAmp;         // maximum amplitude in anaylsis file
        double  *frPtr;         // pointer to the data (past the header)
	ATSSTRUCT atshead;
} ATSBUFREADNZ;

typedef struct _atscrossnz
{       OPDS    h;
        float   *aoutput, *ktimpnt, *ifileno, *ifn, *kmyamp, *katsbufamp, *ibands; // audio output and k & i inputs
        float   *ibandoffset, *ibandincr; // optional arguments
        ATSBUFREADNZ *atsbufreadnz;
        MEMFIL  *mfp;   // a pointer into the ATS file
        FUNC    *ftp;     // pointer to table with wave to synthesize sound
        AUXCH   auxch;
        double  *frPtr;         // pointer to the data (past the header)
        double  maxFr, prFlg;
        int     frmInc;         // amount to increment frame pointer to get to next frame
        double  timefrmInc;
        double  winsize;        // size of windows in analysis file, used to compute RMS amplitude from energy in noise band
        int     firstband;      // location of first wanted band in the frame
        int     bandinc;
        float   *buf;           // stores band information for passing data
        float   MaxAmp;
        // maximum amplitude in anaylsis file
        float   *cosfreq;       // to create an array of noise
        int     *rand;
        long    *nphs;
        float   *num1, *num2, *dfdmax;
        float   *nfreq;
        float   *oscphase;      // for creating noise
	ATSSTRUCT atshead;
}       ATSCROSSNZ;                     
