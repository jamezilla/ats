/* utilities.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
int find_next_val_arr(double* arr, int beg, int size);
int find_next_zero_arr(double* arr, int beg, int size);
int find_prev_val_arr(double* arr, int beg);
void fill_sound_gaps(ATS_SOUND *sound, int min_gap_len);
void trim_partials(ATS_SOUND *sound, int min_seg_len, float min_seg_smr);
void set_av(ATS_SOUND *sound);

/* various conversion functions
 * to deal with dB and dB SPL
 * they take and return double floats
 */
double amp2db(double amp)
{
  return (20* log10(amp));
}

double db2amp(double db)
{
  return (pow(10, db/20));
}

double amp2db_spl(double amp)
{
  return(amp2db(amp) + ATSA_MAX_DB_SPL);
}

double db2amp_spl(double db_spl)
{
  return(db2amp(db_spl - ATSA_MAX_DB_SPL));
}

/* ppp2
 * ====
 * returns the closest power of two
 * greater than num
 */
unsigned int ppp2(int num)
{
  unsigned int tmp = 2;

  while(tmp<num) tmp = tmp << 1;
  return(tmp);    
}

/* optimize_sound
 * ==============
 * optimizes an ATS_SOUND in memory before saving
 * anargs: pointer to analysis parameters
 * sound: pointer to ATS_SOUND structure
 */
void optimize_sound(ANARGS *anargs, ATS_SOUND *sound)
{
  double ampmax = 0.0, frqmax = 0.0;
  int frame, partial;

  for(frame=0; frame<sound->frames; frame++)
    for(partial=0; partial<sound->partials; partial++) {
      if(ampmax < sound->amp[partial][frame]) ampmax = sound->amp[partial][frame];
      if(frqmax < sound->frq[partial][frame]) frqmax = sound->frq[partial][frame];
    }
  sound->ampmax = ampmax;
  sound->frqmax = frqmax;

  fill_sound_gaps(sound, anargs->min_gap_len);
  trim_partials(sound, anargs->min_seg_len, anargs->min_seg_SMR);
  set_av(sound);
  /* finally set slot to 1 */
  sound->optimized = 1;
}

/* fill_sound_gaps
 * ===============
 * fills gaps in ATS_SOUND partials by interpolation
 * sound: pointer to ATS_SOUND
 * min_gap_len: minimum gap length, gaps shorter or equal to this
 * value will be filled in by interpolation
 */
void fill_sound_gaps(ATS_SOUND *sound, int min_gap_len)
{
  int i, j, k, next_val, next_zero, prev_val, gap_size;
  double f_inc, a_inc, s_inc, mag; 
  mag = TWOPI / (double)sound->srate;
  fprintf(stderr, "Filling sound gaps...\n");
  for(i = 0 ; i < sound->partials ; i++){
    /* first we fix the freq gap before attack */
    next_val = find_next_val_arr(sound->frq[i], 0, sound->frames);
    if( next_val > 0 ){
      for( j = 0 ; j < next_val ; j++ ){
	sound->frq[i][j] =  sound->frq[i][next_val];
      }
    }
    /* fix the freq gap at end */
    prev_val = find_prev_val_arr(sound->frq[i], sound->frames-1);
    if( prev_val != NIL && prev_val < sound->frames-1 ){
      for( j = prev_val ; j < sound->frames ; j++ ){
	sound->frq[i][j] =  sound->frq[i][prev_val];
      }
    }
    /* now we fix inner gaps of frq, pha, and amp */
    k = find_next_val_arr(sound->amp[i], 0, sound->frames);
    while( k < sound->frames && k != NIL){
      /* find next gap: we consider gaps in amplitude, we fix frequency and phase in parallel */
      next_zero = find_next_zero_arr(sound->amp[i], k, sound->frames);
      if( next_zero != NIL){
	prev_val = next_zero - 1;
	next_val = find_next_val_arr(sound->amp[i], next_zero, sound->frames);
	/* check if we didn't get to end of array */
	if( next_val == NIL) break;
	gap_size = next_val - prev_val;
	//	fprintf(stderr, "par: %d prev_val: %d next_val: %d gap_size %d\n", 
	//		i, prev_val, next_val, gap_size);
	/* check validity of found gap */
	if( gap_size <= min_gap_len){
	  //	  fprintf(stderr, "Filling gap of par: %d\n", i);
	  f_inc = (sound->frq[i][next_val] - sound->frq[i][prev_val]) / gap_size;
	  a_inc = (sound->amp[i][next_val] - sound->amp[i][prev_val]) / gap_size;
	  s_inc = (sound->smr[i][next_val] - sound->smr[i][prev_val]) / gap_size;
	  for( j = next_zero ; j < next_val ; j++){
	    sound->frq[i][j] = sound->frq[i][j-1] + f_inc;
	    sound->amp[i][j] = sound->amp[i][j-1] + a_inc;
	    sound->smr[i][j] = sound->smr[i][j-1] + s_inc;
	    sound->pha[i][j] = sound->pha[i][j-1] - (sound->frq[i][j] * sound->frame_size * mag);
	    /* wrap phase */
	    while(sound->pha[i][j] > PI){ sound->pha[i][j] -= TWOPI; }
	    while(sound->pha[i][j] < (PI * -1.0)){ sound->pha[i][j] += TWOPI; }
	  }
	  /* gap fixed, find next gap */
	  k = next_val;
	} else {
	  /* gap not valid, move to next one */
	  //	  fprintf(stderr, "jumping to next_val: %d\n", next_val);
	  k = next_val;
	}
      } else {
	break;
      }
    }
  }
}

/* trim_partials
 * =============
 * trim short segments from ATS_SOUND partials
 * sound: pointer to ATS_SOUND
 * min_seg_len: minimum segment length, segments shorter or equal
 * to this value will be candidates for trimming
 * min_seg_smr: minimum segment average SMR, segment candidates
 * should have an average SMR below this value to be trimmed
 */
void trim_partials(ATS_SOUND *sound, int min_seg_len, float min_seg_smr)
{
  int i, j, k, seg_beg, seg_end, seg_size, count=0;
  double val=0.0, smr_av=0.0;
  fprintf(stderr, "Trimming short partials...\n");
  for(i = 0 ; i < sound->partials ; i++){
    k = 0;
    while( k < sound->frames ){
      /* find next segment */
      seg_beg = find_next_val_arr(sound->amp[i], k, sound->frames);
      if( seg_beg != NIL){
	seg_end = find_next_zero_arr(sound->amp[i], seg_beg, sound->frames);
	/* check if we didn't get to end of array */
	if( seg_end == NIL) seg_end = sound->frames;
	seg_size = seg_end - seg_beg;
	//	fprintf(stderr, "par: %d seg_beg: %d seg_end: %d seg_size %d\n", 
	//		i, seg_beg, seg_end, seg_size);
	if( seg_size <= min_seg_len ){
	  for( j = seg_beg ; j < seg_end ; j++){
	    if( sound->smr[i][j] > 0.0 ){
	      val += sound->smr[i][j];
	      count++;
	    }
	  }
	  if(count > 0) smr_av = val/(double)count;
	  if( smr_av < min_seg_smr ){
	    /* trim segment, only amplitude and SMR data */
	    //	    fprintf(stderr, "Trimming par: %d\n", i);
	    for( j = seg_beg ; j < seg_end ; j++){
	      sound->amp[i][j] = (double)0.0;
	      sound->smr[i][j] = (double)0.0;
	    }
	  } 
	  k = seg_end;
	} else {
	  /* segment not valid, move to the next one */
	  //	  fprintf(stderr, "jumping to seg_end: %d\n", seg_end);
	  k = seg_end;
	}
      } else {
	break;
      }
    }
  }
}

/* auxiliary functions to fill_sound_gaps and trim_partials */
int find_next_val_arr(double* arr, int beg, int size)
{
  int j, next_val=NIL;
  for( j = beg ; j < size ; j++){
    if( arr[j] > 0.0 ){
      next_val = j;
      break;
    }
  }
  return(next_val);
}

int find_next_zero_arr(double* arr, int beg, int size)
{
  int j, next_zero=NIL;
  for( j = beg ; j < size ; j++){
    if( arr[j] == 0.0 ){
      next_zero = j;
      break;
    }
  }
  return(next_zero);
}

int find_prev_val_arr(double* arr, int beg)
{
  int j, prev_val=NIL;
  for( j = beg ; j >= 0 ; j--){
    if( arr[j] != 0.0 ){
      prev_val = j;
      break;
    }
  }
  return(prev_val);
}

/* set_av
 * ======
 * sets the av structure slot of an ATS_SOUND,
 * it computes the average freq. and SMR for each partial
 * sound: pointer to ATS_SOUND structure
 */
void set_av(ATS_SOUND *sound)
{
  int i, j, count;
  double val;
  fprintf(stderr,"Computing averages..\n");
  for( i = 0 ; i < sound->partials ; i++){
    /* smr */
    val=0.0;
    count = 0;
    for(j = 0 ; j < sound->frames ; j++){
      if(sound->smr[i][j] > 0.0){
	val += sound->smr[i][j];
	count++;
      }
    }
    if(count > 0){
      sound->av[i].smr =  val/(double)count;
    } else {
      sound->av[i].smr = (double)0.0;
    }
    //    fprintf(stderr,"par: %d smr_av: %f\n", i, (float)sound->av[i].smr);
    /* frq */
    val=0.0;
    count = 0;
    for(j = 0 ; j < sound->frames ; j++){
      if(sound->frq[i][j] > 0.0){
	val += sound->frq[i][j];
	count++;
      }
    }
    if(count > 0){
      sound->av[i].frq =  val/(double)count;
    } else {
      sound->av[i].frq = (double)0.0;
    }
    /* set track# */
    sound->av[i].track = i;
  }
}

/* init_sound
 * ==========
 * initializes a new sound allocating memory
 */
void init_sound(ATS_SOUND *sound, int sampling_rate, int frame_size, int window_size, int frames, double duration, int partials)
{
  int i, j, k;
  sound->srate = sampling_rate;
  sound->frame_size = frame_size;
  sound->window_size = window_size;
  sound->frames = frames;
  sound->dur = duration;
  sound->partials = partials;
  sound->av = (ATS_PEAK *)malloc(partials * sizeof(ATS_PEAK));
  sound->optimized = NIL;
  sound->time = (void *)malloc(partials * sizeof(void *));
  sound->frq = (void *)malloc(partials * sizeof(void *));
  sound->amp = (void *)malloc(partials * sizeof(void *));
  sound->pha = (void *)malloc(partials * sizeof(void *));
  sound->smr = (void *)malloc(partials * sizeof(void *));
  sound->res = (void *)malloc(partials * sizeof(void *));
  /* allocate memory for time, amp, frq, smr, and res data */
  for(k=0; k<partials; k++) {
    sound->time[k] = (double *)malloc(frames * sizeof(double));
    sound->amp[k] = (double *)malloc(frames * sizeof(double));
    sound->frq[k] = (double *)malloc(frames * sizeof(double));
    sound->pha[k] = (double *)malloc(frames * sizeof(double));
    sound->smr[k] = (double *)malloc(frames * sizeof(double));
    sound->res[k] = (double *)malloc(frames * sizeof(double));
  }
  /* init all array values with 0.0 */
  for( i = 0 ; i < partials ; i++){
    for( j = 0 ; j < frames ; j++){
      sound->amp[i][j] = (double)0.0;
      sound->frq[i][j] = (double)0.0;
      sound->pha[i][j] = (double)0.0;
      sound->smr[i][j] = (double)0.0;
      sound->res[i][j] = (double)0.0;
    }
  }    
  sound->band_energy = (void *)malloc(ATSA_CRITICAL_BANDS * sizeof(void *));
  for(i=0; i<ATSA_CRITICAL_BANDS; i++)
    sound->band_energy[i] = (double *)malloc(frames * sizeof(double));
}  


/* free_sound
 * ==========
 * frees sound's memory
 */
void free_sound(ATS_SOUND *sound)
{
  int k;
  free(sound->av);
  /* data */
  for(k=0; k < sound->partials; k++) {
    free(sound->time[k]);
    free(sound->amp[k]);
    free(sound->frq[k]);
    free(sound->pha[k]);
    free(sound->smr[k]);
    free(sound->res[k]);
  }
  /* pointers to data */
  free(sound->time);
  free(sound->frq);
  free(sound->amp);
  free(sound->pha);
  free(sound->smr);
  free(sound->res);
  /* check if we have residual data 
   * and free its memory up
   */
  if( sound->band_energy != NULL ){
    for(k=0 ; k<ATSA_CRITICAL_BANDS ; k++){
      free(sound->band_energy[k]);
    }
    free(sound->band_energy);
  }
}
