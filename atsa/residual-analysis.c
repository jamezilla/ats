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
  int def_size;
  def_size = factor * M;
  while(def_size < min_fft_size) def_size = ppp2(def_size+1);
  return(def_size);
}

void residual_get_bands(double fft_mag, double *true_bands, int *limits, int bands)
{
  int k;
  for(k=0; k<bands; k++) limits[k] = floor(true_bands[k] / fft_mag);
}


double residual_compute_time_domain_energy(ATS_FFT *fft_struct)
{
  //  Parseval's Theorem states:
  //     N-1                   N-1
  //     sum(|x(n)^2|) =  1/N* sum (|X(k)|^2)
  //     n=0                   k=0
  //     then we multiply the time domain energy by 1/2
  //     because we only compute frequency energy between 
  //     0 Hz and Nyquist only (0 -> N/2)
  int n; 
  double sum=0.0;
  for(n=0; n<fft_struct->size; n++) sum += fabs(fft_struct->fdr[n] * fft_struct->fdr[n]);
  return(sum);
}
  
double residual_get_band_energy(int lo, int hi, ATS_FFT *fft_struct, double norm)
{
  // does 1/N * sum(re^2+im^2) within a band around <center>
  // from <lo> lower bin to <hi> upper bin in <fft-struct>
  int k;
  double sum = 0.0;
  if(lo<0) lo = 0;
  if(hi> floor(fft_struct->size * 0.5)) hi = floor(fft_struct->size * 0.5);
  for(k = lo ; k < hi ; k++)
    sum += MAG_SQUARED( fft_struct->fdr[k], fft_struct->fdi[k], norm);
  return( sum/(double)fft_struct->size );
}

void residual_compute_band_energy(ATS_FFT *fft_struct, int *band_limits, int bands, double *band_energy, double norm)
{
  // loop trough bands and evaluate energy
  // we compute energy of one band as:
  //     (N-1)/2
  // 1/N * sum(|X(k)|^2)
  //       k=0
  // N=fft size, K=bins in band
  int b;
  for(b = 0 ; b<bands-1 ; b++)
    band_energy[b] = residual_get_band_energy(band_limits[b], band_limits[b+1], fft_struct, norm);
}

/* residual_analysis
 * =================
 * performs the critical-band analysis of the residual file
 * file: name of the sound file containing the residual 
 * sound: sound to store the residual data 
 */
void residual_analysis(char *file, ATS_SOUND *sound)
{
  int fil, file_sampling_rate, sflen, hop, M, N, frames, *band_limits;
  int smp=0, M_2, st_pt, filptr, i, frame_n, k;
  double norm=1.0, threshold, fft_mag, **band_arr, *band_energy;
  double time_domain_energy=0.0, freq_domain_energy=0.0, sum=0.0;
  double edges[ATSA_CRITICAL_BANDS+1] = ATSA_CRITICAL_BAND_EDGES;
  ATS_FFT fft_struct;
  mus_sample_t **bufs;
  if ((fil = mus_sound_open_input(file))== -1) {
    fprintf(stderr, "%s: %s\n", file, strerror(errno));
    exit(1);
  }
  file_sampling_rate = mus_sound_srate(file);
  sflen = mus_sound_frames(file);
  hop = sound->frame_size;
  M = sound->window_size;
  N = residual_get_N(M, ATSA_RES_MIN_FFT_SIZE, ATSA_RES_PAD_FACTOR);
  bufs = (mus_sample_t **)malloc(2*sizeof(mus_sample_t*));
  bufs[0] = (mus_sample_t *)malloc(sflen * sizeof(mus_sample_t));
  bufs[1] = (mus_sample_t *)malloc(sflen * sizeof(mus_sample_t));
  fft_struct.size = N;
  fft_struct.rate = file_sampling_rate;
  fft_struct.fdr = (double *)malloc(N * sizeof(double));
  fft_struct.fdi = (double *)malloc(N * sizeof(double));
  threshold = AMP_DB(ATSA_NOISE_THRESHOLD);
  frames = sound->frames;
  fft_mag = (double)file_sampling_rate / (double)N;
  band_limits = (int *)malloc(sizeof(int)*(ATSA_CRITICAL_BANDS+1));
  residual_get_bands(fft_mag, edges, band_limits, ATSA_CRITICAL_BANDS+1);
  band_arr = (double **)malloc(ATSA_CRITICAL_BANDS*sizeof(double *));
  for(i =0 ; i<ATSA_CRITICAL_BANDS ; i++){
    band_arr[i] = (double *)malloc(frames*sizeof(double));
  }
  band_energy = (double *)malloc(ATSA_CRITICAL_BANDS*sizeof(double));
  M_2 = floor( ((double)M - 1) * 0.5 );
  st_pt = N - M_2;
  filptr = M_2 * -1;
  /* read sound into memory */
  mus_sound_read(fil, 0, sflen-1, 2, bufs);     

  fprintf(stderr, "Analyzing residual...\n");

  for(frame_n = 0 ; frame_n < frames ; frame_n++){ 
    for(i = 0 ; i < N ; i++){
      fft_struct.fdr[i] = 0.0;
      fft_struct.fdi[i] = 0.0;
    }
    for(k = 0 ; k < M ; k++){
      if (filptr >= 0 && filptr < sflen)
	fft_struct.fdr[(k+st_pt)%N] = MUS_SAMPLE_TO_FLOAT(bufs[0][filptr]);
      filptr++;
    }
    smp = filptr - M_2 - 1;
    time_domain_energy = residual_compute_time_domain_energy(&fft_struct);
    /* take the fft */
    fft(fft_struct.fdr, fft_struct.fdi, fft_struct.size, 1);
    residual_compute_band_energy(&fft_struct, band_limits, ATSA_CRITICAL_BANDS+1, band_energy, norm);
    sum = 0.0;
    for(k = 0;  k < ATSA_CRITICAL_BANDS; k++){
      sum += band_energy[k];
    }
    freq_domain_energy = 2.0 * sum;
    //    e_ratio = (freq_domain_energy > 0.0) ? (time_domain_energy / freq_domain_energy) : 1.0;
    //    fprintf(stderr, "[FDE: %f TDE: %f e_ratio: %f]\n", freq_domain_energy, time_domain_energy, e_ratio);
    for(k = 0; k < ATSA_CRITICAL_BANDS; k++){
      if( band_energy[k] < threshold) {
	band_arr[k][frame_n] = 0.0;
      } else {
	band_arr[k][frame_n] = band_energy[k];
      }
    }
    filptr = filptr - M + hop;
  }
  // save data in sound
  sound->band_energy = band_arr;
  free(fft_struct.fdr);
  free(fft_struct.fdi);
  free(band_energy);
  free(band_limits);
  free(bufs[0]);
  free(bufs[1]);
  free(bufs);
}

/* band_energy_to_res
 * ==================
 * transfers residual engergy from bands to partials
 * sound: sound structure containing data
 * frame: frame number
 */
void band_energy_to_res(ATS_SOUND *sound, int frame)
{
  int j, k, par, first_par, last_par=0;
  double sum;
  double edges[ATSA_CRITICAL_BANDS+1] = ATSA_CRITICAL_BAND_EDGES;  
  par = 0;
  /* find partials by band */
  for(j=0 ; j<ATSA_CRITICAL_BANDS ; j++){
    first_par = par;
    sum = 0.0;
    while( par < sound->partials &&
	  (sound->band_energy[j][frame] > 0.0) && 
	  (sound->frq[par][frame] >= edges[j]) && 
	  (sound->frq[par][frame] < edges[j+1]))
      {
	sum += sound->amp[par][frame];
	last_par = par;
	par++;
      }
    if( sum > 0.0 ){
      /* transfer band energy to partials */
      for(k=first_par ; k<last_par+1; k++){
	if(k >= sound->partials) {
	  break;
	}
	sound->res[k][frame] = sound->amp[k][frame] * sound->band_energy[j][frame] / sum;
	//	sound->band_energy[j][frame] = 0.0;
      }
    }
  }
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
  for(j=0 ; j<ATSA_CRITICAL_BANDS ; j++){
    sum = 0.0;
    while(sound->frq[par][frame] >= edges[j] && 
	  sound->frq[par][frame] < edges[j+1])
      {
	sum += sound->res[par][frame];
	par++;
      }
    sound->band_energy[j][frame] = sum;
  }
}
