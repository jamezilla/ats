/*
ATSH-MASK.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"

/* make_peaks
 * ==========
 * creates peaks from data in a frame
 * and store them in *peaks
 */
void make_peaks(ATS_SOUND *sound, ATS_PEAK *peaks, int frame) 
{
  int i;
  for(i = 0; i < sound->partials ; i++){
    peaks[i].frq = sound->frq[i][frame];
    peaks[i].amp = sound->amp[i][frame];
    peaks[i].smr = (double)0.0;
  }
}

/* atsh_compute_SMR
 * ================
 * computes the SMR data for a set of frames
 * *sound: pointer to an ATS_SOUND structure
 * from_frame: initial frame
 * to_frame: last frame
 */
void atsh_compute_SMR(ATS_SOUND *sound, int from_frame, int to_frame)
{
  ATS_PEAK *peaks;
  int i, j, nValue=0;
  int todo  = to_frame - from_frame;

  peaks = (ATS_PEAK *)malloc(sound->partials * sizeof(ATS_PEAK));
  
  StartProgress("Computing SMR...", FALSE);
  for(i = from_frame; i < to_frame; i++)
    {
      //fprintf(stderr," frm=%d", i);

      make_peaks(sound,peaks,i);
      evaluate_smr(peaks, sound->partials);
      for(j = 0 ; j < sound->partials ; j++)
	{
	  sound->smr[j][i] = peaks[j].smr;
	  //fprintf(stderr,"%8.3f ",sound->smr[j][i] );
	}
    //fprintf(stderr,"\n");
     ++nValue;
      UpdateProgress(nValue,todo);
    }
  EndProgress();
  smr_done=TRUE;
  free(peaks);
}

  


