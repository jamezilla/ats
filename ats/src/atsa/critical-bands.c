/* critical-bands.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
void clear_mask(ATS_PEAK *peaks, int peaks_size);
double compute_slope_r(double val);
double frq2bark(double frq, double *edges);
int find_band(double frq, double *edges);

/* frq2bark
 * ========
 * frequency to bark scale conversion 
 */
double frq2bark(double frq, double *edges)
{
  double lo_frq, hi_frq;
  int band;

  if(frq <= 400.0) return(frq * .01);
  if(frq >= 20000.0) return(NIL);

  band = find_band(frq, edges);
  lo_frq = edges[band];
  hi_frq = edges[band+1];
  return(1 + band + fabs(log10(frq/lo_frq) / log10(lo_frq/hi_frq)));
}

/* find_band
 * =========
 * returns the critical band number
 * corresponding to frq
 */
int find_band(double frq, double *edges)
{
  int i = 0;
  while(frq > edges[i++]);
  return(i-2);
}

/* compute_slope_r
 * ===============
 * computes masking curve's right slope from val
 */
double compute_slope_r(double val)
{
  double i = val - 40.0;
  return(((i > 0.0) ? i : 0.0) * 0.37 - 27.0);
}

/* clear_mask
 * ==========
 * clears masking curves
 * peaks: array of peaks representing the masking curve
 * peaks_size: number of peaks in curve
 */
void clear_mask(ATS_PEAK *peaks, int peaks_size)
{
  while(peaks_size--) peaks[peaks_size].smr = 0.0;
}

/* evaluate_smr
 * ============
 * evalues the masking curves of an analysis frame
 * setting the peaks smr slot.
 * peaks: pointer to an array of peaks
 * peaks_size: number of peaks
 */
void evaluate_smr(ATS_PEAK *peaks, int peaks_size)
{
  double slope_l = -27.0, slope_r, delta_dB = -50.0;
  double frq_masker, amp_masker, frq_maskee, amp_maskee, mask_term;
  int i, j;
  ATS_PEAK *maskee;
  double edges[ATSA_CRITICAL_BANDS+1] = ATSA_CRITICAL_BAND_EDGES;
  clear_mask(peaks, peaks_size);
  if(peaks_size == 1) peaks[0].smr = amp2db_spl(peaks[0].amp);
  else for(i=0; i<peaks_size; i++) {
    maskee = &peaks[i];
    frq_maskee = frq2bark(maskee->frq, edges);
    amp_maskee = amp2db_spl(maskee->amp);
    for(j=0; j<peaks_size; j++) 
      if(i != j) {
        frq_masker = frq2bark(peaks[j].frq, edges);
        amp_masker = amp2db_spl(peaks[j].amp);
        slope_r = compute_slope_r(amp_masker);
        mask_term = (frq_masker < frq_maskee) ? 
          (amp_masker + delta_dB + (slope_r * (frq_maskee - frq_masker))) :
          (amp_masker + delta_dB + (slope_l * (frq_masker - frq_maskee)));
        if(mask_term > maskee->smr) maskee->smr = mask_term;
      }
    maskee->smr = amp_maskee - maskee->smr;
  }
}
