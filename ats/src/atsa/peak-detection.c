/* peak-detection.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h" 

/* private function prototypes */
void parabolic_interp(double alpha, double beta, double gamma, double *offset, double *height);
double phase_interp(double PeakPhase, double OtherPhase, double offset);
void to_polar(ATS_FFT *ats_fft, double *mags, double *phase, int N, double norm);

/* peak_detection
 * ==============
 * detects peaks in a ATS_FFT block
 * returns pointer to an array of detected peaks.
 * ats_fft: pointer to ATS_FFT structure
 * lowest_bin: lowest fft bin to start detection
 * highest_bin: highest fft bin to end detection
 * lowest_mag: lowest magnitude to detect peaks
 * norm: analysis window norm
 * peaks_size: pointer to size of the returned peaks array
 */
ATS_PEAK *peak_detection(ATS_FFT *ats_fft, int lowest_bin, int highest_bin, float lowest_mag, double norm, int *peaks_size)
{
  int k, N = (highest_bin ? highest_bin : ats_fft->size/2);
  int first_bin = (lowest_bin ? ((lowest_bin>2)?lowest_bin:2) : 2);
  double fft_mag = ((double)ats_fft->rate/ats_fft->size), *fftmags, *fftphase;
  double right_bin, left_bin, central_bin, offset;
  ATS_PEAK ats_peak, *peaks = NULL;

  lowest_mag  = (float)db2amp(lowest_mag);

  /* init peak */
  ats_peak.amp = (double)0.0; 
  ats_peak.frq = (double)0.0; 
  ats_peak.pha = (double)0.0;
  ats_peak.smr = (double)0.0;

  fftmags = (double *)malloc(N * sizeof(double));
  fftphase = (double *)malloc(N * sizeof(double));
  /* convert spectrum to polar coordinates */
  to_polar(ats_fft, fftmags, fftphase, N, norm);
  central_bin = fftmags[first_bin - 2];
  right_bin = fftmags[first_bin - 1];
  /* peak detection */
  for (k=first_bin; k<N; k++) {
    left_bin = central_bin;
    central_bin = right_bin;
    right_bin = fftmags[k];
    if((central_bin > (double)lowest_mag) && (central_bin > right_bin) && (central_bin > left_bin)) {
      parabolic_interp(left_bin, central_bin, right_bin, &offset, &ats_peak.amp);
      ats_peak.frq = fft_mag * ((k - 1) + offset);
      ats_peak.pha = (offset < 0.0) ? phase_interp(fftphase[k-2], fftphase[k-1], abs(offset)) : phase_interp(fftphase[k-1], fftphase[k], offset);
      ats_peak.track = -1;
      /* push peak into peaks list */
      peaks = push_peak(&ats_peak, peaks, peaks_size);
    }
  }
  /* free up fftmags and fftphase */
  free(fftmags);
  free(fftphase);
  return(peaks);
}

/* to_polar
 * ========
 * rectangular to polar conversion
 * values are also scaled by window norm
 * and stored into separate arrays of
 * magnitudes and phases.
 * ats_fft: pointer to ATS_FFT structure
 * mags: pointer to array of magnitudes
 * phase: pointer to array of phases
 * N: highest bin in fft data array
 * norm: window norm used to scale magnitudes
 */
void to_polar(ATS_FFT *ats_fft, double *mags, double *phase, int N, double norm)
{
  int k;
  double x, y;

  for(k=0; k<N; k++) {
#ifdef FFTW
    x = ats_fft->data[k][0];
    y = ats_fft->data[k][1];
#else
    x = ats_fft->fdr[k];
    y = ats_fft->fdi[k];
#endif
    mags[k] = norm * sqrt(x*x + y*y);
    phase[k] = ((x==0.0 && y==0.0) ? 0.0 : atan2(-y, x));
  }
}


/* parabolic_interp
 * ================
 * parabolic peak interpolation
 */
void parabolic_interp(double alpha, double beta, double gamma, double *offset, double *height)  
{
  double dbAlpha = amp2db(alpha), dbBeta = amp2db(beta), dbGamma = amp2db(gamma);
  *offset = .5 * ((dbAlpha - dbGamma) / (dbAlpha - 2*dbBeta + dbGamma));
  *height = db2amp(dbBeta - ((dbAlpha - dbGamma) * .25 * *offset));
}

/* phase_interp
 * ============
 * phase interpolation
 */
double phase_interp(double PeakPhase, double RightPhase, double offset)
{
  if ((PeakPhase - RightPhase) > PI*1.5) RightPhase += TWOPI;
  else if ((RightPhase - PeakPhase) > PI*1.5) PeakPhase += TWOPI;
  return ((RightPhase - PeakPhase) * offset + PeakPhase);
}

