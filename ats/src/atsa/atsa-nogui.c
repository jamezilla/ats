/* atsa-nogui.c
 * atsa: ATS analysis implementation
 * Oscar Pablo Di Liscia / Pete Moss / Juan Pampin 
 */

#include "atsa.h"

void usage(void)
{
  fprintf(stderr, "ATSA ");
  fprintf(stderr, VERSION);
  fprintf(stderr, "\natsa soundfile atsfile [flags]\n");
  fprintf(stderr, "Flags:\n");
  fprintf(stderr, "\t -b start (%f seconds)\n"           \
          "\t -e duration (%f seconds or end)\n"         \
          "\t -l lowest frequency (%f Hertz)\n"          \
          "\t -H highest frequency (%f Hertz)\n"         \
          "\t -d frequency deviation (%f of partial freq.)\n"    \
          "\t -c window cycles (%d cycles)\n"                           \
          "\t -w window type (type: %d)\n"                              \
          "\t\t(Options: 0=BLACKMAN, 1=BLACKMAN_H, 2=HAMMING, 3=VONHANN)\n" \
          "\t -h hop size (%f of window size)\n"                        \
          "\t -m lowest magnitude (%f)\n"                               \
          "\t -t track length (%d frames)\n"                            \
          "\t -s min. segment length (%d frames)\n"                     \
          "\t -g min. gap length (%d frames)\n"                         \
          "\t -T SMR threshold (%f dB SPL)\n"                           \
          "\t -S min. segment SMR (%f dB SPL)\n"                        \
          "\t -P last peak contribution (%f of last peak's parameters)\n" \
          "\t -M SMR contribution (%f)\n"                               \
          "\t -F File Type (type: %d)\n"                                \
          "\t\t(Options: 1=amp.and freq. only, 2=amp.,freq. and phase, 3=amp.,freq. and residual, 4=amp.,freq.,phase, and residual)\n\n", 
	  ATSA_START, 
	  ATSA_DUR,
	  ATSA_LFREQ, 
	  ATSA_HFREQ, 
	  ATSA_FREQDEV, 
	  ATSA_WCYCLES, 
	  ATSA_WTYPE, 
	  ATSA_HSIZE, 
	  ATSA_LMAG,
	  ATSA_TRKLEN, 
	  ATSA_MSEGLEN, 
	  ATSA_MGAPLEN, 
	  ATSA_SMRTHRES, 
	  ATSA_MSEGSMR, 
	  ATSA_LPKCONT, 
	  ATSA_SMRCONT,
	  ATSA_TYPE);
  exit(1);
}


int main(int argc, char **argv)
{
  int i, val;
  ANARGS *anargs;
  char *soundfile, *ats_outfile;

  if(argc < 3 || argv[1][0] == '-' || argv[2][0] == '-') usage();

  anargs=(ANARGS*)malloc(sizeof(ANARGS));
  
  /* default values for analysis args */
  anargs->start = ATSA_START; 
  anargs->duration = ATSA_DUR;
  anargs->lowest_freq = ATSA_LFREQ; 
  anargs->highest_freq = ATSA_HFREQ; 
  anargs->freq_dev = ATSA_FREQDEV; 
  anargs->win_cycles = ATSA_WCYCLES; 
  anargs->win_type = ATSA_WTYPE; 
  anargs->hop_size = ATSA_HSIZE; 
  anargs->lowest_mag = ATSA_LMAG;
  anargs->track_len = ATSA_TRKLEN; 
  anargs->min_seg_len = ATSA_MSEGLEN; 
  anargs->min_gap_len = ATSA_MGAPLEN; 
  anargs->SMR_thres = ATSA_SMRTHRES; 
  anargs->min_seg_SMR = ATSA_MSEGSMR; 
  anargs->last_peak_cont = ATSA_LPKCONT; 
  anargs->SMR_cont = ATSA_SMRCONT ; 
  anargs->type = ATSA_TYPE; 

  soundfile = argv[1];
  ats_outfile = argv[2];

  for(i=3; i<argc; ++i) {
    switch(argv[i][0]) {
    case '-' :
      {switch(argv[i][1]) {
        case 'b' : sscanf(argv[i]+2, "%f\n", &(anargs->start)); break;
        case 'e' : sscanf(argv[i]+2, "%f\n", &(anargs->duration)); break;
        case 'l' : sscanf(argv[i]+2, "%f\n", &(anargs->lowest_freq)); break;
        case 'H' : sscanf(argv[i]+2, "%f\n", &(anargs->highest_freq)); break;
        case 'd' : sscanf(argv[i]+2, "%f\n", &(anargs->freq_dev)); break;
        case 'c' : sscanf(argv[i]+2, "%d\n", &(anargs->win_cycles)); break;
        case 'w' : sscanf(argv[i]+2, "%d\n", &(anargs->win_type)); break;
        case 'h' : sscanf(argv[i]+2, "%f\n", &(anargs->hop_size)); break;
        case 'm' : sscanf(argv[i]+2, "%f\n", &(anargs->lowest_mag)); break;
        case 't' : sscanf(argv[i]+2, "%d\n", &(anargs->track_len)); break;
        case 's' : sscanf(argv[i]+2, "%d\n", &(anargs->min_seg_len)); break;
        case 'g' : sscanf(argv[i]+2, "%d\n", &(anargs->min_gap_len)); break;
        case 'T' : sscanf(argv[i]+2, "%fL\n", &(anargs->SMR_thres)); break;
        case 'S' : sscanf(argv[i]+2, "%f\n", &(anargs->min_seg_SMR)); break;
        case 'P' : sscanf(argv[i]+2, "%f\n", &(anargs->last_peak_cont)); break;
        case 'M' : sscanf(argv[i]+2, "%f\n", &(anargs->SMR_cont)); break;
        case 'F' : sscanf(argv[i]+2, "%d\n", &(anargs->type)); break;
        default  : usage();
        }
        break;
      }
    default : usage();
    }
  }
  val = main_anal(soundfile, ats_outfile, anargs, ATSA_RES_FILE);
  free(anargs);
  return(val);
}
