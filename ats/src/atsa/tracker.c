/* tracker.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
int compute_frames(ANARGS *anargs);


/* ATS_SOUND *tracker (ANARGS *anargs, char *soundfile)
 * partial tracking function 
 * anargs: pointer to analysis parameters
 * soundfile: path to input file 
 * returns an ATS_SOUND with data issued from analysis
 */
ATS_SOUND *tracker (ANARGS *anargs, char *soundfile, char *resfile)
{
  int fd, M_2, first_point, filptr, n_partials = 0;
  int frame_n, k, sflen, *win_samps, peaks_size, tracks_size = 0;
  int i, frame, i_tmp;
  float *window, norm, sfdur, f_tmp;
  /* declare structures and buffers */
  ATS_SOUND *sound = NULL;
  ATS_PEAK *peaks, *tracks = NULL, cpy_peak;
  ATS_FRAME *ana_frames = NULL, *unmatched_peaks = NULL;
  mus_sample_t **bufs;
  ATS_FFT fft;
#ifdef FFTW
  fftw_plan plan;
  FILE *fftw_wisdom_file;
#endif

  /* open input file
     we get srate and total_samps in file in anargs */
  if ((fd = mus_sound_open_input(soundfile))== -1) {
    fprintf(stderr, "%s: %s\n", soundfile, strerror(errno));
    return(NULL);
  }
  /* warn about multi-channel sound files */
  if (mus_sound_chans(soundfile) > 1) {
    fprintf(stderr, "Error: file has %d channels, must be mono!\n",
	    mus_sound_chans(soundfile));
    return(NULL);
  }

  fprintf(stderr, "tracking...\n");

  /* get sample rate and # of frames from file header */
  anargs->srate = mus_sound_srate(soundfile);
  sflen = mus_sound_frames(soundfile);
  sfdur = (float)sflen/anargs->srate;
  /* check analysis parameters */
  /* check start time */
  if( !(anargs->start >= 0.0 && anargs->start < sfdur) ){
    fprintf(stderr, "Warning: start %f out of bounds, corrected to 0.0\n", anargs->start);
    anargs->start = (float)0.0;
  }
  /* check duration */
  if(anargs->duration == ATSA_DUR) {
    anargs->duration = sfdur - anargs->start;
  }
  f_tmp = anargs->duration + anargs->start;
  if( !(anargs->duration > 0.0 && f_tmp <= sfdur) ){
    fprintf(stderr, "Warning: duration %f out of bounds, limited to file duration\n", anargs->duration);
    anargs->duration = sfdur - anargs->start;
  }
  /* print time bounds */
  fprintf(stderr, "start: %f duration: %f file dur: %f\n", anargs->start, anargs->duration , sfdur);
  /* check lowest frequency */
  if( !(anargs->lowest_freq > 0.0 && anargs->lowest_freq < anargs->highest_freq)){
    fprintf(stderr, "Warning: lowest freq. %f out of bounds, forced to default: %f\n", anargs->lowest_freq, ATSA_LFREQ);
    anargs->lowest_freq = ATSA_LFREQ;
  }
  /* check highest frequency */
  if( !(anargs->highest_freq > anargs->lowest_freq && anargs->highest_freq <= anargs->srate * 0.5 )){
    fprintf(stderr, "Warning: highest freq. %f out of bounds, forced to default: %f\n", anargs->highest_freq, ATSA_HFREQ);
    anargs->highest_freq = ATSA_HFREQ;
  }
  /* frequency deviation */
  if( !(anargs->freq_dev > 0.0 && anargs->freq_dev < 1.0) ){
    fprintf(stderr, "Warning: freq. dev. %f out of bounds, should be > 0.0 and <= 1.0,  forced to default: %f\n", anargs->freq_dev, ATSA_FREQDEV);
    anargs->freq_dev = ATSA_FREQDEV;
  }
  /* window cycles */
  if( !(anargs->win_cycles >= 1 && anargs->win_cycles <= 8) ){
    fprintf(stderr, "Warning: windows cycles %d out of bounds, should be between 1 and 8, forced to default: %d\n", anargs->win_cycles, ATSA_WCYCLES);
    anargs->win_cycles = ATSA_WCYCLES;
  }
  /* window type */
  if( !(anargs->win_type >= 0 && anargs->win_type <= 3) ){
    fprintf(stderr, "Warning: window type %d out of bounds, should be between 0 and 3, forced to default: %d\n", anargs->win_type, ATSA_WTYPE);
    anargs->win_type = ATSA_WTYPE;
  }
  /* hop size */
  if( !(anargs->hop_size > 0.0 && anargs->hop_size <= 1.0) ){
    fprintf(stderr, "Warning: hop size %f out of bounds, should be > 0.0 and <= 1.0, forced to default: %f\n", anargs->hop_size, ATSA_HSIZE);
    anargs->hop_size = ATSA_HSIZE;
  }
  /* lowest mag */
  if( !(anargs->lowest_mag <= 0.0) ){
    fprintf(stderr, "Warning: lowest magnitude %f out of bounds, should be >= 0.0 and <= 1.0, forced to default: %f\n", anargs->lowest_mag, ATSA_LMAG);
    anargs->lowest_mag = ATSA_LMAG;
  }
  /* set some values before checking next set of parameters */
  anargs->first_smp = (int)floor(anargs->start * (float)anargs->srate);
  anargs->total_samps = (int)floor(anargs->duration * (float)anargs->srate);
  /* fundamental cycles */
  anargs->cycle_smp = (int)floor((double)anargs->win_cycles * (double)anargs->srate / (double)anargs->lowest_freq);
  /* window size */
  anargs->win_size = (anargs->cycle_smp % 2 == 0) ? anargs->cycle_smp+1 : anargs->cycle_smp;
  /* calculate hop samples */
  anargs->hop_smp = floor( (float)anargs->win_size * anargs->hop_size );
  /* compute total number of frames */
  anargs->frames = compute_frames(anargs);
  /* check that we have enough frames for the analysis */
  if( !(anargs->frames >= ATSA_MFRAMES) ){
    fprintf(stderr, "Error: %d frames are not enough for analysis, nead at least %d\n", anargs->frames , ATSA_MFRAMES);
    return(NULL);
  }
  /* check other user parameters */
  /* track length */
  if( !(anargs->track_len >= 1 && anargs->track_len < anargs->frames) ){
    i_tmp = (ATSA_TRKLEN < anargs->frames) ? ATSA_TRKLEN : anargs->frames-1;
    fprintf(stderr, "Warning: track length %d out of bounds, forced to: %d\n", anargs->track_len , i_tmp);
    anargs->track_len = i_tmp;
  }    
  /* min. segment length */
  if( !(anargs->min_seg_len >= 1 && anargs->min_seg_len < anargs->frames) ){
    i_tmp = (ATSA_MSEGLEN < anargs->frames) ? ATSA_MSEGLEN : anargs->frames-1;
    fprintf(stderr, "Warning: min. segment length %d out of bounds, forced to: %d\n", anargs->min_seg_len, i_tmp);
    anargs->min_seg_len = i_tmp;
  }
  /* min. gap length */
  if( !(anargs->min_gap_len >= 0 && anargs->min_gap_len < anargs->frames) ){
    i_tmp = (ATSA_MGAPLEN < anargs->frames) ? ATSA_MGAPLEN : anargs->frames-1;
    fprintf(stderr, "Warning: min. gap length %d out of bounds, forced to: %d\n", anargs->min_gap_len, i_tmp);
    anargs->min_gap_len = i_tmp;
  }
  /* SMR threshold */
  if( !(anargs->SMR_thres >= 0.0 && anargs->SMR_thres < ATSA_MAX_DB_SPL) ){
    fprintf(stderr, "Warning: SMR threshold %f out of bounds, shoul be >= 0.0 and < %f dB SPL, forced to default: %f\n", anargs->SMR_thres, ATSA_MAX_DB_SPL, ATSA_SMRTHRES);
    anargs->SMR_thres = ATSA_SMRTHRES;
  }
  /* min. seg. SMR */
  if( !(anargs->min_seg_SMR >= anargs->SMR_thres && anargs->min_seg_SMR < ATSA_MAX_DB_SPL) ){
    fprintf(stderr, "Warning: min. seg. SMR  %f out of bounds, shoul be >= %f and < %f dB SPL, forced to default: %f\n", anargs->min_seg_SMR, anargs->SMR_thres, ATSA_MAX_DB_SPL, ATSA_MSEGSMR);
    anargs->min_seg_SMR = ATSA_MSEGSMR;
  }
  /* last peak contibution */
  if( !(anargs->last_peak_cont >= 0.0 && anargs->last_peak_cont <= 1.0) ){
    fprintf(stderr, "Warning: last peak contibution %f out of bounds, should be >= 0.0 and <= 1.0, forced to default: %f\n", anargs->last_peak_cont, ATSA_LPKCONT);
    anargs->last_peak_cont = ATSA_LPKCONT;
  }
  /* SMR cont. */
  if( !(anargs->SMR_cont >= 0.0 && anargs->SMR_cont <= 1.0) ){
    fprintf(stderr, "Warning: SMR contibution %f out of bounds, should be >= 0.0 and <= 1.0, forced to default: %f\n", anargs->SMR_cont, ATSA_SMRCONT);
    anargs->SMR_cont = ATSA_SMRCONT;
  }
  /* continue computing parameters */
  /* fft size */
  anargs->fft_size = ppp2(2*anargs->win_size);

  /* allocate memory for sound, we read the whole sound in memory */
  bufs = (mus_sample_t **)malloc(sizeof(mus_sample_t*));
  bufs[0] = (mus_sample_t *)malloc(sflen * sizeof(mus_sample_t));
  /*  bufs = malloc(sizeof(mus_sample_t*));
      bufs[0] = malloc(sflen * sizeof(mus_sample_t)); */
  /* make our window */
  window = make_window(anargs->win_type, anargs->win_size);
  /* get window norm */
  norm = window_norm(window, anargs->win_size);
  /* fft mag for computing frequencies */
  anargs->fft_mag = (double)anargs->srate / (double)anargs->fft_size;
  /* lowest fft bin for analysis */
  anargs->lowest_bin = floor( anargs->lowest_freq / anargs->fft_mag );
  /* highest fft bin for analisis */
  anargs->highest_bin = floor( anargs->highest_freq / anargs->fft_mag );
  /* allocate an array analysis frames in memory */
  ana_frames = (ATS_FRAME *)malloc(anargs->frames * sizeof(ATS_FRAME));
  /* alocate memory to store mid-point window sample numbers */
  win_samps = (int *)malloc(anargs->frames * sizeof(int));
  /* center point of window */
  M_2 = floor((anargs->win_size - 1) / 2); 
  /* first point in fft buffer to write */
  first_point = anargs->fft_size - M_2;  
  /* half a window from first sample */
  filptr = anargs->first_smp - M_2;   
  /* read sound into memory */
  mus_sound_read(fd, 0, sflen-1, 1, bufs);     

  /* make our fft-struct */
  fft.size = anargs->fft_size;
  fft.rate = anargs->srate;
#ifdef FFTW
  fft.data = fftw_malloc(sizeof(fftw_complex) * fft.size);
  if(fftw_import_system_wisdom()) fprintf(stderr, "system wisdom loaded!\n");
  else fprintf(stderr, "cannot locate system wisdom!\n");
  if((fftw_wisdom_file = fopen("ats-wisdom", "r")) != NULL) {
    fftw_import_wisdom_from_file(fftw_wisdom_file);
    fprintf(stderr, "ats-wisdom loaded!\n");
    fclose(fftw_wisdom_file);
  } else fprintf(stderr, "cannot locate ats-wisdom!\n");
  plan = fftw_plan_dft_1d(fft.size, fft.data, fft.data, FFTW_FORWARD, FFTW_PATIENT);
#else
  fft.fdr = (double *)malloc(anargs->fft_size * sizeof(double));
  fft.fdi = (double *)malloc(anargs->fft_size * sizeof(double));
#endif

  /* main loop */
  for (frame_n=0; frame_n<anargs->frames; frame_n++) {
    /* clear fft arrays */
#ifdef FFTW
    for(k=0; k<fft.size; k++) fft.data[k][0] = fft.data[k][1] = 0.0f;
#else
    for(k=0; k<fft.size; k++) fft.fdr[k] = fft.fdi[k] = 0.0f;
#endif
    /* multiply by window */
    for (k=0; k<anargs->win_size; k++) {
      if ((filptr >= 0) && (filptr < sflen)) 
#ifdef FFTW
        fft.data[(k+first_point)%fft.size][0] = window[k] * MUS_SAMPLE_TO_FLOAT(bufs[0][filptr]);
#else
        fft.fdr[(k+first_point)%anargs->fft_size] = window[k] * MUS_SAMPLE_TO_FLOAT(bufs[0][filptr]);
#endif
      filptr++;
    }
    /* we keep sample numbers of window midpoints in win_samps array */
    win_samps[frame_n] = filptr - M_2 - 1;
    /* move file pointer back */
    filptr = filptr - anargs->win_size + anargs->hop_smp;
    /* take the fft */
#ifdef FFTW
    fftw_execute(plan);
#else
    fft_slow(fft.fdr, fft.fdi, fft.size, 1);
#endif
    /* peak detection */
    peaks_size = 0;
    peaks = peak_detection(&fft, anargs->lowest_bin, anargs->highest_bin, anargs->lowest_mag, norm, &peaks_size); 
    /* peak tracking */
    if (peaks != NULL) {
      /* evaluate peaks SMR (masking curves) */
      evaluate_smr(peaks, peaks_size);
      if (frame_n) {
	/* initialize or update tracks */
	if ((tracks = update_tracks(tracks, &tracks_size, anargs->track_len, frame_n, ana_frames, anargs->last_peak_cont)) != NULL) {
	  /* do peak matching */
          unmatched_peaks = peak_tracking(tracks, &tracks_size, peaks, &peaks_size,  anargs->freq_dev, 2.0 * anargs->SMR_cont, &n_partials);
	  /* kill unmatched peaks from previous frame */
          if(unmatched_peaks[0].peaks != NULL) {
	    for(k=0; k<unmatched_peaks[0].n_peaks; k++) {
	      cpy_peak = unmatched_peaks[0].peaks[k];
	      cpy_peak.amp = cpy_peak.smr = 0.0;
	      peaks = push_peak(&cpy_peak, peaks, &peaks_size);
             }
             free(unmatched_peaks[0].peaks);
           }
           /* give birth to peaks from new frame */
           if(unmatched_peaks[1].peaks != NULL) {
             for(k=0; k<unmatched_peaks[1].n_peaks; k++) {
               tracks = push_peak(&unmatched_peaks[1].peaks[k], tracks, &tracks_size);
               unmatched_peaks[1].peaks[k].amp = unmatched_peaks[1].peaks[k].smr = 0.0;
               ana_frames[frame_n-1].peaks = push_peak(&unmatched_peaks[1].peaks[k], ana_frames[frame_n-1].peaks, &ana_frames[frame_n-1].n_peaks);
             }
             free(unmatched_peaks[1].peaks);
           }
         } else {
           /* give number to all peaks */
           qsort(peaks, peaks_size, sizeof(ATS_PEAK), peak_frq_inc);
           for(k=0; k<peaks_size; k++) peaks[k].track = n_partials++;
         }
      } else {
        /* give number to all peaks */
        qsort(peaks, peaks_size, sizeof(ATS_PEAK), peak_frq_inc);
        for(k=0; k<peaks_size; k++) peaks[k].track = n_partials++;
      }
      /* attach peaks to ana_frames */
      ana_frames[frame_n].peaks = peaks;
      ana_frames[frame_n].n_peaks = n_partials;
      ana_frames[frame_n].time = (double)(win_samps[frame_n] - anargs->first_smp) / (double)anargs->srate;
      /* free memory */
      free(unmatched_peaks);
    } else {
      /* if no peaks found, initialize empty frame */
      ana_frames[frame_n].peaks = NULL;
      ana_frames[frame_n].n_peaks = 0;
      ana_frames[frame_n].time = (double)(win_samps[frame_n] - anargs->first_smp) / (double)anargs->srate;
    }
  }
  /* free up some memory */
  free(window);
  free(tracks);
#ifdef FFTW
  fftw_destroy_plan(plan);
  fftw_free(fft.data);
#else
  free(fft.fdr);
  free(fft.fdi);
#endif
  /* init sound */
  fprintf(stderr, "Initializing ATS data...");
  sound = (ATS_SOUND *)malloc(sizeof(ATS_SOUND));
  init_sound(sound, anargs->srate, (int)(anargs->hop_size * anargs->win_size), 
             anargs->win_size, anargs->frames, anargs->duration, n_partials,
             ((anargs->type == 3 || anargs->type == 4) ? 1 : 0));
  /* store values from frames into the arrays */
  for(k=0; k<n_partials; k++) {
    for(frame=0; frame<sound->frames; frame++) {
      sound->time[k][frame] = ana_frames[frame].time;
      for(i=0; i<ana_frames[frame].n_peaks; i++) 
        if(ana_frames[frame].peaks[i].track == k) {
	  sound->amp[k][frame] = ana_frames[frame].peaks[i].amp;
          sound->frq[k][frame] = ana_frames[frame].peaks[i].frq;
          sound->pha[k][frame] = ana_frames[frame].peaks[i].pha;
          sound->smr[k][frame] = ana_frames[frame].peaks[i].smr;
        }
    }
  }
  fprintf(stderr, "done!\n");
  /* free up ana_frames memory */
  /* first, free all peaks in each slot of ana_frames... */
  for (k=0; k<anargs->frames; k++) free(ana_frames[k].peaks);  
  /* ...then free ana_frames */
  free(ana_frames);                                            
  /* optimize sound */
  optimize_sound(anargs, sound);
  /* compute  residual */
  if( anargs->type == 3 || anargs->type == 4 ) {
    fprintf(stderr, "Computing residual...");
    compute_residual(bufs, sflen, resfile, sound, win_samps, anargs->srate);
    fprintf(stderr, "done!\n");
  }
  /* free the rest of the memory */
  free(win_samps);
  free(bufs[0]);
  free(bufs);
  /* analyze residual */
  if( anargs->type == 3 || anargs->type == 4 ) {
    fprintf(stderr, "Analyzing residual...");
    residual_analysis(ATSA_RES_FILE, sound);
    fprintf(stderr, "done!\n");
  }
#ifdef FFTW
  fftw_wisdom_file = fopen("ats-wisdom", "w");
  fftw_export_wisdom_to_file(fftw_wisdom_file);
  fclose(fftw_wisdom_file);
#endif
  fprintf(stderr, "tracking completed.\n");
  return(sound);
}

/* int compute_frames(ANARGS *anargs)
 * computes number of analysis frames from the user's parameters 
 * returns the number of frames
 * anargs: pointer to analysis parameters
 */
int compute_frames(ANARGS *anargs)
{
  int n_frames = (int)floor((float)anargs->total_samps / (float)anargs->hop_smp);
  while((n_frames++ * anargs->hop_smp - anargs->hop_smp + anargs->first_smp) < (anargs->first_smp + anargs->total_samps));
  return(n_frames);
}
