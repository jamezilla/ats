/* other-utils.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
void fft_bit_reversal(double* rl, double* im, int n);

/* make_window
 * ===========
 * makes an analysis window, returns a pointer to it.
 * win_type: window type, available types are:
 * BLACKMAN, BLACKMAN_H, HAMMING and VONHANN
 * win_size: window size
 */
float *make_window(int win_type, int win_size)
{
  float *buffer;
  int i;
  float arg   = TWOPI / (float)(win_size - 1);

  buffer   = (float *)malloc(win_size*sizeof(float));

  for(i=0; i < win_size; i++) {
    switch (win_type) {
    case BLACKMAN:   //Blackman (3 term)
      buffer[i]= 0.42 - 0.5*cos(arg*i) + 0.08*cos(arg*i*2.);
      break;
    case BLACKMAN_H: //Blackman-Harris (4 term)
      buffer[i]= 0.35875 - 0.48829*cos(arg*i) + 0.14128*cos(arg*i*2.) - 0.01168*cos(arg*i*3.);
      break;
    case HAMMING:    //Hamming
      buffer[i]= 0.54 - 0.46*cos(arg*i);
      break;
    case VONHANN:    //Von Hann ("hanning")
      buffer[i]= 0.5 - 0.5*cos(arg*i);
      break;      
    }
  }
  return(buffer);
}

/* window_norm
 * ===========
 * computes the norm of a window
 * returns the norm value
 * win: pointer to a window
 * size: window size
 */
float window_norm(float *win, int size)
{
  float acc=0.0;
  int i;
  for(i=0 ; i<size ; i++){
    acc += win[i];
  }
  return(2.0 / acc);
}

/* push_peak
 * =========
 * pushes a peak into an array of peaks 
 * re-allocating memory and updating its size
 * returns a pointer to the array of peaks.
 * new_peak: pointer to new peak to push into the array
 * peaks_list: list of peaks
 * peaks_size: pointer to the current size of the array.
 */
ATS_PEAK *push_peak(ATS_PEAK *new_peak, ATS_PEAK *peaks_list, int *peaks_size)
{
  peaks_list = (ATS_PEAK *)realloc(peaks_list, sizeof(ATS_PEAK) * ++*peaks_size);
  peaks_list[*peaks_size-1] = *new_peak;
  return(peaks_list);
}

/* peak_frq_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing frequency order.
 */
int peak_frq_inc(void const *a, void const *b)
{
  return(1000.0 * (((ATS_PEAK *)a)->frq - ((ATS_PEAK *)b)->frq));
}

/* peak_amp_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing amplitude order.
 */
int peak_amp_inc(void const *a, void const *b)
{
  return(1000.0 * (((ATS_PEAK *)a)->amp - ((ATS_PEAK *)b)->amp));
}

/* peak_smr_dec
 * ============
 * function used by qsort to sort an array of peaks
 * in decreasing SMR order.
 */
int peak_smr_dec(void const *a, void const *b)
{
  return(1000.0 * (((ATS_PEAK *)b)->smr - ((ATS_PEAK *)a)->smr));
}

/* fft
 * ===
 * standard fft based on simplfft by Joerg Arndt.
 * rl: pointer to real part data 
 * im: pointer to imaginary part data
 * n: size of data
 * is: 1=forward trasnform -1=backward transform
 */
void fft(double *rl, double *im, int n, int is)
{
  int m, j, mh, ldm, lg, i, i2, j2, imh;
  double ur, ui, u, vr, vi, angle, c, s;
  imh = (int)(log(n + 1) / log(2.0));
  fft_bit_reversal(rl, im, n);
  m = 2;
  ldm = 1;
  mh = n >> 1;
  angle = (M_PI * is);
  for (lg = 0; lg < imh; lg++)
    {
      c = cos(angle);
      s = sin(angle);
      ur = 1.0;
      ui = 0.0;
      for (i2 = 0; i2 < ldm; i2++)
	{
	  i = i2;
	  j = i2 + ldm;
	  for (j2 = 0; j2 < mh; j2++)
	    {
	      vr = ur * rl[j] - ui * im[j];
	      vi = ur * im[j] + ui * rl[j];
	      rl[j] = rl[i] - vr;
	      im[j] = im[i] - vi;
	      rl[i] += vr;
	      im[i] += vi;
	      i += m;
	      j += m;
	    }
	  u = ur;
	  ur = (ur * c) - (ui * s);
	  ui = (ui * c) + (u * s);
	}
      mh >>= 1;
      ldm = m;
      angle *= 0.5;
      m <<= 1;
    }
}

/* bit reversal */
void fft_bit_reversal(double* rl, double* im, int n)
{
  int i, m, j;
  double vr, vi;
  j = 0;
  for (i = 0; i < n; i++) {
    if (j > i) {
      vr = rl[j];
      vi = im[j];
      rl[j] = rl[i];
      im[j] = im[i];
      rl[i] = vr;
      im[i] = vi;
    }
    m = n >> 1;
    while ((m >= 2) && (j >= m)) {
      j -= m;
      m = m >> 1;
    }
    j += m;
  }
}
