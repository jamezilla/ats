/* peak-tracking.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
ATS_PEAK *find_candidates(ATS_PEAK *peaks, int peaks_size, double lo, double hi, int *cand_size);
ATS_PEAK *find_best_candidate(ATS_PEAK *peak_candidates, int cand_size, double peak_frq, double peak_smr, float alpha, ATS_PEAK *tracks, int tracks_size);

#define LARGEST_DOUBLE 1.7976931348623157e308

/* peak_tracking
 * =============
 * connects peaks from one analysis frame to tracks
 * returns a pointer to the analysis frame.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * peaks: peaks to connect
 * peaks_size: number of peaks
 * frq_dev: frequency deviation from tracks
 * SMR_cont: contribution of SMR to tracking
 * n_partials: pointer to the number of partials before tracking
 */
ATS_FRAME *peak_tracking(ATS_PEAK *tracks, int tracks_size, ATS_PEAK *peaks, int *peaks_size, float frq_dev, float SMR_cont, int *n_partials)
{
  ATS_PEAK track, *matched_peak = NULL, *peak_candidates = NULL;
  double track_frq, track_smr, lo, hi;
  int h, i, k, j, cand_size;
  ATS_FRAME *returned_peaks = (ATS_FRAME *)malloc(2*sizeof(ATS_FRAME));

  returned_peaks[0].peaks = returned_peaks[1].peaks = NULL;
  returned_peaks[0].n_peaks = returned_peaks[1].n_peaks = 0;

  if(tracks_size && *peaks_size) {
    qsort(peaks, *peaks_size, sizeof(ATS_PEAK), peak_frq_inc);
    for (k=0; k<tracks_size; k++) {
      track = tracks[k];
      track_frq = track.frq;
      track_smr = track.smr;
      /* find frq limits for candidates */
      lo = track_frq - (.5 * track_frq * frq_dev);
      hi = track_frq + (.5 * track_frq * frq_dev);
      /* get possible candidates */
      cand_size = 0;
      peak_candidates = find_candidates(peaks, *peaks_size, lo, hi, &cand_size);
      /* find best candidate */
      matched_peak = find_best_candidate(peak_candidates, cand_size, track_frq, track_smr, SMR_cont, tracks, tracks_size);
      if(matched_peak != NULL) {
        for(i=0; i<*peaks_size; i++) 
          if(peaks[i].frq == matched_peak->frq) {
	    if( matched_peak->track >= 0){
	      /* put previously holding track into unmatched peaks */
	      for(j=0; j<tracks_size; j++){
		if(tracks[j].track == matched_peak->track) break;
	      } 
	      returned_peaks[0].peaks = push_peak(&tracks[j], returned_peaks[0].peaks, &returned_peaks[0].n_peaks);
	    }
	    peaks[i].track = track.track;
	    break;
	  }
      } else {
	returned_peaks[0].peaks = push_peak(&track, returned_peaks[0].peaks, &returned_peaks[0].n_peaks);
      }
      free(peak_candidates);
      peak_candidates = NULL;
    }      
  }
  for(i=0; i<*peaks_size; i++)
    if(peaks[i].track < 0) {
      peaks[i].track = (*n_partials)++;
      returned_peaks[1].peaks = push_peak(&peaks[i], returned_peaks[1].peaks, &returned_peaks[1].n_peaks);
    }
  return(returned_peaks);
}

/* find_candidates
 * ===============
 * find candidates to continue a track form an array of peaks
 * returns a pointer to an array of candidates
 * peaks: pointer to array of peaks
 * peaks_size: number of peaks
 * lo: lowest frequency to consider candidates
 * hi: highest frequency to consider candidates
 * cand_size: pointer to the number of candidates returned
 * Note: this function assumes peaks were sorted 
 * by increasing freq by caller
 */
ATS_PEAK *find_candidates(ATS_PEAK *peaks, int peaks_size, double lo, double hi, int *cand_size)
{
  int i;
  ATS_PEAK *cand_list = NULL;

  for(i=0; i<peaks_size; i++) 
    if((lo <= peaks[i].frq) && (peaks[i].frq <= hi)) {
      cand_list = push_peak(&peaks[i], cand_list, cand_size);
    }
  return(cand_list);
}

/* find_best_candidate
 * ===================
 * finds best candidate to continue a track form a pool of peaks 
 * returns a pointer to the best candidate peak
 * peak_candidates: pointer to an array of candidate peaks
 * cand_size: number of candidates
 * track_frq: frequency of track
 * track_smr: SMR of track 
 * SMR_cont: contribution of SMR to the matching
 * tracks: pointer to array of tracks
 * tracks_size: number of tracks
 */
ATS_PEAK *find_best_candidate(ATS_PEAK *peak_candidates, int cand_size, double track_frq, double track_smr, float SMR_cont, ATS_PEAK *tracks, int tracks_size)
{
  ATS_PEAK *best_peak = NULL;
  double local_delta, delta = LARGEST_DOUBLE;
  int i, k;

  for(i=0; i<cand_size; i++) {
    local_delta = ((fabs(peak_candidates[i].frq - track_frq) + (SMR_cont * fabs(peak_candidates[i].smr - track_smr))) / (SMR_cont + 1));
    /* check if peak has been claimed */
    if(peak_candidates[i].track >= 0) {
      /* find track holding candidate */
      for(k=0; k<tracks_size; k++){
	if(tracks[k].track == peak_candidates[i].track) break;
      }  
      /* see if current delta is greater */
      if(local_delta > ((fabs(tracks[k].frq - peak_candidates[i].frq) + (SMR_cont * fabs(tracks[k].smr - peak_candidates[i].smr))) / (SMR_cont + 1))) {
	continue;
      } 
    }
    if(local_delta < delta) {
      best_peak = &peak_candidates[i];
      delta = local_delta;
      }
  }
  return(best_peak);
}
      
/* update_tracks
 * =============
 * updates analysis tracks
 * returns a pointer to the tracks.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * track_len: length of tracks
 * frame_n: analysis frame number 
 * ana_frames: pointer to previous analysis frames
 * last_peak_cont: contribution of last peak to the track
 */
ATS_PEAK *update_tracks (ATS_PEAK *tracks, int *tracks_size, int track_len, int frame_n, ATS_FRAME *ana_frames, float last_peak_cont)
{
  int frames, first_frame, track, g, i, k;
  double frq_acc, last_frq, amp_acc, last_amp, smr_acc, last_smr;
  int f, a, s;
  ATS_PEAK *l_peaks, *peak;

  if (tracks != NULL) {
    frames = (frame_n < track_len) ? frame_n : track_len;
    first_frame = frame_n - frames;
    for(g=0; g<*tracks_size; g++) {
      track = tracks[g].track;
      frq_acc = last_frq = amp_acc = last_amp = smr_acc = last_smr = 0.0;
      f = a = s = 0;
      for(i=first_frame; i<frame_n; i++) {
        l_peaks = ana_frames[i].peaks;
        peak = NULL;
        for(k=0; k<ana_frames[i].n_peaks; k++)
          if(l_peaks[k].track == track) {
            peak = &l_peaks[k];
            break;
          }
        if(peak != NULL) {
          if (peak->frq > 0.0) {
            last_frq = peak->frq;
            frq_acc += peak->frq;
            f++;
          }
          if (peak->amp > 0.0) {
            last_amp = peak->amp;
            amp_acc += peak->amp;
            a++;
          }
          if (peak->smr > 0.0) {
            last_smr = peak->smr;
            smr_acc += peak->smr;
            s++;
          }
        }
      }
      if(f) tracks[g].frq = (last_peak_cont * last_frq) + ((1 - last_peak_cont) * (frq_acc / f));
      if(a) tracks[g].amp = (last_peak_cont * last_amp) + ((1 - last_peak_cont) * (amp_acc / a));
      if(s) tracks[g].smr = (last_peak_cont * last_smr) + ((1 - last_peak_cont) * (smr_acc / s));
    }
  } else 
    for(g=0; g<ana_frames[frame_n-1].n_peaks; g++)
      tracks = push_peak(&ana_frames[frame_n-1].peaks[g], tracks, tracks_size);

  return(tracks);
}
