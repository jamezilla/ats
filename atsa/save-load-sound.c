/* save-load-sound.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* ats_save
 * ========
 * saves an ATS_SOUND to disk.
 * sound: pointer to ATS_SOUND structure
 * outfile: pointer to output ats file
 * SMR_thres: partials with and avreage SMR 
 * below this value are considered masked
 * and not written out to the ats file
 * type: file type
 * NOTE: sound MUST be optimized using optimize_sound
 * before calling this function
 */
void ats_save(ATS_SOUND *sound, FILE *outfile, float SMR_thres, int type)
{
  int frm, i, par, dead=0;
  double daux;
  ATS_HEADER header;

  if( sound->optimized == NIL ){
    fprintf(stderr, "Error: sound not optimized!\n");
    exit(1);
  }
  /* count how many partials are dead
   * unfortunately we have to do this first to 
   * write the number of partials in the header
   */
  for(i = 0  ; i < sound->partials ; i++){
    /* see if partial is dead */
    if( !(sound->av[i].frq > 0.0) || !(sound->av[i].smr >= SMR_thres) ){
      dead++;
    }
  }
  /* sort partials by increasing frequency */
  qsort(sound->av, sound->partials, sizeof(ATS_PEAK), peak_frq_inc);
  /* fill header up */
  header.mag = (double)123.0;
  header.sr = (double)sound->srate;
  header.fs = (double)sound->frame_size;
  header.ws = (double)sound->window_size;
  header.par = (double)(sound->partials - dead);
  header.fra = (double)sound->frames;
  header.ma = sound->ampmax;
  header.mf = sound->frqmax;
  header.dur = sound->dur;
  header.typ = (double)type;
  /* write header */
  fseek(outfile, 0, SEEK_SET);
  fwrite(&header, 1, sizeof(ATS_HEADER), outfile);
  /* write frame data */
  for(frm=0; frm<sound->frames; frm++) {
    daux = sound->time[0][frm];
    fwrite(&daux, 1, sizeof(double), outfile);
    for(i=0; i<sound->partials; i++) {
      /* we ouput data in increasing frequency order
       * and we check for dead partials
       */
      if((sound->av[i].frq > 0.0) && (sound->av[i].smr >= SMR_thres)){
	/* get partial number from sound */
	par = sound->av[i].track;
	/* output data to file */
	daux = sound->amp[par][frm];
	fwrite(&daux, 1, sizeof(double), outfile);
	daux = sound->frq[par][frm];
	fwrite(&daux, 1, sizeof(double), outfile);
	if( type == 2 || type == 4){
	  daux = sound->pha[par][frm];
	  fwrite(&daux, 1, sizeof(double), outfile);
	}
      }
    }
    /* write noise data */
    if( type == 3 || type == 4){
      for(i=0 ; i<ATSA_CRITICAL_BANDS ; i++){
	daux = sound->band_energy[i][frm];
	fwrite(&daux, 1, sizeof(double), outfile);
      }
    }
  }
}
