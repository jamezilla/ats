/* atsread by Alex Norman
 * read the README file for info about the opcode
 * tabstop = 3
*/

#include "m_pd.h"
#include <stdio.h>
#include <math.h>

#define ATSA_NOISE_VARIANCE 0.04

typedef struct _atsdataloc
{
	double amp;
	double freq;
}	t_atsdataloc;

typedef struct _atshead
{
	double  magic;  /* ats magic number */
	double  sampr;  /* sampling rate */
	double  frmsz;  /* frame size in samples */
	double  winsz;  /* window size in samples */
	double  npartials;      /* number of partials */
	double  nfrms;  /* number of frames */
	double  ampmax; /* max amplitude */
	double  freqmax;        // max frequency
	double  dur;    /* duration seconds */
	double  type;   /* Ats Frame type 1-4 */
}	t_atshead;

static t_class *atsread_class;

typedef struct _atsread {
	t_object  x_obj;
  	t_float timepnt;
  	int berrflg;	//time pointer bounderror flag
  	t_atsdataloc ** data;
  	double ** nzdata;
  	t_atshead atshead;
  	double framemult;
  	int * outpartials;	//partials to output
	int outnzbands[25];
	int outnz;	//should we output the noise
	int outsines;	//should we output the sines?
  	
	t_atom * amplist;
  	t_atom * freqlist;
	t_atom nzlist[25];
	
  	t_outlet *freq_out, *amp_out, *nz_out;
} t_atsread;


#ifdef BYTESWAP
//byte swaps a double
double bswap(double * swap_me){
	unsigned char *sw_in, sw_out[8];	//for swaping
	sw_in = (unsigned char *)swap_me;
   sw_out[0] = sw_in[7];
   sw_out[1] = sw_in[6];
   sw_out[2] = sw_in[5];
   sw_out[3] = sw_in[4];
   sw_out[4] = sw_in[3];
   sw_out[5] = sw_in[2];
   sw_out[6] = sw_in[1];
   sw_out[7] = sw_in[0];
	return *((double *)sw_out);
}
#endif

//function prototype
void readdatafile(t_atsread *, const char *);

void atsread_timepnt(t_atsread *x, t_floatarg time){
	double mytime = (double)time;
	int i, frame, outp, outpnz;	//outp is the number of partials to output
	double frac, tempamp, tempfreq, tempnz;
	t_atom *amplist, *freqlist, *nzlist;
	amplist = x->amplist;
	freqlist = x->freqlist;
	nzlist = x->nzlist;

	if(x->atshead.magic != 123){
		post("ATSREAD: you must first open an Atsfile");
		return;
	}
	outp = 0;	//set the output partial count to zero
	outpnz = 0;	//set the output noiseband count to zero
	
	frame = (int)floor(mytime * x->framemult);
	
	if(frame < 0){
		if(x->berrflg){
			post("ATSREAD: negative time pointers now allowed, setting to zero");
			x->berrflg = 0;
		}
		if(x->outsines == 1){
			for(i = 0; i < x->atshead.npartials; i++){
				if(x->outpartials[i] == 1){
					tempamp = (x->data[0][i]).amp;
					tempfreq = (x->data[0][i]).freq;
					SETFLOAT(&(amplist[outp]),(float)tempamp);
					SETFLOAT(&(freqlist[outp]),(float)tempfreq);
					outp++;
				}
			}
		}
		if(x->outnz == 1 && (int)x->atshead.type != 1 && (int)x->atshead.type != 2){
			//we can and should output noise
			for(i = 0; i < 25; i++){
				if(x->outnzbands[i] == 1){
					tempnz = (x->nzdata[0][i]);
					tempnz = sqrt((tempnz / ((x->atshead.winsz) * (float)ATSA_NOISE_VARIANCE) ) );
					SETFLOAT(&(nzlist[outpnz]),(float)tempnz);
					outpnz++;
				}
			}
		}
	}
	else if(frame >= ((int)(x->atshead.nfrms) - 1)){
		frame = (int)(x->atshead.nfrms) - 1;
		if(x->berrflg){
			post("ATSREAD: timepointer exceeds last frame, setting to last frame");
			x->berrflg = 0;
		}
		if(x->outsines == 1){
			for(i = 0; i < x->atshead.npartials; i++){
				if(x->outpartials[i] == 1){
					tempamp = (x->data[frame][i]).amp;
					tempfreq = (x->data[frame][i]).freq;
					SETFLOAT(&(amplist[outp]),(float)tempamp);
					SETFLOAT(&(freqlist[outp]),(float)tempfreq);
					outp++;
				}
    	}
		}
		if(x->outnz == 1 && (int)x->atshead.type != 1 && (int)x->atshead.type != 2){
			//we can and should output noise
			for(i = 0; i < 25; i++){
				if(x->outnzbands[i] == 1){
					tempnz = (x->nzdata[frame][i]);
					tempnz = sqrt((tempnz / ((x->atshead.winsz) * (float)ATSA_NOISE_VARIANCE) ) );
					SETFLOAT(&(nzlist[outpnz]),(float)tempnz);
					outpnz++;
				}
			}
		}
	}
	else{
		x->berrflg = 1;
	                
		frac = (mytime - ((double)(frame))/x->framemult) / (((double)(frame + 1))/x->framemult - ((double)(frame))/x->framemult);
	
		if(x->outsines == 1){
			for(i = 0; i < x->atshead.npartials; i++){
				if(x->outpartials[i] == 1){
					tempamp = (x->data[frame][i]).amp + frac * ((x->data[1 + frame][i]).amp - (x->data[frame][i]).amp);
					tempfreq = (x->data[frame][i]).freq + frac * ((x->data[1 + frame][i]).freq - (x->data[frame][i]).freq);
					SETFLOAT(&(amplist[outp]),(float)tempamp);
					SETFLOAT(&(freqlist[outp]),(float)tempfreq);
					outp++;
				}
			}
		}
		if(x->outnz == 1 && (int)x->atshead.type != 1 && (int)x->atshead.type != 2){
			//we can and should output noise
			for(i = 0; i < 25; i++){
				if(x->outnzbands[i] == 1){
					tempnz = (x->nzdata[frame][i]) + frac * ((x->nzdata[frame + 1][i]) - (x->nzdata[frame][i])) ;
					tempnz = sqrt((tempnz / ((x->atshead.winsz) * (float)ATSA_NOISE_VARIANCE) ) );
					SETFLOAT(&(nzlist[outpnz]),(float)tempnz);
					outpnz++;
				}
			}
		}
	}
	//output the data
	if(x->outsines == 1){
		outlet_list(x->freq_out, gensym("list"), outp, freqlist);
		outlet_list(x->amp_out, gensym("list"), outp, amplist);
	}
	if(x->outnz == 1)
		outlet_list(x->nz_out, gensym("list"), outpnz, nzlist);
}

void atsread_any(t_atsread *x, t_symbol *s, int argc, t_atom *argv){
	char * filename;
	int i, j, from, to;
	t_symbol * temp;

	if(strcmp(s->s_name, "open") == 0){
		//open data file
		if(argc < 1){
			post("ATSREAD: you need to specify a file name");
			return;
		}
		
		temp = atom_getsymbol(&argv[0]);
		
		if(temp == NULL){
			post("ATSREAD: not a valid filename");
			return;
		}
		readdatafile(x, temp->s_name);
		x->berrflg = 1;
		return;
	}
	if(strcmp(s->s_name, "noise") == 0){
		x->outnz = 1;
		post("ATSREAD: outputing noise, if there is any");
		return;
	}
	if(strcmp(s->s_name, "nonoise") == 0){
		x->outnz = 0;
		post("ATSREAD: not outputing noise");
		return;
	}
	if(strcmp(s->s_name, "sines") == 0){
		x->outsines = 1;
		post("ATSREAD: outputing sines");
		return;
	}
	if(strcmp(s->s_name, "nosines") == 0){
		x->outsines = 0;
		post("ATSREAD: not outputing sines");
		return;
	}
	
	/* BELOW HERE WE DEAL WITH PARTIAL and BAND ADDING/SETTING/REMOVING */
	
	if(x->atshead.magic != 123){
		post("ATSREAD: you must open a file before setting up the partials");
		return;
	}
	if(argc < 1){
		post("ATSREAD: you need to specify at least 1 partial/band to set/add/remove");
		return;
	}
	
	if(strcmp(s->s_name, "set") == 0 || strcmp(s->s_name, "add") == 0){
		//set or add partials to output
		if(strcmp(s->s_name, "set") == 0){
			//clear the data table
			for(i = 0; i < (int)x->atshead.npartials; i++)
				x->outpartials[i] = 0;
		}
		
		for(i = 0; i < argc; i++){
			temp = atom_getsymbol(&argv[i]);
			if(strcmp(temp->s_name, "float") == 0){
				j = (int)atom_getfloat(&argv[i]);
				if(j < 1)
					j = 1;
				else if(j >= (int)x->atshead.npartials)
					j = (int)x->atshead.npartials;
				x->outpartials[j - 1] = 1;
			}
			else{
				if(sscanf(temp->s_name, "%i..%i", &from, &to) == 2){
					if(from > to){
						j = from;
						from = to;
						to = j;
					}
					if(from < 1)
						from = 1;
					if(to > (int)x->atshead.npartials)
						to = (int)x->atshead.npartials;
					for(j = from; j <= to; j++)
						x->outpartials[j-1] = 1;
				}
				else
					post("ATSREAD: %s is not a valid range (ie 1..42) or partial number (ie 12)", temp->s_name);
			}
		}
		return;
	}
	if(strcmp(s->s_name, "remove") == 0){
		//remove partials
		for(i = 0; i < argc; i++){
			temp = atom_getsymbol(&argv[i]);
			if(strcmp(temp->s_name, "float") == 0){
				j = (int)atom_getfloat(&argv[i]);
				if(j < 1)
					j = 1;
				else if(j >= (int)x->atshead.npartials)
					j = (int)x->atshead.npartials;
				x->outpartials[j - 1] = 0;
			}
			else{
				if(sscanf(temp->s_name, "%i..%i", &from, &to) == 2){
					if(from > to){
						j = from;
						from = to;
						to = j;
					}
					if(from < 1)
						from = 1;
					if(to > (int)x->atshead.npartials)
						to = (int)x->atshead.npartials;
					for(j = from; j <= to; j++)
						x->outpartials[j-1] = 0;
				}
				else
					post("ATSREAD: %s is not a valid range (ie 1..42) or partial number (ie 12)", temp->s_name);
			}
		}
		return;
	}
	if(strcmp(s->s_name, "setnz") == 0 || strcmp(s->s_name, "addnz") == 0){
		//set or add bands to output
		if(strcmp(s->s_name, "setnz") == 0){
			//clear the data table
			for(i = 0; i < 25; i++)
				x->outnzbands[i] = 0;
		}
		
		for(i = 0; i < argc; i++){
			temp = atom_getsymbol(&argv[i]);
			if(strcmp(temp->s_name, "float") == 0){
				j = (int)atom_getfloat(&argv[i]);
				if(j < 1)
					j = 1;
				else if(j >= 25)
					j = 25;
				x->outnzbands[j - 1] = 1;
			}
			else{
				if(sscanf(temp->s_name, "%i..%i", &from, &to) == 2){
					if(from > to){
						j = from;
						from = to;
						to = j;
					}
					if(from < 1)
						from = 1;
					if(to > 25)
						to = 25;
					for(j = from; j <= to; j++)
						x->outnzbands[j-1] = 1;
				}
				else
					post("ATSREAD: %s is not a valid range (ie 1..25) or band number (ie 2)", temp->s_name);
			}
		}
		return;
	}
	if(strcmp(s->s_name, "removenz") == 0){
		//remove bands
		for(i = 0; i < argc; i++){
			temp = atom_getsymbol(&argv[i]);
			if(strcmp(temp->s_name, "float") == 0){
				j = (int)atom_getfloat(&argv[i]);
				if(j < 1)
					j = 1;
				else if(j >= 25)
					j = 25;
				x->outnzbands[j - 1] = 0;
			}
			else{
				if(sscanf(temp->s_name, "%i..%i", &from, &to) == 2){
					if(from > to){
						j = from;
						from = to;
						to = j;
					}
					if(from < 1)
						from = 1;
					if(to > 25)
						to = 25;
					for(j = from; j <= to; j++)
						x->outnzbands[j-1] = 0;
				}
				else
					post("ATSREAD: %s is not a valid range (ie 1..25) or band number (ie 12)", temp->s_name);
			}
		}
		return;
	}
}


void *atsread_new(t_symbol *s, int argc, t_atom *argv){
	t_symbol * temp;
	int i;
	
	t_atsread *x = (t_atsread *)pd_new(atsread_class);
  	
	// initalize
	x->berrflg = 1;
	x->data = NULL;
	x->nzdata = NULL;
	x->atshead.magic = 0;
	x->outpartials = NULL;
	x->amplist = NULL;
	x->freqlist = NULL;
	x->outnz = 1;
	x->outsines = 1;
	
	//outlets
	x->freq_out = outlet_new(&x->x_obj, &s_float);
	x->amp_out = outlet_new(&x->x_obj, &s_float);
	x->nz_out = outlet_new(&x->x_obj, &s_float);

	/* deal with creation time arguments */
	for(i = 0; i < argc; i++){
		temp = atom_getsymbol(&argv[i]);
		if(strcmp(temp->s_name, "noise") == 0){
			x->outnz = 1;
			post("ATSREAD: outputing noise, if there is any");
		}
		else if(strcmp(temp->s_name, "nonoise") == 0){
			x->outnz = 0;
			post("ATSREAD: not outputing noise");
		}
		else if(strcmp(temp->s_name, "sines") == 0){
			x->outsines = 1;
			post("ATSREAD: outputing sines");
		}
		else if(strcmp(temp->s_name, "nosines") == 0){
			x->outsines = 0;
			post("ATSREAD: not outputing sines");
		}
		else{
			//open data file
			if(temp == NULL){
				post("ATSREAD: not a valid filename");
			}
			else
				readdatafile(x, temp->s_name);
		}
	}
	return (void *)x;
}
void atsread_destroy(t_atsread *x){
	int i;
	//clean up allocated data
	if(x->data != NULL){
		for(i = 0; i < (int)(x->atshead).nfrms; i++)
			free(x->data[i]);
		free(x->data);
		x->data = NULL;
	}
	if(x->nzdata != NULL){
		for(i = 0; i < (int)(x->atshead).nfrms; i++)
			free(x->nzdata[i]);
		free(x->nzdata);
		x->nzdata = NULL;
	}
	if(x->outpartials != NULL)
		free(x->outpartials);
	if(x->amplist != NULL)
		free(x->amplist);
	if(x->freqlist != NULL)
		free(x->freqlist);
}

void atsread_setup(void) {
	atsread_class = class_new(gensym("atsread"),
		(t_newmethod)atsread_new, (t_method)atsread_destroy, sizeof(t_atsread), CLASS_DEFAULT, A_GIMME, 0);
	
	class_addfloat(atsread_class, atsread_timepnt);
  	class_addanything(atsread_class, atsread_any);
  	class_sethelpsymbol(atsread_class, gensym("help-atsread"));
}



        // set up data file to be read
void readdatafile(t_atsread *x, const char * filename){
	FILE * fin;
	double temp;
	int i,j;
#ifdef BYTESWAP
	int swapped = 0;
	int k;
#endif
	
	if((fin=fopen(filename,"rb"))==0){
		post("ATSREAD: cannot open file %s", filename);
		return;
	}
	//delete any previous data
	if(x->data != NULL){
		for(i = 0; i < (int)(x->atshead).nfrms; i++)
			free(x->data[i]);
		free(x->data);
		x->data = NULL;
	}
	if(x->nzdata != NULL){
		for(i = 0; i < (int)(x->atshead).nfrms; i++)
			free(x->nzdata[i]);
		free(x->nzdata);
		x->nzdata = NULL;
	}
         
	// read the header
	fread(&(x->atshead),sizeof(t_atshead),1,fin);

	//make sure this is an ats file
	if((int)x->atshead.magic != 123){

#ifndef BYTESWAP
		post("ATSREAD - Magic Number not correct in %s", filename);
		fclose(fin);
		return;
#else
		//attempt to byte swap
		if((int)bswap(&x->atshead.magic) != 123){	//not an ats file
			post("ATSREAD - Magic Number not correct in %s", filename);
			fclose(fin);
			return;
		} else {
			post("ATSREAD - %s is of the wrong endianness, byteswapping.", filename);
			swapped = 1;	//true (for later use)
			//byte swap
			for(i = 0; i < (sizeof(t_atshead) / sizeof(double)); i++)
				((double *)(&x->atshead))[i]  = bswap(&(((double *)(&x->atshead))[i]));
		}
#endif
	}
	post("\t %s stats: \n \t\tType: %i, Frames: %i, Partials: %i, Sampling rate: %i Hz, Duration %f sec, Max Freq: %f Hz, Max Amp: %f",
		filename, (int)x->atshead.type, (int)x->atshead.nfrms, (int)x->atshead.npartials, (int)x->atshead.sampr, (float)x->atshead.dur, 
		(float)x->atshead.freqmax, (float)x->atshead.ampmax);
	
	if((int)x->atshead.type > 4 || (int)x->atshead.type < 1){
		post("ATSREAD - %i type Ats files not supported yet",(int)x->atshead.type);
		x->atshead.magic = 0;
		fclose(fin);
		return;
	}
        
	//allocate memory for a new data table
	x->data = (t_atsdataloc **)malloc(sizeof(t_atsdataloc *) * (int)x->atshead.nfrms);
	for(i = 0; i < (int)x->atshead.nfrms; i++)
		x->data[i] = (t_atsdataloc *)malloc(sizeof(t_atsdataloc) * (int)x->atshead.npartials);
                
	// store the data into table(s)
        
	switch ((int)x->atshead.type){
		case 1 :
			for(i = 0; i < (int)x->atshead.nfrms; i++){
				//eat time data
				fread(&temp,sizeof(double),1,fin);
				//get a whole frame of partial data
				fread(&x->data[i][0], sizeof(t_atsdataloc),(int)x->atshead.npartials,fin);
#ifdef BYTESWAP
				if(swapped) {	//byte swap if nessisary
					for(k = 0; k < (int)x->atshead.npartials; k++){
						x->data[i][k].amp  = bswap(&x->data[i][k].amp);
						x->data[i][k].freq  = bswap(&x->data[i][k].freq);
					}
				}
#endif
			}
			break;
		case 2 :
			for(i = 0; i < (int)x->atshead.nfrms; i++){
				//eat time data
				fread(&temp,sizeof(double),1,fin);
				//get partial data
				for(j = 0; j < (int)x->atshead.npartials; j++){
					fread(&x->data[i][j], sizeof(t_atsdataloc),1,fin);
					//eat phase info
					fread(&temp, sizeof(double),1,fin);
#ifdef BYTESWAP
					if(swapped) {	//byte swap if nessisary
						x->data[i][j].amp = bswap(&x->data[i][j].amp);
						x->data[i][j].freq = bswap(&x->data[i][j].freq);
					}
#endif
				}
			}
			break;
		case 3 :
			//allocate mem for the noise data
			x->nzdata = (double **)malloc(sizeof(double *) * (int)x->atshead.nfrms);
			for(i = 0; i < (int)x->atshead.nfrms; i++){
				//allocate this row
				x->nzdata[i] = (double *)malloc(sizeof(double) * 25);
				//eat time data
				fread(&temp,sizeof(double),1,fin);
				//get a whole frame of partial data
				fread(&x->data[i][0], sizeof(t_atsdataloc),(int)x->atshead.npartials,fin);
				//get the noise
				fread(&x->nzdata[i][0], sizeof(double),25,fin);
#ifdef BYTESWAP
					if(swapped) {	//byte swap if nessisary
						for(k = 0; k < (int)x->atshead.npartials; k++){
							x->data[i][k].freq = bswap(&x->data[i][k].freq);
							x->data[i][k].amp = bswap(&x->data[i][k].amp);
						}
						for(k = 0; k < 25; k++)
							x->nzdata[i][k] = bswap(&x->nzdata[i][k]);
					}
#endif
			}
			break;
		case 4 :
			//allocate mem for the noise data
			x->nzdata = (double **)malloc(sizeof(double *) * (int)x->atshead.nfrms);
			for(i = 0; i < (int)x->atshead.nfrms; i++){
				//allocate noise for this row
				x->nzdata[i] = (double *)malloc(sizeof(double) * 25);
				//eat time data
				fread(&temp,sizeof(double),1,fin);
				//get partial data
				for(j = 0; j < (int)x->atshead.npartials; j++){
					fread(&x->data[i][j], sizeof(t_atsdataloc),1,fin);
					//eat phase info
					fread(&temp, sizeof(double),1,fin);
#ifdef BYTESWAP
					if(swapped) {	//byte swap if nessisary
						x->data[i][j].freq = bswap(&x->data[i][j].freq);
						x->data[i][j].amp = bswap(&x->data[i][j].amp);
					}
#endif
				}
				//get the noise
				fread(&x->nzdata[i][0], sizeof(double),25,fin);
#ifdef BYTESWAP
				if(swapped) {	//byte swap if nessisary
					for(k = 0; k < 25; k++)
						x->nzdata[i][k] = bswap(&x->nzdata[i][k]);
				}
#endif
			}
			break;
	}

	// set up the frame multiplyer
	x->framemult = (x->atshead.nfrms - (double)1)/ x->atshead.dur;

	if(x->outpartials != NULL)
		free(x->outpartials);
	x->outpartials = (int *)malloc(sizeof(int) * (int)(x->atshead.npartials));
	for(i = 0; i < (int)x->atshead.npartials; i++)
		x->outpartials[i] = 1;
	for(i = 0; i < 25; i++)
		x->outnzbands[i] = 1;

	if(x->amplist != NULL)
		free(x->amplist);
	if(x->freqlist != NULL)
		free(x->freqlist);
	x->amplist = (t_atom *)malloc(sizeof(t_atom) * (int)(x->atshead.npartials));
	x->freqlist = (t_atom *)malloc(sizeof(t_atom) * (int)(x->atshead.npartials));

	//close the input file
	fclose(fin);
	return;
}
