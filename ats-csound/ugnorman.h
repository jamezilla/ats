/* header file for all of the ATScsound functions by Alex Norman
*/
//#include "ftgen.h"

typedef struct { //the data for the randi UG
  int   size; //size of the frame in samples this should be sr/freq.
  float a1;   //first amplitude value
  float a2;   //next  amplitude value
  int   cnt;  //sample position counter
} RANDIATS;


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

typedef struct
{       OPDS    h;
        float   *kfreq, *kamp, *ktimpnt, *ifileno, *ipartial; //outputs (2) and inputs
        MEMFIL  *mfp;
        double  *frPtr; // pointer to the data (past the header)
        double  maxFr, prFlg;
        int     frmInc;         // amount to increment frame pointer to get to next frame
        double  timefrmInc;
        int     partialloc;     // location of the partial in the frame
}       ATSREAD;

typedef struct
{       OPDS    h;
        float   *kenergy, *ktimpnt, *ifileno, *inzbin; //outputs (1) and inputs
        MEMFIL  *mfp;
        double  *frPtr; // pointer to the data (past the header)
        double  maxFr, prFlg;
        int     frmInc;         // amount to increment frame pointer to get to next frame
        double  timefrmInc;
        int     nzbinloc;       // location of the noise bin in the frame
}       ATSREADNZ;

typedef struct
{       OPDS    h;
        float   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn, *iptls; // audio output and k & i inputs
        float   *iptloffset, *iptlincr, *igatefun; // optional arguments
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
        float   * oscphase;      // for creating noise
        float   MaxAmp;         // maximum amplitude in anaylsis file
        int     mems;           // memory size of aux channel

}       ATSADD;

typedef struct
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
}       ATSADDNZ;

typedef struct
{
        double   buf[25];
        float   phaseinc[25];
        float   nfreq[25];
	RANDIATS randinoise[25];
}       atsnzAUX;

typedef struct
{
        float   amp;
        float   freq;
}       atspinfo;               //a sturucture that holds the amp and freq of 1 partial


typedef struct {
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
} ATSBUFREAD;

typedef struct
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

}       ATSCROSS;       //modified from atsadd

typedef struct {
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
} ATSBUFREADNZ;

typedef struct
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
}       ATSCROSSNZ;                     
