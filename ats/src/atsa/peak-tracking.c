/* peak-tracking.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private types */
typedef struct {
  int size;
  ATS_PEAK *cands;
} ATS_CANDS;

/* private function prototypes */
ATS_PEAK *find_candidates(ATS_PEAK *peaks, int peaks_size, double lo, double hi, int *cand_size);
void sort_candidates(ATS_CANDS *cands, ATS_PEAK peak, float SMR_cont);

/* peak_tracking
 * =============
 * connects peaks from one analysis frame to tracks
 * returns a pointer to two frames of orphaned peaks.
 * tracks: pointer to the tracks
 * tracks_size: numeber of tracks
 * peaks: peaks to connect
 * peaks_size: number of peaks
 * frq_dev: frequency deviation from tracks
 * SMR_cont: contribution of SMR to tracking
 * n_partials: pointer to the number of partials before tracking
 */
ATS_FRAME *peak_tracking(ATS_PEAK *tracks, int *tracks_size, ATS_PEAK *peaks, int *peaks_size, float frq_dev, float SMR_cont, int *n_partials)
{
  ATS_CANDS *track_candidates = (ATS_CANDS *)malloc(*peaks_size*sizeof(ATS_CANDS));
  double lo, hi;
  int k, j, used, goback;
  ATS_FRAME *returned_peaks = (ATS_FRAME *)malloc(2*sizeof(ATS_FRAME));

  returned_peaks[0].peaks = returned_peaks[1].peaks = NULL;
  returned_peaks[0].n_peaks = returned_peaks[1].n_peaks = 0;

  /* sort data to prepare for matching */
  qsort(tracks, *tracks_size, sizeof(ATS_PEAK), peak_frq_inc);
  qsort(peaks, *peaks_size, sizeof(ATS_PEAK), peak_frq_inc);

  /* find candidates for each peak and set each peak to best candidate */
  for (k=0; k<*peaks_size; k++) {
    /* find frq limits for candidates */
    lo = peaks[k].frq - (.5 * peaks[k].frq * frq_dev);
    hi = peaks[k].frq + (.5 * peaks[k].frq * frq_dev);
    /* get possible candidates */
    track_candidates[k].size = 0;
    track_candidates[k].cands = find_candidates(tracks, *tracks_size, lo, hi, &track_candidates[k].size);
    if(track_candidates[k].size) {
      sort_candidates(&track_candidates[k], peaks[k], SMR_cont);
      peaks[k].track = track_candidates[k].cands[0].track;
    } 
  }      

  /* compare adjacent peaks track numbers to insure unique track numbers */
  do {
    goback = 0;
    for (j=0; j<(*peaks_size - 1); j++) 
      if((peaks[j].track == peaks[j+1].track) && (peaks[j].track > -1)) {
        if(track_candidates[j].cands[0].amp > track_candidates[j+1].cands[0].amp) {
          track_candidates[j].cands[0].amp = ATSA_HFREQ;
          qsort(track_candidates[j].cands, track_candidates[j].size, sizeof(ATS_PEAK), peak_amp_inc);
          if(track_candidates[j].cands[0].amp < ATSA_HFREQ) {
            peaks[j].track = track_candidates[j].cands[0].track;
            goback = 1;
          } else peaks[j].track = -1;
        } else {
          track_candidates[j+1].cands[0].amp =  ATSA_HFREQ;
          qsort(track_candidates[j+1].cands, track_candidates[j+1].size, sizeof(ATS_PEAK), peak_amp_inc);
          if(track_candidates[j+1].cands[0].amp < ATSA_HFREQ)
            peaks[j+1].track = track_candidates[j+1].cands[0].track;
          else peaks[j+1].track = -1;
        }
      }
  } while (goback);

  /* by this point, all peaks will either have a unique track number, or -1 
     now we need to take care of those left behind */
  for(k=0; k<*peaks_size; k++)
    if(peaks[k].track == -1) {
      peaks[k].track = (*n_partials)++;
      returned_peaks[1].peaks = push_peak(&peaks[k], returned_peaks[1].peaks, &returned_peaks[1].n_peaks);
    }

  /* check for tracks that didnt get assigned */
  for(k=0; k<*tracks_size; k++) {
    used = 0;
    for(j=0; j<*peaks_size; j++)
      if(tracks[k].track == peaks[j].track) {
        used = 1;
        break;
      }
    if(!used) returned_peaks[0].peaks = push_peak(&tracks[k], returned_peaks[0].peaks, &returned_peaks[0].n_peaks);
  }

  for (k=0; k<*peaks_size; k++) free(track_candidates[k].cands);
  free(track_candidates);
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
 */
ATS_PEAK *find_candidates(ATS_PEAK *peaks, int peaks_size, double lo, double hi, int *cand_size)
{
  int i;
  ATS_PEAK *cand_list = NULL;

  for(i=0; i<peaks_size; i++) 
    if((lo <= peaks[i].frq) && (peaks[i].frq <= hi))
      cand_list = push_peak(&peaks[i], cand_list, cand_size);

  return(cand_list);
}

/* sort_candidates
 * ===================
 * sorts candidates from best to worst according to frequency and SMR
 * peak_candidates: pointer to an array of candidate peaks
 * peak: the peak we are matching
 * SMR_cont: contribution of SMR to the matching
 */
void sort_candidates(ATS_CANDS *cands, ATS_PEAK peak, float SMR_cont)
{
  int i;

  /* compute delta values and store them in cands.amp 
     (dont worry, the candidate amps are useless otherwise!) */
  for(i=0; i<cands->size; i++)
    cands->cands[i].amp = (fabs(cands->cands[i].frq - peak.frq) + (SMR_cont * fabs(cands->cands[i].smr - peak.smr))) / (SMR_cont + 1);

  /* sort list by amp (increasing) */
  qsort(cands->cands, cands->size, sizeof(ATS_PEAK), peak_amp_inc);

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
