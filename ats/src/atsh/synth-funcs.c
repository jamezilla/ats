/*
SYNTH-FUNCS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
#include "my_curve.h"

#define ENG_RMS(val, ws) sqrt((double)val/(ws * (float)ATSA_NOISE_VARIANCE))
#define SYNTH_RES  1
#define SYNTH_DET  2
#define SYNTH_BOTH 3

typedef struct { //the data for the randi UG
  int   size; //size of the frame in samples this should be sr/freq.
  float a1;   //first amplitude value
  float a2;   //next  amplitude value
  int   cnt;  //sample position counter
} RANDI;

void synth_buffer_phint(float a1, float a2, float f1, float f2, float p1, float p2, float dt, float frame_samps);
float ioscilator(float amp,float freq,int op,float *oscpt);
int locate_frame(int from_frame, float time, float dif);
void set_output_type(int *format, int *header);
void randi_setup(float sr, float freq, RANDI *radat);
float randi(RANDI *radat);
float randif(RANDI *radat, float freq);
void synth_deterministic_only(float a1, float a2, float f1, float f2, float frame_samps, int op, float *oscpt);
void synth_residual_only(float a1, float a2,float freq,float frame_samps,int op,float *oscpt, RANDI* rdata);
void synth_both(float a1, float a2, float f1, float f2, float frame_samps,int op, float *oscpt,float r1, float r2, RANDI* rdata);

double tl_sr;
extern float *sine_table;
extern char out_title[];
extern char ats_title[];
extern SPARAMS sparams;
extern ATS_SOUND *ats_sound;
extern ATS_HEADER atshed;
extern short outype;


//randi output random numbers in the range of 1,-1
//getting a new number at frequency freq and interpolating
//the intermediate values.
void randi_setup(float sr, float freq, RANDI *radat)
{
  radat->size= (int) (sr / freq) - 1;
  radat->a1  = (float)random();
  radat->a2  = (float)random();
  radat->cnt = 0;
}

float randi(RANDI *radat)
{
  //float output;

  if(radat->cnt == radat->size) { //get a new random value
    radat->a1  = radat->a2; 
    radat->a2  = (float)random();
    radat->cnt = 0;
  }

  //output=(float) (((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1;
  radat->cnt++;
  return(1. - ((((((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1) /RAND_MAX) * 2.));
}

float randif(RANDI *radat, float freq)
{
  //float output;

  if(radat->cnt == radat->size) { //get a new random value
    radat->a1  = radat->a2; 
    radat->a2  = (float)random();
    radat->cnt = 0;
    radat->size= (int) (sparams.sr / freq) - 1;
  }

  //output=(float)(((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1;
  radat->cnt++;
  return(1. - ((((((radat->a2 - radat->a1)/ radat->size) * radat->cnt)+ radat->a1) /RAND_MAX) * 2.));
  //return(1. - ((float)(output /(long int)RAND_MAX) * 2.));
}

void make_sine_table() 
{
  int i;
  float theta=0.;
  float incr = TWOPI / (float)TABLE_LENGTH;

  sine_table= (float *) malloc(TABLE_LENGTH * sizeof(float));

  for(i=0; i < TABLE_LENGTH; i++) {
    sine_table[i] = sin(theta);
    theta +=incr;
  }
}

float ioscilator(float amp,float freq,int op, float *oscpt)
{
  float output;
  float incr = freq * tl_sr; 
  int   v2=0;

  //LINEAR INTERPOLATION
  v2=(int)(oscpt[op] + 1.);
  while (v2 >= TABLE_LENGTH) v2 -=TABLE_LENGTH;
  output=amp *  (sine_table[(int)oscpt[op]] + 	
                 ((sine_table[(int)v2] - sine_table[(int)oscpt[op]]) * ( oscpt[op] - (int)oscpt[op] )));
  
  oscpt[op]  += incr;
  
  while ( oscpt[op] >= (float)TABLE_LENGTH ) oscpt[op] -=(float)TABLE_LENGTH;
  while ( oscpt[op] < 0. ) oscpt[op] +=(float)TABLE_LENGTH;
  
  return(output);
}

//Synthesizes a Buffer using phase interpolation
void synth_buffer_phint(float a1, float a2, float f1, float f2, float p1, float p2, float dt, float frame_samps)
{
  float t_inc, a_inc, M, aux, alpha, beta, time, amp, scale, new_phase;
  int k, index;
  float out=0., phase=0.;

  f1  *=TWOPI;
  f2  *=TWOPI;
  t_inc= dt  / frame_samps;
  a_inc= (a2 - a1) / frame_samps;
  M    = COMPUTE_M(p1, f1, p2, f2,dt);
  aux  = COMPUTE_AUX(p1, p2, f1, dt, M);
  alpha= COMPUTE_ALPHA(aux,f1,f2,dt);
  beta = COMPUTE_BETA(aux,f1,f2,dt);
  time = 0.;
  amp  = a1;
  scale = TWOPI / ((float)TABLE_LENGTH - 1.);/*must take it out from here...*/

  for(k = 0; k < (int)frame_samps; k++) { 
    phase = INTERP_PHASE(p1,f1,alpha,beta,time);
    new_phase = (phase >= TWOPI  ? phase - TWOPI : phase);
    index=(int)((new_phase / TWOPI)*(float)TABLE_LENGTH - 1.);
    while ( index >= TABLE_LENGTH ) index -=TABLE_LENGTH;
    while ( index < 0 ) index +=TABLE_LENGTH;
    out = sine_table[index] * amp;
    time +=t_inc;
    amp  +=a_inc;
    frbuf[k] +=out; //buffer adds each partial at each pass
  }
}

//Synthesizes a Buffer NOT using phase interpolation
void synth_deterministic_only(float a1, float a2, float f1, float f2, float frame_samps,int op, float *oscpt)
{
  int k;
  float  a_inc, f_inc, amp, freq;
  float out=0.;

  if(a1!=0. || a2!=0.) {  //no synthesis if no amplitude
    amp  =  a1;
    freq =  f1;
    a_inc= (a2 - a1) / frame_samps;
    f_inc= (f2 - f1) / frame_samps;
  
    //we should do this conditional on the main loop... 
    if (sparams.allorsel == FALSE)
      for(k = 0; k < (int)frame_samps; k++) { 
        out =ioscilator(amp,freq,op,oscpt) * sparams.amp;  
        amp  +=a_inc;
        freq +=f_inc;
        frbuf[k] +=out; //buffer adds each partial at each pass
      }
    else 
      if(selected[op] == TRUE)
        for(k = 0; k < (int)frame_samps; k++) { 
          out =ioscilator(amp,freq,op,oscpt) * sparams.amp;  
          amp  +=a_inc;
          freq +=f_inc;
          frbuf[k] +=out; //buffer adds each partial at each pass
        }
  }
}

void synth_residual_only(float a1, float a2,float freq,float frame_samps,int op,float *oscpt, RANDI* rdata)
{
  int k, samps=(int)frame_samps;
  float  a_inc, amp;
  float out=0.;
  
  if(a1!=0. || a2!=0.) { //no synthesis if no amplitude
    amp  =  a1;
    a_inc= (a2 - a1) / frame_samps;
    
    for(k=0; k<samps; k++) { 
      out  = ioscilator(amp,freq,op,oscpt);  
      amp  += a_inc;
      frbuf[k] += out * sparams.ramp * randi(rdata); 
    }
  }
}

void synth_both(float a1, float a2, float f1, float f2, float frame_samps,int op, float *oscpt,float r1, float r2, RANDI* rdata)
{
  int k;
  float  a_inc, f_inc, amp, freq;
  float  r_inc, res;
  float  out=0., rsig;
  float  rf1, rf2, rf_inc, rfreq;
  
  if(a1!=0. || a2!=0.) { //no synthesis if no amplitude
    rf1   =(f1< 500.? 50. : f1 * .05);
    rf2   =(f2< 500.? 50. : f2 * .05);
    rf_inc=(rf2 - rf1) / frame_samps;
    rfreq = rf1;
    amp   =  a1;
    freq  =  f1;
    res   =  r1;
    a_inc = (a2 - a1) / frame_samps;
    f_inc = (f2 - f1) / frame_samps;
    r_inc = (r2 - r1) / frame_samps;
  
    //we should do this conditional on the main loop... 
    if (sparams.allorsel == FALSE)
      for(k = 0; k < (int)frame_samps; k++) { 
        out = ioscilator(1.0,freq,op,oscpt);  
        rsig = res*randif(rdata,rfreq);
        frbuf[k] += (out * amp * sparams.amp) + (out * rsig * sparams.ramp); 
        amp   +=  a_inc;
        freq  +=  f_inc;
        res   +=  r_inc;
        rfreq += rf_inc;
      }
    else {
      if(selected[op] == TRUE) 
        for(k = 0; k < (int)frame_samps; k++) { 
          out =ioscilator(1.0,freq,op,oscpt);  
          rsig = res*randif(rdata,rfreq);
          frbuf[k] += (out * amp * sparams.amp) + (out * rsig * sparams.ramp); 	
          amp   +=  a_inc;
          freq  +=  f_inc;
          res   +=  r_inc;
          rfreq += rf_inc;
        }
    }
  }
}

////////////////////////////////////////////////////////////////////
/////////THIS IS THE MAIN SYNTHESIS LOOP////////////////////////////
////////////////////////////////////////////////////////////////////
void do_synthesis()
{
  int i,x,z,j,maxlen, todo, curr, next, sflag=0;
  float dt=0., rfreq;
  float frame_samps, bw=.1;
  int   ptout;
  float maxamp=0.;
  int   bframe=0, eframe=0;
  int   nValue=0;
  char  stamp[16];
  int   written;
  int   format,header;
  mus_sample_t *obuf[1];
  float cxval, cyval, nxval, nyval, difx, dify;
  TIME_DATA *tdata;
  int nbp;
  float *dospt=NULL, *rospt=NULL;
  RANDI *rarray=NULL;
  float res_band_edges[ATSA_CRITICAL_BANDS+1]=ATSA_CRITICAL_BAND_EDGES;
  float res_band_centers[ATSA_CRITICAL_BANDS];
  char str[100];

  //ATTEMPT TO CATCH TWO POSSIBLE ERRORS........ 
  if(*ats_title==0) {
    Popup("ERROR: ATS file undefined,  select it first");
    return; 
  }
  if(*out_title==0) {
    Popup("ERROR: Output Soundfile undefined, select it first");  
    return; 
  }  
  if(sparams.amp==0. && sparams.ramp==0.) {
    Popup("ERROR: Deterministic and Residual output set to zero");  
    return; 
  }  


  //OPEN OUTPUT FILE
  set_output_type(&format, &header);

  if((ptout=mus_sound_open_output(out_title,(int)sparams.sr,1,format,header,"created by ATSH"))==-1) {
    Popup("ERROR: could not open Output Soundfile for writing");  
    return;
  }

  //do residual data transfer
  if (FILE_HAS_NOISE)
    for(i=0; i<(int)atshed.fra; ++i) band_energy_to_res(ats_sound, i);

  //NOW CHECK WHAT TO DO...
  if(sparams.amp > 0.) sflag |= SYNTH_DET;  //deterministic synthesis only
  if(sparams.ramp > 0.) sflag |= SYNTH_RES; //residual synthesis only

  tl_sr = (float)TABLE_LENGTH / sparams.sr; //needed for ioscilator...
  nbp   = get_nbp(timenv->curve);
  tdata = (TIME_DATA*)malloc(nbp * sizeof(TIME_DATA));
  

  //g_print(" \nNPOINTS= %d \n", nbp); 
  sparams.max_stretch=0.;  
  todo=0;

  //We first must calculate data of each breakpoint
  timenv->dur=fabs(timenv->dur); //correct if negative
  for(i=0; i < nbp - 1; ++i){
    //get the data from the time envelope and convert it to time
    cxval= timenv->dur * (get_x_value(timenv->curve,i)/100.);
    cyval= timenv->ymin + ((timenv->ymax - timenv->ymin) * (get_y_value(timenv->curve,i)/100.));
    nxval= timenv->dur * (get_x_value(timenv->curve,i+1)/100.);
    nyval= timenv->ymin + ((timenv->ymax - timenv->ymin) * (get_y_value(timenv->curve,i+1)/100.));
    
    //diff=0. is a special case we must take in account
    //here all we do is to set it to one millisecond (arbitrarly)
    difx= nxval - cxval;
    if(difx == 0.) difx=.001; 
    dify= nyval - cyval;
    if(dify == 0.) dify=.001; 

    //find out the max. stretching factor(needed to alocate the I/O buffer)
    if(fabs(difx) / fabs(dify) >= sparams.max_stretch) {
      sparams.max_stretch=fabs(difx) / fabs(dify);
    }
    
    //locate the frame for the beggining and end of segments
    if(i == 0){
      if(dify < 0.) bframe= locate_frame((int)atshed.fra-1,cyval, dify);
      else bframe= locate_frame(0,cyval, dify);
    }
    eframe= locate_frame(bframe, nyval, dify);
    //collect the data to be used
    tdata[i].from=bframe;
    tdata[i].to  =eframe;
    tdata[i].size= (int)(abs(eframe - bframe)) + 1;
    tdata[i].tfac=fabs(difx) / fabs(dify);
    todo+=tdata[i].size;
    
    //g_print("\n from frame=%d to frame=%d", bframe,eframe);

    bframe=eframe;
    ////////////////////////////////////////////////////////
    
  }
  

  //INITIALIZE PROGRESSBAR
  strcpy(str,"Writing File " );
  strcat(str, out_title);
  StartProgress(str, TRUE);

  //ALLOCATE AND CLEAN AUDIO BUFFERS
  maxlen= (int)ceil(maxtim * sparams.sr * sparams.max_stretch); 
  frbuf = (float *) malloc(maxlen * sizeof(float));
  for(z = 0; z < maxlen; ++z) frbuf[z]=0.;
  obuf[0] = (mus_sample_t *)calloc(maxlen, sizeof(mus_sample_t));

  switch(sflag) { //see which memory resources do we need and allocate them
  case SYNTH_DET: 
    dospt = (float *) malloc( (int)atshed.par * sizeof(float));
    for(z=0; z<(int)atshed.par; ++z) dospt[z]=0.;
    break;
    
  case SYNTH_RES: 
    rospt = (float *) malloc((int)ATSA_CRITICAL_BANDS * sizeof(float)); 
    rarray= (RANDI *) malloc((int)ATSA_CRITICAL_BANDS * sizeof(RANDI));
    for(z=0; z<(int)ATSA_CRITICAL_BANDS; ++z) {
      res_band_centers[z]= res_band_edges[z]+((res_band_edges[z+1]-res_band_edges[z])*0.5); 
      randi_setup(sparams.sr,res_band_edges[z+1]-res_band_edges[z],&rarray[z]);
      rospt[z]=0.;
    }
    break;

  case SYNTH_BOTH: 
    dospt = (float *) malloc( (int)atshed.par * sizeof(float));
    rarray= (RANDI *) malloc( (int)atshed.par * sizeof(RANDI));
    for(z=0; z<(int)atshed.par; ++z) {
      rfreq=(ats_sound->frq[z][tdata[0].from] < 500.? 50. : ats_sound->frq[z][tdata[0].from] * bw);
      randi_setup(sparams.sr,rfreq,&rarray[z]);
      dospt[z]=0.;
    }
    break;  

  }
  //NOW DO IT...
  written=0;
  stopper=FALSE;

  for(i = 0; i < nbp - 1; i++) { 

    curr=tdata[i].from;
 
    for(j=0; j < tdata[i].size;   j++) {

      next=(tdata[i].from < tdata[i].to ? curr+1 : curr-1 );	        
      if(next < 0 || next >= (int)atshed.fra) break;

      dt=fabs(ats_sound->time[0][next] - ats_sound->time[0][curr]);
      frame_samps=dt * sparams.sr * tdata[i].tfac ;      

      switch (sflag) {
      case SYNTH_DET: { //deterministic synthesis only
	for(x = 0; x < (int)atshed.par; x++) {                       
	  synth_deterministic_only(ats_sound->amp[x][curr], 
				   ats_sound->amp[x][next], 
				   ats_sound->frq[x][curr] * sparams.freq,
				   ats_sound->frq[x][next] * sparams.freq, 
				   frame_samps,x, dospt);     	  	         
	}
	break;
      }	
      case SYNTH_RES: { //residual synthesis only
	for(x = 0; x < (int)ATSA_CRITICAL_BANDS; x++) {
	  synth_residual_only(ENG_RMS(ats_sound->band_energy[x][curr], atshed.ws), 
			      ENG_RMS(ats_sound->band_energy[x][next],atshed.ws) ,
			      res_band_centers[x],frame_samps,x,rospt,&rarray[x]);  
	}
	break;
      }
      case SYNTH_BOTH: { //residual and deterministic synthesis  
	for(x = 0; x < (int)atshed.par; x++) {
	  rfreq=(ats_sound->frq[x][curr] < 500.? 50. : ats_sound->frq[x][curr]* bw);
	  synth_both(ats_sound->amp[x][curr], 
		     ats_sound->amp[x][next], 
		     ats_sound->frq[x][curr] * sparams.freq,
		     ats_sound->frq[x][next] * sparams.freq, 
		     frame_samps,x, dospt,
		     ENG_RMS(ats_sound->res[x][curr] * sparams.ramp, atshed.ws), 
		     ENG_RMS(ats_sound->res[x][next] * sparams.ramp, atshed.ws),
		     &rarray[x]);     	  	         
	}
	break;
	
      }
      }//end switch
      
      for(z=0; z< maxlen; ++z) {      //write and clean output buffer     
	if(z < (int)frame_samps) {      
	  
	  obuf[0][z] = MUS_FLOAT_TO_SAMPLE(frbuf[z]);	  
	  written++;
	  if (fabs(frbuf[z]) >= maxamp) {maxamp=fabs(frbuf[z]);}
	}
	frbuf[z]=0.;   
      }
      mus_sound_write(ptout, 0, frame_samps-1, 1, obuf);
      if(stopper==TRUE) goto finish;
      ++nValue;
      UpdateProgress(nValue,todo);
      curr=(tdata[i].from < tdata[i].to ? curr+1 :curr-1  );
    }
    
  } //CHANGE BREAKPOINT
  
 finish:

  free(frbuf);
  
  switch (sflag) {
  case SYNTH_DET:
    free(dospt);
    break;
  case SYNTH_RES: 
    free(rospt);
    free(rarray);
    break;
  case SYNTH_BOTH: 
    free(dospt);
    free(rarray);
    break;
  }
  
  mus_sound_close_output(ptout,written * mus_data_format_to_bytes_per_sample(format));

//   *info=0;
//   strcat(info, "DONE OK...!!! MAXAMP= ");
//   sprintf(stamp,"%6.4f ",maxamp);
//   strcat(info, stamp);
//   Popup(info);

  free(obuf[0]);
  free(tdata);
  EndProgress();

  strcpy(str, "DONE! MAXAMP= ");
  sprintf(stamp, "%6.4f", maxamp);
  strcat(str, stamp);
  Popup(str);

}

int locate_frame(int from_frame, float time, float dif)
{
//Assuming that the duration of each frame may be different, we
//do not have any other method to locate the frame for a given time

  int i=from_frame, frame=0;
  
  //These two cases are obvious
  if(time <= ats_sound->time[0][1]) return(0);
  if(time >= ats_sound->time[0][(int)atshed.fra-1]) return((int)atshed.fra-1);

  //not obvious cases...
  do{
    if(dif >= 0.) {
      if(ats_sound->time[0][i] >= time) {
	frame=i;
	break;
      }
      ++i;
    } else {
      if(ats_sound->time[0][i] <= time) {
	frame=i;
	break;
      }
      --i;
    }
    
  } while(i > 0);
  
  if(frame < 0) frame=0;
  if(frame > (int)atshed.fra-1) frame=(int)atshed.fra-1;

  return(frame);
}

void set_output_type(int *format, int *header)
{
  switch(outype) {
  case WAV16:
    *format=MUS_LSHORT;
    *header=MUS_RIFF;  
    break;
  case WAV32:
    *format=MUS_LFLOAT;
    *header=MUS_RIFF;  
    break;
  case AIF16:
    *format=MUS_BSHORT;
    *header=MUS_AIFC;  
    break;
  case AIF32:
    *format=MUS_BFLOAT;
    *header=MUS_AIFC;  
  case SND16:
    *format=MUS_BSHORT;
    *header=MUS_NEXT; 
    break;
  case SND32: 
   *format=MUS_BFLOAT;
   *header=MUS_NEXT; 
   break; 
  }
}

