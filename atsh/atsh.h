/*
ATSH.H
Oscar Pablo Di Liscia / Juan Pampin
*/

#if defined(WINDOZE)
 #include "stdlib.h"	
 #include "math.h"
 #define UNDO_FILE "\\atsh_undo.dat"
 #define DEFAULT_OUTPUT_FILE "\\atsh_out.wav"
 #define DEFAULT_OUTPUT_ATS_FILE "\\ats_temp_analysis.ats"
 #define DEFAULT_OUTPUT_RES_FILE "\\atsa_res.wav"
#endif


#if defined(LINUX)
  #include <string.h>	
  #include <errno.h>
  #include <stdlib.h>
  #include <math.h>
  #define UNDO_FILE "/tmp/atsh_undo.dat"
  #define DEFAULT_OUTPUT_FILE "/tmp/atsh_out.wav"
  #define DEFAULT_OUTPUT_ATS_FILE "/tmp/ats_temp_analysis.ats"
  #define DEFAULT_OUTPUT_RES_FILE "/tmp/atsa_res.wav"
#endif


//THESE FIVE ARE COMMON
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "atsa.h"

#define IN_NAME  0
#define OUT_NAME 1
#define SPA_NAME 2
#define TABLE_LENGTH 16384
#define CURVE_LENGTH 100.

#define MAX_COLOR_VALUE 65535 //this is the max value the color can hold (unsigned short)

#define GRAPH_W 800 
#define GRAPH_H 400 

#define NULL_VIEW  0
#define RES_VIEW   1
#define SON_VIEW   2
#define SEL_ONLY   3

#define ATS_INSEL  0
#define ATS_OUTSEL 1
#define SND_INSEL  2
#define SND_OUTSEL 3
#define APF_INSEL  4
#define APF_OUTSEL 5
#define RES_OUTSEL 6


#define VEC_LEN 1024  //curve vector size

//output audio file formats
#define WAV16  0
#define WAV32  1
#define AIF16  2
#define SND16  3

/*these two not yet supported*/
#define AIF32  4
#define SND32  5

#define AMP_EDIT  0  //kinf of edition (for undo)
#define FRE_EDIT  1
#define TIM_EDIT  2

#define MIN_FLOAT (float)1. / (float)pow(2.,31.)

#define NUNDO  100 //max. number of UNDO allowed 

#define MAT_ALLOC(rows,cols,pointer,data,index)\
              pointer= (data**) malloc (rows * sizeof(*data));\
              for(index=0; index < rows; ++index) {\
                    pointer[index]= (*data) malloc(cols*sizeof(data));}

#define SWAP_INT(a,b)\
              temp=a; a=b; b=temp;

#define FILE_HAS_PHASE atshed->typ==2. || atshed->typ==4. 
#define FILE_HAS_NOISE atshed->typ==3. || atshed->typ==4.
#define FILE_WITHOUT_NOISE atshed->typ==1. || atshed->typ==2. 
#define NOTHING_SELECTED   vertex1==FALSE && vertex2==FALSE
#define STARTING_SELECTION vertex1==TRUE && vertex2==FALSE
#define SOMETHING_SELECTED vertex1==FALSE && vertex2==TRUE
#define VIEWING_RES   view_type==RES_VIEW
#define VIEWING_DET   view_type==SON_VIEW || view_type==SEL_ONLY

#define SYNTH_RES  1
#define SYNTH_DET  2
#define SYNTH_BOTH 3

//MACROS
/*
;;; In these macros pha_1 and frq_1 refers to the instantaneous phase
;;; and frequency of the previous frame while pha and frq refer
;;; to the phase and frequency of the present frame.
*/
#define COMPUTE_M(pha_1, frq_1, pha, frq, dt) ((pha_1 + (frq_1 * dt) - pha) + ((frq - frq_1) * .5 * dt)) / TWOPI   
#define COMPUTE_AUX(pha_1, pha, frq_1, dt, M) (pha + (TWOPI * M)) - (pha_1 + (frq_1 * dt))
#define COMPUTE_ALPHA(aux, frq_1, frq, dt) ((3. / (dt * dt)) * aux ) - ((frq - frq_1) / dt)
#define COMPUTE_BETA(aux, frq_1, frq, dt) ((-2. / (dt * dt * dt)) * aux) + ((frq - frq_1) / (dt * dt))
/*
;;; note that for this macro the values of i should go
;;; from 0 to dt. So in the case of having 220 samples
;;; within a frame the increment for i will be dt/200
*/
#define INTERP_PHASE(pha_1, frq_1, alpha, beta, i) (beta * i * i * i) + (alpha * i * i)+ (frq_1 * i) + pha_1

typedef struct { //parameters for resynthesis

  float amp;  //Deterministic amplitude scalar
  float ramp; //Residual amplitude scalar
  float frec; //Global frequency scalar
  float max_stretch; //MAX TIME SCALAR
  float beg;  //Begin synthesis time
  float end;  //End synthesis time
  float sr;   //Sampling Rate
  short allorsel; //when TRUE, use only selected partials, when FALSE use all
  short upha;     //when TRUE, the fase information (if any) is used, when FALSE is not
}SPARAMS;

typedef struct {

  int viewstart;
  int viewend;
  int diff;
  int step;
  int prev;

} VIEW_PAR;

typedef struct {
  
  int from; //from frame of selection
  int to;   //to frame of slection
  int f1;   //low frequency bound  of selection (Hz)
  int f2;   //high frequency bound  of selection (Hz) 
  int x1;   //x left superior vertex position ABSOLUTE VALUES OF MOUSE POINTER
  int x2;   //y left superior vertex positio
  int y1;   //x right inferior vertex position
  int y2;   //y right inferior vertex position
  int width; //the absolute width of the view in the moment of the selection
} SELECTION;

typedef struct {
  
  int from; //from frame of selection
  int to;   //to frame of slection
  int f1;   //low frequency bound  of selection (Hz)
  int f2;   //high frequency bound  of selection (Hz) 
  int nsel; //number of partials selected
  int ed_kind; //the kind of edition done (either frequency or amplitude at present)

} UNDO_DATA;



typedef struct {

  GtkWidget *progressbar;
  GtkWidget *window;
  int bProgressUp;
  int nLastPct;

}typProgressData;


typedef struct { //the envelope window data

  GtkWidget *curve;         //curve storing the envelope
  GtkWidget *whatodo;       //either scale or add  values toggling this button
  GtkWidget *tlabel;        //the label for the button
  GtkWidget *curve_type;    //change curve type toggling this button
  GtkWidget *clabel;        //the label for the button
  GtkWidget *maxentry;      //max Y value entry 
  GtkWidget *minentry;      //min Y value entry 
  GtkWidget *durentry;      //Duration entry (only for synthesis)
  GtkWidget *info_label;    //The label for printing info
  short tflag;              //the toggle flag for the toggle operation button
  short cflag;              //the toggle flag for changing the curve type
  float ymin;               //min Y value  
  float ymax;               //max Y value
  float dur;                //output duration in  milliseconds(only for synthesis)
} ENVELOPE;


typedef struct { //the time data for each time breakpoint of the time function
  int   from; //from frame
  int   to;   //to frame
  int   size; //the size of the segment in frames
  float tfac; //stretching factor 1=invariant
} TIME_DATA;

typedef struct { //the data for the smart selection menu

  int from;        //from partial #
  int to;          //to partial #
  int step;        //the step
  float tres;      //amplitude treshold
  int met;         //amplitude evaluation method (0=peak, 1=RMS Power)

} SMSEL_DATA;

typedef struct { //the data for the randi UG
  int   size; //size of the frame in samples this should be sr/freq.
  float a1;   //first amplitude value
  float a2;   //next  amplitude value
  int   cnt;  //sample position counter
} RANDI;

//GLOBAL VARIABLES

double tl_sr;
float *sine_table;

//FROM MAIN
GtkWidget     *win_main;
GtkTooltips   *tooltips;
GtkAccelGroup *accel_group;
GtkWidget     *toolbar;
GtkWidget     *main_graph;
GtkWidget     *label;
GtkWidget     *isfile_label;
GtkWidget     *osfile_label;
GtkWidget     *afile_label;
GtkWidget     *rfile_label;

char *in_tittle;
char *out_tittle;
char *out_ats_tittle;
char *ats_tittle;
char *undo_file;
char *apf_tittle;
char *res_tittle;
char *info;
short *selected;
FILE *atsfin, *soundin, *soundout;

ATS_HEADER *atshed;
SPARAMS *sparams;
SELECTION *selection, *position;
ENVELOPE  *ampenv, *freenv, *timenv;
SMSEL_DATA *sdata;

ATS_SOUND *ats_sound;

float  *frbuf;
float  *avec;
float  *fvec;
float  *tvec;

int stopper;
int   ned, led, undo;
int   aveclen, tveclen;
float maxtim;
float frame_step, freq_step;
float valexp;
int   floaded, view_type,vertex1,vertex2;
short outype;
int   depth, interpolated, need_byte_swap, draw;

VIEW_PAR *h, *v;
UNDO_DATA *undat;

ANARGS *atsh_anargs;

GtkObject *hadj1,*hadj2;
GtkObject *vadj1,*vadj2, *valadj;
GtkWidget *hscale1, *hscale2;
GtkWidget *vscale1, *vscale2, *valscale;
GtkWidget *hruler, *vruler;

GtkWidget *fWedit;
GtkWidget *aWedit;
GtkWidget *tWedit;

GtkWidget **entry;
GdkPixmap *pixmap;
GdkGC *draw_pen;
GdkColor *draw_col;
GdkRectangle update_rect;

GtkWidget *optionmenu1,*optionmenu2;
GtkWidget **entrys;
//
GtkWidget *window1;
typProgressData *pdata;
//from list view
gchar  *ats_data[4];
gchar  *nbuf,*abuf,*fbuf,*pbuf,*n_text, *fto, *ffro;
gint offset, maxval, first;
GtkWidget *clist;
GtkObject *adj1;
GtkWidget *ti_num, *fr_num, *fr_from, *fr_to;
//From getsparams.c
char **param_list;
GtkWidget *tlabel1, *tlabel2;
//From header
char *hdata;
GtkWidget *window1;
//

//SYNTHESIS FUNCTIONS
void do_synthesis();
void synth_buffer_phint(float a1, float a2, float f1, float f2, float p1, float p2, float dt, float frame_samps);
void make_sine_table();
float ioscilator(float amp,float frec,int op,float *oscpt);
int locate_frame(int from_frame, float time, float dif);
void set_output_type(int *format, int *header);
void randi_setup(float sr, float freq, RANDI *radat);
float randi(RANDI *radat);
float randif(RANDI *radat, float freq);
void synth_deterministic_only(float a1, float a2, float f1, float f2, float frame_samps, int op, float *oscpt);
void synth_residual_only(float a1, float a2,float freq,float frame_samps,int op,float *oscpt, RANDI* rdata);
void synth_both(float a1, float a2, float f1, float f2, float frame_samps,int op, float *oscpt,float r1, float r2, RANDI* rdata);

//MAIN PROGRAM FUNCTIONS
void CreateMainWindow (char *cmdl_filename);
void DeSelectMenu (GtkWidget *widget, gpointer data);
GtkWidget *CreateMenuItem (GtkWidget *menu, char *szName, char *szAccel, char *szTip, GtkSignalFunc func,gpointer data);
GtkWidget *CreateMenuCheck (GtkWidget *menu, char *szName, GtkSignalFunc func,gpointer data);
void CreateToolbar (GtkWidget *vbox_main);
void Create_menu (GtkWidget *menubar);

gint EndProgram ();
void PrintFunc (GtkWidget *widget, gpointer data);

void init_scalers(gint how);
void h_scroll(GtkObject *adj,gpointer data);
void h_setup();
void v_scroll(GtkObject *adj,gpointer data);
void v_setup();

void mem_realloc();
void set_avec();
void edit_freq(GtkWidget *widget, gpointer data);
void edit_amp (GtkWidget *widget, gpointer data);
void edit_tim (GtkWidget *widget, gpointer data);
void set_time_env(ENVELOPE *tim, int do_dur);
void normalize_amplitude(GtkWidget *widget, gpointer data);

//ENVELOPE (CURVE) FUNCTIONS
GtkWidget  *create_edit_curve(int which_one, ENVELOPE *envelope);
void change_duration(GtkWidget *widget, ENVELOPE *data);
int   get_nbp(GtkWidget *curve);
float get_x_value(GtkWidget *curve, int breakpoint);
float get_y_value(GtkWidget *curve, int breakpoint); 
void do_fredit(GtkWidget *widget, ENVELOPE *data);
void do_amedit(GtkWidget *widget, ENVELOPE *data);
void cancel_wedit(GtkWidget *widget, gpointer *data);
void change_curve_type(GtkWidget *widget, ENVELOPE *data);
void change_operation(GtkWidget *widget, ENVELOPE *data);
void setmax(GtkWidget *widget, ENVELOPE *data);
void setmin(GtkWidget *widget, ENVELOPE *data);
void setmax_time(GtkWidget *widget, ENVELOPE *data);
void setmin_time(GtkWidget *widget, ENVELOPE *data);
void ismoving(GtkWidget *widget, GdkEventMotion *event, ENVELOPE *data);
void ismoving_time(GtkWidget *widget, GdkEventMotion *event, ENVELOPE *data);
void hidewindow(GtkWidget *widget);
void curve_reset(GtkWidget *widget, ENVELOPE *data);
void curve_reset_time(GtkWidget *widget, ENVELOPE *data);


//PROGRESSBAR FUNCTIONS
void StartProgress(char *message, int canstop);
void UpdateProgress(int pos, int len);
void EndProgress ();
gint CanWindowClose(GtkWidget *widget);
void stop_process();

//POPUP DIALOG WINDOW FUNCTIONS
void CloseDialog (GtkWidget *widget, gpointer data);
void ClosingDialog (GtkWidget *widget, gpointer data);
void Popup (char *szMessage);

//LIST VIEW FUNCTIONS
void update_time(gfloat valt, gint valf);
void redraw_screen();
void clist_update(GtkAdjustment *adj);
void delete_window (GtkWidget *widget, gpointer data);
void init_str();
void selection_made(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data );
void unselection_made(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data );
int list_view();
void from_now(GtkWidget *widget, GtkAdjustment *adj);
void to_now(GtkWidget *widget, GtkAdjustment *adj);

// SYNTHESIS PARAMETERS SELECTION FUNCTION
void get_sparams ();
GtkWidget *CreateEditField (char *name, int what);
char **alocstring(int np , int nc);
void set_params();
void allorsel (GtkWidget *widget, gpointer data);
void upha(GtkWidget *widget, gpointer data);

//UNDO FUNCTIONS
void do_undo(GtkWidget *widget,gpointer data);
void backup_edition(int eddie);

//GRAPHIC DISPLAY / SELECTION FUNCTIONS
gint motion_notify(GtkWidget *widget, GdkEventMotion *event);
gint expose_event(GtkWidget *widget, GdkEventExpose *event);
gint configure_event(GtkWidget *widget, GdkEventConfigure *event);
gint click(GtkWidget *widget, GdkEventButton *event); 
void change_color (int nRed, int nGreen, int nBlue);
void draw_selection_line(int x);
void draw_selection();
void erase_selection(int pfrom, int pto);
void draw_default();
void draw_pixm();
void repaint(gpointer data);
void set_selection(int from, int to, int x1, int x2, int width);
void set_hruler(double from, double to, double pos, double max);
void set_spec_view();
void set_res_view();
void update_value(GtkAdjustment *adj);
void unzoom();
void zoom_sel();
void sel_only();
void sel_all();
void sel_un();
void sel_even();
void sel_odd();
void revert_sel();


//ABOUT WINDOW FUNCTIONS
void show_header (void);
void hclose(GtkWidget *widget, gpointer data);
void create_hsep();
void about(void);

//FILE FUNCTIONS
GtkWidget *create_men_snd(char *label, int ID, GtkWidget *parent);
void byte_swap_header(ATS_HEADER *hed, int flag);
double byte_swap(double *in);
void setres(char *pointer);
void setaud(char *pointer);
void setin(char *pointer);
void setout(char *pointer);
void atsin(char *pointer);
void atsout(char *pointer);
void setout_ats(char *pointer);
void set_filter(GtkWidget *widget, gpointer data);
void GetFilename (char *sTitle, void (*callback) (char *),char* selected, char *filter,int opid);
void FileOk (GtkWidget *w, gpointer data);
void destroy (GtkWidget *widget, gpointer *data);
void filesel(GtkWidget *widget,char *what);
void stringinit();
void stringfree();
int  my_filelength(FILE *fpt);
void edit_audio();
void choose_output(GtkWidget *widget, gpointer data);
void getap(char *pointer);
void savap(char *pointer);

//ANALYSIS WINDOW FUNCTIONS
void change_ats_type(GtkWidget *widget, gpointer data);
void select_out_atsfile(GtkWidget *widget, gpointer *data);
void select_in_soundfile(GtkWidget *widget, gpointer *data);
void set_defaults(GtkWidget *widget, gpointer *data);
void unload_ats_file();
void change_win_type(GtkWidget *widget, gpointer data);
GtkWidget *create_itemen(char *label, int ID, GtkWidget *parent, int which);
void set_aparam(GtkWidget *widget, gpointer data);
void do_analysis (GtkWidget *widget, gpointer data);
void destroy_wanalysis (GtkWidget *widget, gpointer data);
GtkWidget *create_label(char *winfo, int p1,int p2,int p3,int p4, 
			GtkWidget *window ,GtkWidget *table, char *ID);
GtkWidget *create_entry(int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID, float value, char *strbuf, int isint, int width);
GtkWidget *create_button(char *winfo, int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID);
void create_ana_dlg (void);
void get_ap(GtkWidget *widget, gpointer *data);
void sav_ap(GtkWidget *widget, gpointer *data);
void update_aparameters();

//SELECTION FILTER FUNCTIONS
void create_sel_dlg (void);
void destroy_smartsel (GtkWidget *widget, gpointer data);
void do_smartsel (GtkWidget *widget, gpointer data);
void get_values(GtkWidget *widget, gpointer data);
GtkObject *create_adj(GtkWidget *window, GtkWidget *table, float min, float max,float value,
		      int p1, int p2, int p3, int p4, char *ID);
void set_met(GtkWidget *widget, gpointer data);


