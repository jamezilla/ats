/* atsa.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* main_anal
 * =========
 * main analysis function
 * soundfile: path to input file 
 * out_file: path to output ats file 
 * anargs: pointer to analysis parameters
 * returns error status
 */
int main_anal(char *soundfile, char *ats_outfile, ANARGS *anargs, char *resfile)
{ 
  /* create pointers and structures */
  ATS_SOUND *sound = NULL;
  FILE *outfile;
  int frame_n;
  /* open output file */
  outfile = fopen(ats_outfile, "wb");
  if (outfile == NULL) {
    printf("\n Could not open %s for writing, bye...\n\n", ats_outfile);
    return(-1);
  }
  /* call tracker */
  tracker_init (anargs, soundfile);
  for (frame_n=0; frame_n<anargs->frames; frame_n++) {
    tracker_fft(anargs, frame_n);
  }
  sound = tracker_sound(anargs);
  tracker_residual(anargs, resfile, sound);
  tracker_free(anargs);
  /* save sound */
  if(sound != NULL) {
    fprintf(stderr,"saving ATS data...\n");
    ats_save(sound, outfile, anargs->SMR_thres, anargs->type);
  }
  else{
    /* file I/O error */
    return(-2); 
  }
  /* close output file */
  fclose(outfile);
  /* free ATS_SOUND memory */
  free_sound(sound);
  return(0);
}

