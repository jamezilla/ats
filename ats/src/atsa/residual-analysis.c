/* residual-analysis.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

#define ATSA_RES_MIN_FFT_SIZE 4096
#define ATSA_RES_PAD_FACTOR 2
#define MAG_SQUARED(re, im, norm) (norm * (re*re+im*im))

/* private function prototypes */
int residual_get_N(int M, int min_fft_size, int factor);
void residual_get_bands(double fft_mag, double *true_bands, int *limits, int bands);
double residual_compute_time_domain_energy(ATS_FFT *fft_struct);
double residual_get_band_energy(int lo, int hi, ATS_FFT *fft_struct, double norm);
void residual_compute_band_energy(ATS_FFT *fft_struct, int *band_limits, int bands, double *band_energy, double norm);

int residual_get_N(int M, int min_fft_size, int factor)
{
  int def_size = factor * M;
  while(def_size < min_fft_size) def_size = ppp2(def_size+1);
  return(def_size);
}

void residual_get_bands(double fft_mag, double *true_bands, int *limits, int bands)
{
  int k;
  for(k=0; k<bands; k++) limits[k] = floor(true_bands[k] / fft_mag);
}


double residual_compute_time_domain_energy(ATS_FFT *fft)
{
  /* Parseval's Theorem states:

     N-1                   N-1
     sum(|x(n)^2|) =  1/N* sum (|X(k)|^2)
     n=0                   k=0

     then we multiply the time domain energy by 1/2
     because we only compute frequency energy between 
     0 Hz and Nyquist only (0 -> N/2)
  */
  int n; 
  double sum=0.0;
#ifdef FFTW
  for(n=0; n<fft->size; n++) sum += fabs(fft->data[n][0] * fft->data[n][0]);
#else
  for(n=0; n<fft->size; n++) sum += fabs(fft->fdr[n] * fft->fdr[n]);
#endif
  return(sum);
}

double residual_get_band_energy(int lo, int hi, ATS_FFT *fft, double norm)
{
  /* does 1/N * sum(re^2+im^2) within a band around <center>
     from <lo> lower bin to <hi> upper bin in <fft-struct> */
  int k;
  double sum = 0.0;
  if(lo<0) lo = 0;
  if(hi> floor(fft->size * 0.5)) hi = floor(fft->size * 0.5);
  for(k = lo ; k < hi ; k++)
#ifdef FFTW
    sum += MAG_SQUARED( fft->data[k][0], fft->data[k][1], norm);
#else
    sum += MAG_SQUARED( fft->fdr[k], fft->fdi[k], norm);
#endif
  return( sum/(double)fft->size );
}

void residual_compute_band_energy(ATS_FFT *fft, int *band_limits, int bands, double *band_energy, double norm)
{
  /* loop trough bands and evaluate energy
     we compute energy of one band as:
     (N-1)/2
     1/N * sum(|X(k)|^2)
     k=0
     N=fft size, K=bins in band */
  int b;
  for(b=0; b<bands-1; b++)
    band_energy[b] = residual_get_band_energy(band_limits[b], band_limits[b+1], fft, norm);
}

/* residual_analysis
 * =================
 * performs the critical-band analysis of the residual file
 * file: name of the sound file containing the residual 
 * sound: sound to store the residual data 
 */
void residual_analysis(ANARGS *anargs, ATS_SOUND *sound)
{
  int fil, file_sampling_rate, sflen, hop, M, N, frames, *band_limits;
  int smp=0, M_2, st_pt, filptr, i, frame_n, k;
  double norm=1.0, threshold, fft_mag, **band_arr=NULL, *band_energy;
  double time_domain_energy=0.0, freq_domain_energy=0.0, sum=0.0;
  double edges[ATSA_CRITICAL_BANDS+1] = ATSA_CRITICAL_BAND_EDGES;
  ATS_FFT fft;
//  mus_sample_t **bufs;
#ifdef FFTW
  fftw_plan plan;
  //  FILE *fftw_wisdom_file;
#endif
//  if ((fil = mus_sound_open_input(file))== -1) {
//    fprintf(stderr, "\n%s: %s\n", file, strerror(errno));
//    exit(1);
//  }
//  file_sampling_rate = mus_sound_srate(file);
//  sflen = mus_sound_frames(file);
  hop = sound->frame_size;
  M = sound->window_size;
  N = residual_get_N(M, ATSA_RES_MIN_FFT_SIZE, ATSA_RES_PAD_FACTOR);
//  bufs = (mus_sample_t **)malloc(2*sizeof(mus_sample_t*));
//  bufs[0] = (mus_sample_t *)malloc(sflen * sizeof(mus_sample_t));
//  bufs[1] = (mus_sample_t *)malloc(sflen * sizeof(mus_sample_t));
  fft.size = N;
  fft.rate = file_sampling_rate;
#ifdef FFTW
  fft.data = fftw_malloc(sizeof(fftw_complex) * fft.size);
  //fftw_wisdom_file = fopen("fftw-wisdom", "r");
  //  fftw_import_wisdom_from_file(fftw_wisdom_file);
  plan = fftw_plan_dft_1d(fft.size, fft.data, fft.data, FFTW_FORWARD, FFTW_PATIENT);
  //  fclose(fftw_wisdom_file);
#else
  fft.fdr = (double *)malloc(N * sizeof(double));
  fft.fdi = (double *)malloc(N * sizeof(double));
#endif
  threshold = AMP_DB(ATSA_NOISE_THRESHOLD);
  frames = sound->frames;
  fft_mag = (double)file_sampling_rate / (double)N;
  band_limits = (int *)malloc(sizeof(int)*(ATSA_CRITICAL_BANDS+1));
  residual_get_bands(fft_mag, edges, band_limits, ATSA_CRITICAL_BANDS+1);
  band_arr = sound->band_energy;
  band_energy = (double *)malloc(ATSA_CRITICAL_BANDS*sizeof(double));

  M_2 = floor( ((double)M - 1) * 0.5 );
  st_pt = N - M_2;
  filptr = M_2 * -1;
  /* read sound into memory */
//  mus_sound_read(fil, 0, sflen-1, 2, bufs);     

  for(frame_n = 0 ; frame_n < frames ; frame_n++){ 
    for(i=0; i<N; i++) {
#ifdef FFTW
      fft.data[i][0] = fft.data[i][1] = 0.0;
#else
      fft.fdr[i] = fft.fdi[i] = 0.0;
#endif
    }
    for(k=0; k<M; k++) {
      if (filptr >= 0 && filptr < sflen)
#ifdef FFTW
//        fft.data[(k+st_pt)%N][0] = MUS_SAMPLE_TO_FLOAT(bufs[0][filptr]);
        fft.data[(k+st_pt)%N][0] = anargs->residual[filptr];
#else
//	fft.fdr[(k+st_pt)%N] = MUS_SAMPLE_TO_FLOAT(bufs[0][filptr]);
	fft.fdr[(k+st_pt)%N] = anargs->residual[filptr];
#endif
      filptr++;
    }
    smp = filptr - M_2 - 1;
    time_domain_energy = residual_compute_time_domain_energy(&fft);
    /* take the fft */
#ifdef FFTW
    fftw_execute(plan);
#else
    fft_slow(fft.fdr, fft.fdi, fft.size, 1);
#endif
    residual_compute_band_energy(&fft, band_limits, ATSA_CRITICAL_BANDS+1, band_energy, norm);
    sum = 0.0;
    for(k = 0;  k < ATSA_CRITICAL_BANDS; k++){
      sum += band_energy[k];
    }
    freq_domain_energy = 2.0 * sum;
    for(k = 0; k < ATSA_CRITICAL_BANDS; k++){
      if( band_energy[k] < threshold) {
	band_arr[k][frame_n] = 0.0;
      } else {
	band_arr[k][frame_n] = band_energy[k];
      }
    }
    filptr = filptr - M + hop;
  }
  /* save data in sound */
  sound->band_energy = band_arr;
#ifdef FFTW
  fftw_destroy_plan(plan);
  fftw_free(fft.data);
#else
  free(fft.fdr);
  free(fft.fdi);
#endif
  free(band_energy);
  free(band_limits);
//  free(bufs[0]);
//  free(bufs[1]);
//  free(bufs);
}

/* band_energy_to_res
 * ==================
 * transfers residual engergy from bands to partials
 * sound: sound structure containing data
 * frame: frame number
 */
void band_energy_to_res(ATS_SOUND *sound, int frame)
{
  int i, j;
  double edges[] = ATSA_CRITICAL_BAND_EDGES;  
  double bandsum[ATSA_CRITICAL_BANDS];
  double partialfreq, partialamp;
  double * partialbandamp;	/* amplitude of the band that the partial is in */
  int * bandnum;	/* the band number that the partial is in */
  
  partialbandamp = malloc(sizeof(double) * sound->partials);
  bandnum = malloc(sizeof(int) * sound->partials);
  if(partialbandamp == NULL || bandnum == NULL) {
    fprintf(stderr, "\n%s: malloc() returned NULL\n", __PRETTY_FUNCTION__);
    return;
  }
  /* initialize the sum per band */
  for(i=0; i<ATSA_CRITICAL_BANDS; i++) bandsum[i] = 0;
  
  /* find find which band each partial is in */
  for(i = 0; i < sound->partials; i++) {
    partialfreq = sound->frq[i][frame];
    partialamp = sound->amp[i][frame];
    for(j = 0; j < 25; j++) {
      if( (partialfreq < edges[j+1]) && (partialfreq >= edges[j]) ) {
        bandsum[j] += partialamp;
        bandnum[i] = j;
        partialbandamp[i] = sound->band_energy[j][frame];
        break;
      }
    }
  }
  /* compute energy per partial */
  for(i=0; i<sound->partials; i++) {
    if(bandsum[bandnum[i]] > 0.0)
      sound->res[i][frame] = sound->amp[i][frame] * partialbandamp[i] / bandsum[bandnum[i]];
    else
      sound->res[i][frame] = 0.0;
  }
  free(partialbandamp);
  free(bandnum);
}		

/* res_to_band_energy
 * ==================
 * transfers residual engergy from partials to bands
 * sound: sound structure containing data
 * frame: frame number
 */
void res_to_band_energy(ATS_SOUND *sound, int frame)
{
  int j, par;
  double sum;
  double edges[ATSA_CRITICAL_BANDS+1] = ATSA_CRITICAL_BAND_EDGES;  
  par = 0;
  for(j=0 ; j<ATSA_CRITICAL_BANDS ; j++) {
    sum = 0.0;
    while(sound->frq[par][frame] >= edges[j] && sound->frq[par][frame] < edges[j+1]) {
      sum += sound->res[par][frame];
      par++;
    }
    sound->band_energy[j][frame] = sum;
  }
}
