/* residual.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

/* private function prototypes */
int compute_m(double pha_1, double frq_1, double pha, double frq, int buffer_size);
double compute_aux(double pha_1, double pha, double frq_1, int buffer_size, int M);
double compute_alpha(double aux, double frq_1, double frq, int buffer_size);
double compute_beta(double aux, double frq_1, double frq, int buffer_size);
double interp_phase(double pha_1, double frq_1, double alpha, double beta, int i);
void read_frame(mus_sample_t **fil, int fil_len, int samp_1, int samp_2, double *in_buffer);
void synth_buffer(double a1,double a2, double f1, double f2, double p1, double p2, double *buffer, int frame_samps);


/* Functions for phase interpolation 
 * All this comes from JOS/XJS article on PARSHL.
 * Original phase interpolation eqns. by Qualtieri/McAulay.
 */

int compute_m(double pha_1, double frq_1, double pha, double frq, int buffer_size) 
{
  int val;
  val = (int)((((pha_1 + (frq_1 * (double)buffer_size) - pha) + ((frq - frq_1) * 0.5 * (double)buffer_size)) / TWOPI) + 0.5);
  return(val);
}


double compute_aux(double pha_1, double pha, double frq_1, int buffer_size, int M)
{
  double val;
  val = (double)((pha + (TWOPI * (double)M)) - (pha_1 + (frq_1 * (double)buffer_size)));
  return(val);
}

double compute_alpha(double aux, double frq_1, double frq, int buffer_size)
{
  double val;
  val = (double)(((3.0 / (double)(buffer_size * buffer_size)) * aux ) - ((frq - frq_1) / (double)buffer_size));
  return(val);
}

double compute_beta(double aux, double frq_1, double frq, int buffer_size)
{
  double val;
  val = (double)(((-2.0 / (double)(buffer_size * buffer_size * buffer_size)) * aux) + ((frq - frq_1) / (double)(buffer_size * buffer_size)));
  return(val);
}

double interp_phase(double pha_1, double frq_1, double alpha, double beta, int i) 
{
  double val;
  val = (double)((beta * (double)(i * i * i)) + (alpha * (double)(i * i)) + (frq_1 * (double)i) + pha_1);
  return(val);
}


/* read_frame
 * ==========
 * reads a frame from the input file
 * fil: pointer to an array with sound data
 * fil_len: length of datas in samples
 * samp_1: first sample number in frame
 * samp_2: last sample number in frame
 * in_buffer: pointer to input buffer 
 * which is filled out by the function
 * NOTE: caller should allocate memory for buffer
 */
void read_frame(mus_sample_t **fil, int fil_len, int samp_1, int samp_2, double *in_buffer)
{
  int i, samps, index;
  mus_sample_t tmp;
  samps = samp_2 - samp_1;
  for(i = 0 ; i < samps ; i++){
    index = samp_1 + i;
    if(index < fil_len){
      tmp = fil[0][index];
    } else {
      tmp = (mus_sample_t)0.0;
    }
    in_buffer[i] = MUS_SAMPLE_TO_FLOAT(tmp);
  }
}


/* synth_buffer
 * ============
 * synthesizes a buffer of sound using
 * amplitude linear interpolation and
 * phase cubic interpolation
 * a1: strating amplitude 
 * a2: ending amplitude
 * f1: starting frequency in radians per sample
 * f2: ending frequency in radians per sample
 * p1: starting phase in radians
 * p2: ending phase in radians
 * buffer: pointer to synthsis buffer
 * which is filled out by the function
 * NOTE: caller should allocate memory for buffer
 * frame_samps: number of samples in frame (buffer)
 */
void synth_buffer(double a1,double a2, double f1, double f2, double p1, double p2, double *buffer, int frame_samps)
{
  int k, M;
  double aux, alpha, beta, amp, amp_inc, int_pha;
  M = compute_m(p1, f1, p2, f2, frame_samps);
  aux = compute_aux(p1, p2, f1, frame_samps, M);
  alpha = compute_alpha(aux, f1, f2, frame_samps);
  beta = compute_beta(aux, f1, f2, frame_samps);
  amp = a1;
  amp_inc = (a2 - a1) / (double)frame_samps;
  for(k = 0; k < frame_samps; k++){
    int_pha = interp_phase(p1, f1, alpha, beta, k);
    buffer[k] += amp * cos(int_pha);
    amp += amp_inc;
 }
}


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
void compute_residual(mus_sample_t **fil, int fil_len, char *output_file, ATS_SOUND *sound, int *win_samps, int file_sampling_rate)
{
  int i, frm, frm_1, frm_2, par, frames, partials, frm_samps, out_smp=0, ptout;
  double *in_buff, *synth_buff, mag, a1, a2, f1, f2, p1, p2, diff, synth;
  mus_sample_t **obuf;

  fprintf(stderr, "Computing residual...\n");

  frames = sound->frames;
  partials = sound->partials;
  frm_samps = sound->frame_size;
  mag = TWOPI / (double)file_sampling_rate;
  in_buff = (double *)malloc(frm_samps * sizeof(double));
  synth_buff = (double *)malloc(frm_samps * sizeof(double));
  // open output file 
  if((ptout=mus_sound_open_output(output_file,file_sampling_rate,2,MUS_LSHORT,MUS_RIFF,"created by ATSA"))==-1) {
    fprintf(stderr, "ERROR: can't open file %s for writing\n", output_file);  
    exit(1);
  } 
  // allocate memory
  obuf = (mus_sample_t **)malloc(2*sizeof(mus_sample_t *));
  obuf[0] = (mus_sample_t *)calloc(frm_samps, sizeof(mus_sample_t));
  obuf[1] = (mus_sample_t *)calloc(frm_samps, sizeof(mus_sample_t));
  // computer residual frame by frame
  for(frm = 1 ; frm < frames ; frm++){
    // clean buffers up
    for(i = 0; i < frm_samps ; i++){
      in_buff[i] = 0.0;
      synth_buff[i] = 0.0;
    }
    frm_1 = frm - 1;
    frm_2 = frm;
    // read frame from input
    read_frame(fil, fil_len, win_samps[frm_1], win_samps[frm_2], in_buff);
    // compute one synthesis frame
    for(par = 0 ; par < partials; par++){
      a1 = sound->amp[par][frm_1];
      a2 = sound->amp[par][frm_2];
      //  have to convert the frequency into radians per sample!!!
      f1 = sound->frq[par][frm_1];
      f2 = sound->frq[par][frm_2];
      f1 *= mag;
      f2 *= mag;
      p1 = sound->pha[par][frm_1];
      p2 = sound->pha[par][frm_2];
      if( !( a1 <= 0.0 && a2 <= 0.0 ) ) {
	// check amp 0 in frame 1
	if( a1 <= 0.0 ){
	  f1 = f2;
	  p1 =  p2 - ( f2 * frm_samps );
      	  while(p1 > PI){ p1 -= TWOPI; }
      	  while(p1 < (PI * -1)){ p1 += TWOPI; }
      	}
	// check amp 0 in frame 2
      	if( a2 <= 0.0 ){
      	  f2 = f1;
      	  p2 = p1 + ( f1 * frm_samps );
      	  while(p2 > PI){ p2 -= TWOPI; }
      	  while(p2 < (PI * -1)){ p2 += TWOPI; }
      	}
	synth_buffer(a1, a2, f1, f2, p1, p2, synth_buff, frm_samps);
      }
    }
    // write output: chan 0 residual chan 1 synthesis
    for(i = 0 ; i < frm_samps ; i++){
      synth = synth_buff[i];
      diff = in_buff[i] - synth;
      obuf[0][i] = MUS_FLOAT_TO_SAMPLE(diff);
      obuf[1][i] = MUS_FLOAT_TO_SAMPLE(synth);
      out_smp++;
    }
    mus_sound_write(ptout, 0, frm_samps-1, 2, obuf);
  }
  free(in_buff);
  free(synth_buff);
  // update header and close output file
  mus_sound_close_output(ptout,2 * out_smp * mus_data_format_to_bytes_per_sample(MUS_LSHORT));
  free(obuf[0]);
  free(obuf[1]);
  free(obuf);
}
