/* atsa.h
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#ifdef FFTW
#include <fftw3.h>
#endif

/* sndlib stuff starts here */
#if defined(HAVE_CONFIG_H)
#include <config.h>
#else
#define _FILE_OFFSET_BITS 64
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#if (defined(NEXT) || (defined(HAVE_LIBC_H) && (!defined(HAVE_UNISTD_H))))
#include <libc.h>
#else
#if (!(defined(_MSC_VER))) && (!(defined(MPW_C)))
#include <unistd.h>
#endif
#include <string.h>
#endif
#include <errno.h>
#include "./atsa-sndlib/sndlib.h" 
#include "./atsa-sndlib/clm.h" 
/* sndlib stuff ends here */

/*  window types */
#define  BLACKMAN   0
#define  BLACKMAN_H 1
#define  HAMMING    2
#define  VONHANN    3

/********** ANALYSIS PARAMETERS ***********/
/* start time */
#define  ATSA_START 0.0
/* duration */
#define  ATSA_DUR 0.0
/* lowest frequency (hertz)  */
#define  ATSA_LFREQ 20.0
/* highest frequency (hertz) */
#define  ATSA_HFREQ 20000.0
/* frequency deviation (ratio) */
#define  ATSA_FREQDEV 0.1
/* number of f0 cycles in window */
#define  ATSA_WCYCLES 4
/* window type */
#define  ATSA_WTYPE BLACKMAN_H
/* window size */
#define  ATSA_WSIZE 1024
/* hop size proportional to window size (ratio) */
#define  ATSA_HSIZE 0.25
/* lowest magnitude for peaks (amp) */
#define  ATSA_LMAG  -60.0
/* length of analysis tracks (frames) */
#define  ATSA_TRKLEN 3
/* minimum short partial length (frames) */
#define  ATSA_MSEGLEN 3
/* minimum short partial SMR avreage (dB SPL) */
#define  ATSA_MSEGSMR 60.0 
/* minimum gap length (frames) */
#define  ATSA_MGAPLEN 3
/* threshold for partial SMR average (dB SPL) */
#define  ATSA_SMRTHRES 30.0
/* last peak contribution for tracking (ratio) */
#define  ATSA_LPKCONT 0.0
/* SMR contribution for tracking (ratio) */
#define  ATSA_SMRCONT 0.5
/* minimum number of frames for analysis (frames) */
#define ATSA_MFRAMES 4
/* default analysis file type 
 * 1 =only amp. and freq.
 * 2 =amp., freq. and phase
 * 3 =amp., freq. and noise
 * 4 =amp., freq., phase, and noise  
 */
#define ATSA_TYPE 4
/* default residual file */
#define ATSA_RES_FILE "/tmp/atsa_res.wav"

/* constants and macros */
#define  PI         3.141592653589793
#define  TWOPI      6.283185307179586
#define  NIL        -1
#define  AMP_DB(amp) (amp!=0.0 ? (float)log10(amp*20.0) : (float)-32767.0)
#define  DB_AMP(db) ((float)pow(10.0, db/20.0))
#define  ATSA_MAX_DB_SPL 100.0
#define  ATSA_NOISE_THRESHOLD -120
#define  ATSA_CRITICAL_BANDS 25
#define  ATSA_NOISE_VARIANCE 0.04 
/* array of critical band frequency edges base on data from: 
 * Zwicker, Fastl (1990) "Psychoacoustics Facts and Models", 
 * Berlin ; New York : Springer-Verlag
 */
#define ATSA_CRITICAL_BAND_EDGES {0.0, 100.0, 200.0, 300.0, 400.0, 510.0,\
                                      630.0, 770.0, 920.0, 1080.0, 1270.0,\
                                      1480.0, 1720.0, 2000.0, 2320.0, 2700.0,\
                                      3150.0, 3700.0, 4400.0, 5300.0, 6400.0,\
					7700.0, 9500.0, 12000.0, 15500.0, 20000.0}

/* data structures */

/* ANARGS
 * ======
 * analysis parameters
 */
typedef struct {
  /* args[0] is infile, args[1] is outfile */
  char *args[2];  
  float start;
  float duration;
  float lowest_freq;
  float highest_freq;
  float freq_dev;
  int win_cycles;
  int win_type;
  int win_size;
  float hop_size;
  float lowest_mag;
  int track_len;
  int min_seg_len;
  int min_gap_len;
  float last_peak_cont;
  float SMR_cont;
  float SMR_thres;
  float min_seg_SMR;
  /* parameters computed from command line */
  int first_smp;
  int cycle_smp;
  int hop_smp;
  int total_samps;
  int srate;
  int fft_size;
  float fft_mag;
  int lowest_bin;
  int highest_bin;
  int frames;
  int type;
} ANARGS;

/* ATS_FFT
 * fft data
 */
#if defined(FFTW)
typedef struct {
  int size;
  int rate;
  fftw_complex *data;
} ATS_FFT;
#else
typedef struct {
  int size;
  int rate;
  double *fdr;
  double *fdi;
} ATS_FFT;
#endif

/* ATS_PEAK
 * ========
 * spectral peak data
 */
typedef struct {
  double amp; 
  double frq; 
  double pha;
  double smr;
  int track;
} ATS_PEAK;

/* ATS_FRAME
 * =========
 * analysis frame data
 */
typedef struct {
  ATS_PEAK *peaks;
  int n_peaks;
  double time;
} ATS_FRAME;

/* ATS_HEADER
 * ==========
 * ats file header data 
 */
typedef struct { 
  /* Magic Number for ID of file, must be 123.00 */
  double mag; 
  /* sampling rate */
  double sr;  
  /* Frame size (samples) */
  double fs;  
  /* Window size (samples) */
  double ws;  
  /* number of partials per frame */
  double par; 
  /* number of frames present */
  double fra; 
  /* max. amplitude */
  double ma;  
  /* max. frequency */
  double mf;  
  /* duration (secs) */
  double dur; 
  /* type (1,2 3 or 4)  
   * 1 =only amp. and freq.
   * 2 =amp., freq. and phase
   * 3 =amp., freq. and noise
   * 4 =amp., freq., phase, and noise  
   */
  double typ;  
} ATS_HEADER;

/* ATS_SOUND
 * =========
 * ats analysis data
 */
typedef struct {
  /* global sound info */
  int srate;
  int frame_size;
  int window_size;
  int partials;
  int frames;
  double dur;
  /* info deduced from analysis */
  /* we use optimized to keep the 
   * # of partials killed by optimization 
   */
  int optimized;
  double ampmax;
  double frqmax;
  ATS_PEAK *av;
  /* sinusoidal data */
  /* all of these ** are accessed as [partial][frame] */
  double **time;  
  double **frq;
  double **amp;
  double **pha;
  double **smr;
  /* noise data */
  int *bands;
  double **res;
  double **band_energy;
} ATS_SOUND;

/* Interface: 
 * ==========
 * grouped by file in alphabetical order
 */

/* atsa.c */

/* main_anal
 * =========
 * main analysis function
 * soundfile: path to input file 
 * out_file: path to output ats file 
 * anargs: pointer to analysis parameters
 * resfile: path to residual file
 * returns error status
 */
int main_anal (char *soundfile, char *ats_outfile, ANARGS *anargs, char *resfile);

/* critical-bands.c */

/* evaluate_smr
 * ============
 * evalues the masking curves of an analysis frame
 * peaks: pointer to an array of peaks
 * peaks_size: number of peaks
 */
void evaluate_smr (ATS_PEAK *peaks, int peaks_size);

/* other-utils.c */

/* window_norm
 * ===========
 * computes the norm of a window
 * returns the norm value
 * win: pointer to a window
 * size: window size
 */
float window_norm(float *win, int size);

/* make_window
 * ===========
 * makes an analysis window, returns a pointer to it.
 * win_type: window type, available types are:
 * BLACKMAN, BLACKMAN_H, HAMMING and VONHANN
 * win_size: window size
 */
float *make_window(int win_type, int win_size);

/* push_peak
 * =========
 * pushes a peak into an array of peaks 
 * re-allocating memory and updating its size
 * returns a pointer to the array of peaks.
 * new_peak: pointer to new peak to push into the array
 * peaks_list: list of peaks
 * peaks_size: pointer to the current size of the array.
 */
ATS_PEAK *push_peak(ATS_PEAK *new_peak, ATS_PEAK *peaks, int *peaks_size);

/* peak_frq_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing frequency order.
 */
int peak_frq_inc(void const *a, void const *b);

/* peak_amp_inc
 * ============
 * function used by qsort to sort an array of peaks
 * in increasing amplitude order.
 */
int peak_amp_inc(void const *a, void const *b);

/* peak_smr_dec
 * ============
 * function used by qsort to sort an array of peaks
 * in decreasing SMR order.
 */
int peak_smr_dec(void const *a, void const *b);

#ifndef FFTW
/* fft
 * ===
 * standard fft based on simplfft by Joerg Arndt.
 * rl: pointer to real part data 
 * im: pointer to imaginary part data
 * n: size of data
 * is: 1=forward trasnform -1=backward transform
 */
void fft_slow(double *rl, double *im, int n, int is);
#endif

/* peak-detection.c */

/* peak_detection
 * ==============
 * detects peaks in a ATS_FFT block
 * returns an array of detected peaks.
 * ats_fft: pointer to ATS_FFT structure
 * lowest_bin: lowest fft bin to start detection
 * highest_bin: highest fft bin to end detection
 * lowest_mag: lowest magnitude to detect peaks
 * norm: analysis window norm
 * peaks_size: pointer to size of the returned peaks array
 */
ATS_PEAK *peak_detection(ATS_FFT *ats_fft, int lowest_bin, int highest_bin, float lowest_mag, double norm, int *peaks_size);

/* peak-tracking.c */

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
ATS_FRAME *peak_tracking(ATS_PEAK *tracks, int *tracks_size, ATS_PEAK *peaks, int *peaks_size, float frq_dev, float SMR_cont, int *n_partials);

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
ATS_PEAK *update_tracks (ATS_PEAK *tracks, int *tracks_size, int track_len, int frame_n, ATS_FRAME *ana_frames, float last_peak_cont);

/* save-load-sound.c */

/* ats_save
 * ========
 * saves an ATS_SOUND to disk.
 * sound: pointer to ATS_SOUND structure
 * outfile: pointer to output ats file
 * SMR_thres: partials with and avreage SMR 
 * below this value are considered masked
 * and not written out to the ats file
 * type: file type
 * NOTE: sound MUST be optimized using optimize_sound
 * before calling this function
 */
void ats_save(ATS_SOUND *sound, FILE *outfile, float SMR_thres, int type);


/* tracker.c */

/* tracker
 * =======
 * partial tracking function 
 * returns an ATS_SOUND with data issued from analysis
 * anargs: pointer to analysis parameters
 * soundfile: path to input file 
 * resfile: path to residual file 
 */
ATS_SOUND *tracker (ANARGS *anargs, char *soundfile, char *resfile);

/* utilities.c */

/* ppp2
 * ====
 * returns the closest power of two
 * greater than num
 */
unsigned int ppp2(int num);

/* various conversion functions
 * to deal with dB and dB SPL
 * they take and return double floats
 */
double amp2db(double amp);
double db2amp(double db);
double amp2db_spl(double amp);
double db2amp_spl(double db_spl);

/* optimize_sound
 * ==============
 * optimizes an ATS_SOUND in memory before saving
 * anargs: pointer to analysis parameters
 * sound: pointer to ATS_SOUND structure
 */
void optimize_sound(ANARGS *anargs, ATS_SOUND *sound);

/* residual.c */

/* compute_residual
 * ================
 * Computes the difference between the synthesis and the original sound. 
 * the <win-samps> array contains the sample numbers in the input file corresponding to each frame
 * fil: pointer to analyzed data
 * fil_len: length of data in samples
 * output_file: output file path
 * sound: pointer to ATS_SOUND
 * win_samps: pointer to array of analysis windows center times
 * file_sampling_rate: sampling rate of analysis file
 */
void compute_residual(mus_sample_t **fil, int fil_len, char *output_file, ATS_SOUND *sound, int *win_samps, int file_sampling_rate);

/* residual-analysis.c */

/* residual_analysis
 * =================
 * performs the critical-band analysis of the residual file
 * file: name of the sound file containing the residual 
 * sound: sound to store the residual data 
 */
void residual_analysis(char *file, ATS_SOUND *sound);

/* band_energy_to_res
 * ==================
 * transfers residual engergy from bands to partials
 * sound: sound structure containing data
 * frame: frame number
 */
void band_energy_to_res(ATS_SOUND *sound, int frame);

/* res_to_band_energy
 * ==================
 * transfers residual engergy from partials to bands
 * sound: sound structure containing data
 * frame: frame number
 */
void res_to_band_energy(ATS_SOUND *sound, int frame);


/* init_sound
 * ==========
 * initializes a new sound allocating memory
 */
void init_sound(ATS_SOUND *sound, int sampling_rate, int frame_size, int window_size, int frames, double duration, int partials, int use_noise);

/* free_sound
 * ==========
 * frees sound's memory
 */
void free_sound(ATS_SOUND *sound);
