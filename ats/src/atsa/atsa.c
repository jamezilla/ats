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
  /* open output file */
  outfile = fopen(ats_outfile, "wb");
  if (outfile == NULL) {
    printf("\n Could not open %s for writing, bye...\n\n", ats_outfile);
    return(-1);
  }
  /* call tracker */
  sound = tracker(anargs, soundfile, resfile);
  /* save sound */
  if(sound != NULL) {
    fprintf(stderr,"saving ATS data...");
    ats_save(sound, outfile, anargs->SMR_thres, anargs->type);
    fprintf(stderr, "done!\n");
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

