/*
ATSH.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "atsh.h"
#include "my_curve.h"

#define APP_W 800 
#define APP_H 400 

typedef struct {
  char *name;
  char *accel;
  GCallback func;
  gpointer data;
} MENUITEM;

typedef struct {
  char *name;
  int num;
  MENUITEM *items;
} SUBMENU;

void CreateMainWindow (char *cmdl_filename);
void scroll(GtkObject *adj,gpointer data);
void Create_menu (GtkWidget *box);
//void repaint(void);

GtkWidget *main_graph, *statusbar;
gint context_id;
GtkWidget *label;

char apf_title[256] = "";
char res_title[256] = DEFAULT_OUTPUT_RES_FILE;
char out_title[256] = DEFAULT_OUTPUT_FILE;
char out_ats_title[256] = DEFAULT_OUTPUT_ATS_FILE;
char in_title[256] = "";
char undo_file[256] = UNDO_FILE;
char ats_title[256] = "";

extern float *sine_table;

GtkObject *hadj1,*hadj2;
GtkObject *vadj1,*vadj2, *valadj;
GtkWidget *hscale1, *hscale2;
GtkWidget *vscale1, *vscale2, *valscale;
GtkWidget *hruler, *vruler;
UNDO_DATA *undat;
GtkWidget **entry;

extern ATS_HEADER atshed;
extern float *avec, *fvec;
SELECTION selection, position;
VIEW_PAR h, v;
extern int view_type, aveclen;
GtkWidget *win_main;
int floaded = FALSE;
SPARAMS sparams = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 44100, FALSE, FALSE};
short smr_done = FALSE;
extern ATS_SOUND *ats_sound;


int main(int argc, char *argv[])
{
  char suffix[10];
  
  /* --- Initialize GTK --- */
  gtk_init (&argc, &argv);
  /* --- SINE TABLE --- */ 
  make_sine_table();    
  /* initialize sndlib */
  mus_sound_initialize();

  selected=(short*)malloc(sizeof(short));
  info=(char*)malloc(sizeof(char)*1024);

  /* set backup file name */
  sprintf(suffix, "_%u", getpid());
  strcat(undo_file,suffix);
    
  entry=(GtkWidget**)g_malloc(sizeof(GtkWidget*)*6);
  undat =(UNDO_DATA*)malloc(sizeof(UNDO_DATA));
  ampenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
  freenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
  timenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
  sdata= (SMSEL_DATA*)malloc(sizeof(SMSEL_DATA));

  vertex1 = vertex2 = FALSE;
  //    outype =WAV16;

  /* see if we have an ats file delivered by command line */
  if(argc > 1) CreateMainWindow(argv[1]); //pass the filename to main function
  else CreateMainWindow(NULL); //no filename delivered by command line

  gtk_main();
  return(0);
}


gint EndProgram ()
{
  
  //  free(atshed);
  free(sine_table);
  if(!entry)g_free(entry);

  free(selected);
  free(info);
  free(undat);
  free(sdata);
  free(avec);
  free(fvec);
  
  //kill UNDO file
  remove(undo_file);
  
  //  gdk_pixmap_unref(pixmap);
  gdk_gc_unref(draw_pen);

  //probably dont need to destoy anything here
  //these will be destroyed by their parent
  //make sure they actually have a parent  
  if(fWedit)
    gtk_widget_destroy (GTK_WIDGET (fWedit)); //Kill Frequency edit Window
  if(aWedit)
    gtk_widget_destroy (GTK_WIDGET (aWedit)); //Kill Amplitude edit Window
  if(tWedit)
    gtk_widget_destroy (GTK_WIDGET (tWedit)); //Kill Time edit Window
  
  free(ampenv);
  free(freenv);
  free(timenv);
  free_sound(ats_sound);
  
  /* --- End gtk event loop processing --- */
  gtk_main_quit ();
  
  /* --- Ok to close the app. --- */
  return (FALSE);
}

void CreateMainWindow (char *cmdl_filename)
{
  GtkWidget *main_box, *vbox, *hbox, *center_table;

  /* --- Create the top window and size it. --- */
  win_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (GTK_WIDGET (win_main), APP_W, APP_H);
  gtk_window_set_position(GTK_WINDOW(win_main),GTK_WIN_POS_CENTER);
   
  /* --- Top level window should listen for the delete_event --- */
  g_signal_connect (G_OBJECT (win_main), "delete_event", G_CALLBACK(EndProgram), NULL);

  /* --- main box --- */
  main_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (main_box);
  gtk_container_add (GTK_CONTAINER(win_main), main_box);

  /* --- Menu Bar --- */
  Create_menu(main_box);

  /* center table */
  center_table = gtk_table_new (2,2,FALSE);
  gtk_widget_show (center_table);
  gtk_box_pack_start(GTK_BOX(main_box), center_table, TRUE, TRUE, 0);

  /* status bar */
  statusbar = gtk_statusbar_new();
  context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "");
  gtk_box_pack_end (GTK_BOX (main_box), statusbar, FALSE, FALSE, 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Welcome to ATSH");
  gtk_widget_show(statusbar);
     
  /* graphic containers */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_table_attach(GTK_TABLE(center_table), vbox, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0,0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach(GTK_TABLE(center_table), hbox, 1, 2, 0, 1, GTK_FILL, GTK_FILL | GTK_EXPAND, 0,0);

  /* main graphic area */
  main_graph= gtk_drawing_area_new();
  gtk_table_attach_defaults(GTK_TABLE(center_table), main_graph, 0, 1, 0, 1);
    
  g_signal_connect(G_OBJECT(main_graph), "expose-event", G_CALLBACK(expose_event),NULL);
  g_signal_connect(G_OBJECT(main_graph), "configure_event",G_CALLBACK(configure_event), NULL);
  g_signal_connect(G_OBJECT(main_graph), "motion_notify_event",G_CALLBACK(motion_notify) ,NULL);
  g_signal_connect(G_OBJECT(main_graph), "button-press-event",G_CALLBACK(click),NULL);

  gtk_widget_set_events(main_graph, GDK_EXPOSURE_MASK
                        | GDK_LEAVE_NOTIFY_MASK
                        | GDK_BUTTON_PRESS_MASK
                        | GDK_POINTER_MOTION_MASK
                        | GDK_POINTER_MOTION_HINT_MASK
                        | GDK_BUTTON_RELEASE_MASK);
    
  draw_col = (GdkColor *) g_malloc (sizeof (GdkColor));

  /* RULERS go first */
  hruler = gtk_hruler_new ();
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(hruler), FALSE, FALSE, 0);
  vruler = gtk_vruler_new ();
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET(vruler), FALSE, FALSE,0);
     
  /* scales */
  vadj1 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  vadj2 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  hadj1 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  hadj2 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  valadj = gtk_adjustment_new(0., 0., 0., 1., 100., 0.);

  hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (hadj1));
  gtk_scale_set_digits (GTK_SCALE(hscale1),0);
  g_signal_connect (G_OBJECT(hadj1), "value_changed", G_CALLBACK (scroll), GINT_TO_POINTER(1));
  gtk_box_pack_start (GTK_BOX (vbox), hscale1, FALSE, TRUE, 0);

  hscale2 = gtk_hscale_new (GTK_ADJUSTMENT (hadj2));
  gtk_scale_set_digits (GTK_SCALE(hscale2),0);
  g_signal_connect (G_OBJECT(hadj2), "value_changed", G_CALLBACK (scroll),GINT_TO_POINTER(2) );
  gtk_scale_set_draw_value(GTK_SCALE(hscale2),FALSE);
  gtk_range_set_update_policy(GTK_RANGE(hscale2),GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start (GTK_BOX (vbox), hscale2, FALSE, TRUE, 0);
     
  vscale1 = gtk_vscale_new (GTK_ADJUSTMENT (vadj1));
  gtk_scale_set_draw_value(GTK_SCALE(vscale1),FALSE);
  g_signal_connect (G_OBJECT(vadj1), "value_changed", G_CALLBACK (scroll),GINT_TO_POINTER(3) );   
  gtk_box_pack_start (GTK_BOX (hbox), vscale1, FALSE, FALSE, 0);

  vscale2 = gtk_vscale_new (GTK_ADJUSTMENT (vadj2));
  g_signal_connect (G_OBJECT(vadj2), "value_changed", G_CALLBACK (scroll), GINT_TO_POINTER(4));
  gtk_scale_set_draw_value(GTK_SCALE(vscale2),FALSE);
  gtk_range_set_update_policy(GTK_RANGE(vscale2),GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start (GTK_BOX (hbox), vscale2, FALSE, FALSE, 0);

  valscale = gtk_vscale_new (GTK_ADJUSTMENT (valadj));
  gtk_box_pack_start (GTK_BOX (hbox), valscale, FALSE, FALSE,20);
  gtk_scale_set_digits (GTK_SCALE (valscale), 0);
  gtk_range_set_update_policy (GTK_RANGE (valscale), GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE (valscale),FALSE);
  g_signal_connect (G_OBJECT(valadj), "value_changed", G_CALLBACK(update_value), NULL);

  gtk_widget_show(main_graph);
  gtk_widget_show(win_main);

  if(cmdl_filename) atsin(cmdl_filename);
  else show_file_name(NULL);
}

#define NUMMENUS 5

void Create_menu(GtkWidget *box)
{
  int i, j;
  GtkAccelGroup *accel_group;
  GtkWidget *rootmenu, *menu, *menuitem, *menubar;  
  MENUITEM analysismenu[6] = {{"Open ATS file", "^O", G_CALLBACK(filesel), "atsin"},
                          {"Save ATS file", "^S", G_CALLBACK(filesel), "atsave"},
                          {NULL, NULL, NULL, NULL},
                          {"New ATS File", "^N", G_CALLBACK(create_ana_dlg), NULL},
                          {NULL, NULL, NULL, NULL},
                          {"Quit", "^Q", G_CALLBACK(EndProgram), NULL}};
  MENUITEM transformationmenu[12] = {{"Undo", "^0", G_CALLBACK(do_undo), GINT_TO_POINTER(-1)},
                                 {NULL, NULL, NULL, NULL},
                                 {"Select All Partials", "^1", G_CALLBACK(sel_all), NULL},
                                 {"Unselect All Partials", "^2", G_CALLBACK(sel_un), NULL},
                                 {"Select Even Partials", "^3", G_CALLBACK(sel_even), NULL},
                                 {"Select Odd Partials", "^4", G_CALLBACK(sel_odd), NULL},
                                 {"Invert Partial Selection", "^5", G_CALLBACK(revert_sel), NULL},
                                 {"Smart Selection", "^6", G_CALLBACK(create_sel_dlg), NULL},
                                 {NULL, NULL, NULL, NULL},
                                 {"Edit Amplitude", "^7", G_CALLBACK(edit_amp), NULL},
                                 {"Normalize Selection", "^8", G_CALLBACK(normalize_amplitude), NULL},
                                 {"Edit Frequency", "^9", G_CALLBACK(edit_freq), NULL}};
  MENUITEM synthesismenu[3] = {{"Synthesis Parameters", "^P", G_CALLBACK(get_sparams), "Parameters"},
                           {NULL, NULL, NULL, NULL},
                           {"Synthesize", "^R", G_CALLBACK(do_synthesis), "Synthesize"}};
  MENUITEM viewmenu[12] = {{"List", "^T", G_CALLBACK(list_view), NULL},
                       {NULL, NULL, NULL, NULL},
                       {"Sinusoidal Amplitude", "^C", G_CALLBACK(set_spec_view), NULL},
                       {"Sinusoidal SMR", "", G_CALLBACK(set_smr_view), NULL},
                       {"Noise", "^D", G_CALLBACK(set_res_view), NULL},
                       {"Lines / Dashes", "^M", G_CALLBACK(set_interpolated_view), NULL},
                       {NULL, NULL, NULL, NULL},
                       {"unzoom", "^U", G_CALLBACK(unzoom), NULL},
                       {"zoom selection", "^Z", G_CALLBACK(zoom_sel), NULL}, 
                       {"all / only selection", "^W", G_CALLBACK(sel_only), NULL},
                       {NULL, NULL, NULL, NULL},
                       {"Header Data", "^E", G_CALLBACK(show_header), NULL}};
  MENUITEM helpmenu[2] = {{"Analysis Parameters", "^H", G_CALLBACK(help), NULL},
                      {"About ATSH", "", G_CALLBACK(about), NULL}};
  SUBMENU menus[NUMMENUS] = {{"Analysis", 6, analysismenu},
                             {"Transformation", 12, transformationmenu},
                             {"Synthesis", 3, synthesismenu},
                             {"View", 12, viewmenu},
                             {"Help", 2, helpmenu}};

  menubar = gtk_menu_bar_new();
  gtk_box_pack_start (GTK_BOX(box), menubar, FALSE, FALSE, 0);
  gtk_widget_show(menubar);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (win_main), accel_group);

  for (i=0; i<NUMMENUS; i++) {
    menu = gtk_menu_new();
    for (j=0; j<menus[i].num; j++) {
      if(menus[i].items[j].name != NULL) {
        menuitem = gtk_menu_item_new_with_label(menus[i].items[j].name);
        g_signal_connect (G_OBJECT(menuitem), "activate", menus[i].items[j].func, menus[i].items[j].data);
        if (menus[i].items[j].accel != NULL && menus[i].items[j].accel[0] == '^') 
          gtk_widget_add_accelerator(menuitem, "activate", accel_group, menus[i].items[j].accel[1], GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
      } else menuitem = gtk_menu_item_new();
      gtk_menu_shell_append (GTK_MENU_SHELL(menu), menuitem);
      gtk_widget_show(menuitem);

    }
    rootmenu = gtk_menu_item_new_with_label(menus[i].name);
    gtk_widget_show(rootmenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(rootmenu), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), rootmenu);
  }
}



void init_scalers(void)
{
  if(floaded) {
    gtk_ruler_set_range (GTK_RULER (hruler),1.,atshed.fra, 1.,atshed.fra);
    gtk_ruler_set_range (GTK_RULER (vruler),((atshed.mf*ATSH_FREQ_OFFSET) - 1.)/1000., 
                         1., 0.,((atshed.mf*ATSH_FREQ_OFFSET )- 1.)/1000.);
    h.viewstart=1;
    h.viewend=(int)atshed.fra;
    h.prev = h.diff = h.viewend - h.viewstart;      
    GTK_ADJUSTMENT(hadj1)->lower = GTK_ADJUSTMENT(hadj1)->upper = GTK_ADJUSTMENT(hadj2)->lower = 1.;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj1),1.);
    GTK_ADJUSTMENT(hadj2)->upper=atshed.fra;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj2),atshed.fra);
   
    v.viewstart = 1;
    v.viewend = (int)(atshed.mf*ATSH_FREQ_OFFSET);
    v.prev = v.diff = (int)(atshed.mf*ATSH_FREQ_OFFSET);
    GTK_ADJUSTMENT(vadj1)->lower = GTK_ADJUSTMENT(vadj1)->upper = GTK_ADJUSTMENT(vadj2)->lower = 1.;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj1),1.);
    GTK_ADJUSTMENT(vadj2)->upper=(atshed.mf* ATSH_FREQ_OFFSET)-1.;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj2),(atshed.mf* ATSH_FREQ_OFFSET));

    gtk_widget_show(GTK_WIDGET(vscale1));
    gtk_widget_show(GTK_WIDGET(vscale2));
    gtk_widget_show(GTK_WIDGET(hscale1));
    gtk_widget_show(GTK_WIDGET(hscale2));
    gtk_widget_show(GTK_WIDGET(valscale));
    gtk_widget_show(GTK_WIDGET(hruler));
    gtk_widget_show(GTK_WIDGET(vruler));
  } else {
    gtk_widget_hide(GTK_WIDGET(vscale1));
    gtk_widget_hide(GTK_WIDGET(vscale2));
    gtk_widget_hide(GTK_WIDGET(hscale1));
    gtk_widget_hide(GTK_WIDGET(hscale2));
    gtk_widget_hide(GTK_WIDGET(valscale));
    gtk_widget_hide(GTK_WIDGET(hruler));
    gtk_widget_hide(GTK_WIDGET(vruler));
  }
}

void scroll(GtkObject *adj, gpointer data)
{ 
  switch(GPOINTER_TO_INT(data)) {
  case 1:
    h.viewstart  = (int)GTK_ADJUSTMENT(hadj1)->value;
    h.viewend    = h.viewstart + ((int)GTK_ADJUSTMENT(hadj2)->value)-1;  
    h.diff       = h.viewend - h.viewstart;      
    GTK_ADJUSTMENT(hadj2)->upper= (atshed.fra - GTK_ADJUSTMENT(hadj1)->value) + 1;
    gtk_adjustment_changed(GTK_ADJUSTMENT(hadj2));
    set_hruler((double)h.viewstart, (double)h.viewend, (double)h.viewstart, (double)h.diff);
    break;
  case 2:
    h.viewend    = h.viewstart +((int)GTK_ADJUSTMENT(hadj2)->value)-1;
    h.diff=h.viewend - h.viewstart;
    GTK_ADJUSTMENT(hadj1)->upper=(atshed.fra - GTK_ADJUSTMENT(hadj2)->value) + 1;
    gtk_adjustment_changed (GTK_ADJUSTMENT(hadj1));
    set_hruler((double)h.viewstart, (double)h.viewend, (double)h.viewstart, (double)h.diff);
    break;
  case 3:
    v.viewstart  = (int)GTK_ADJUSTMENT(vadj1)->value;
    v.viewend    = (v.viewstart + ((int)GTK_ADJUSTMENT(vadj2)->value));  
    v.diff       = ((int)GTK_ADJUSTMENT(vadj2)->value);      
    GTK_ADJUSTMENT(vadj2)->upper= (atshed.mf*ATSH_FREQ_OFFSET) - GTK_ADJUSTMENT(vadj1)->value ;
    gtk_adjustment_changed(GTK_ADJUSTMENT(vadj2));
    if(VIEWING_DET) gtk_ruler_set_range(GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
                                        (float)v.viewstart/1000.,(float)v.diff/1000.);
    break;
  case 4: 
    v.viewend    = v.viewstart +((int)GTK_ADJUSTMENT(vadj2)->value);
    v.diff       = (int)GTK_ADJUSTMENT(vadj2)->value; 
    GTK_ADJUSTMENT(vadj1)->upper=(atshed.mf*ATSH_FREQ_OFFSET) - GTK_ADJUSTMENT(vadj2)->value;
    gtk_adjustment_changed (GTK_ADJUSTMENT(vadj1));
    if(VIEWING_DET) gtk_ruler_set_range (GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
                                         (float)v.viewstart/1000.,(float)v.diff/1000.);
    break;
  }
  draw_pixm();
}

void edit_freq(GtkWidget *widget, gpointer data)
{
  if(floaded && !(NOTHING_SELECTED) && (view_type!=RES_VIEW)) {
    if(fWedit == NULL) fWedit=create_edit_curve(FRE_EDIT, freenv);
    else gtk_widget_show(GTK_WIDGET(fWedit));
  }
}

void edit_amp(GtkWidget *widget, gpointer data)
{
  if(floaded && !(NOTHING_SELECTED) && (view_type != RES_VIEW)) {
    if(aWedit == NULL) aWedit = create_edit_curve(AMP_EDIT, ampenv);
    else gtk_widget_show(GTK_WIDGET(aWedit));
  }
}

void edit_tim(GtkWidget *widget, gpointer data)
{     
  if(floaded) {
    if(tWedit == NULL) tWedit=create_edit_curve(TIM_EDIT, timenv);
    else set_time_env(timenv, FALSE);
    gtk_widget_show(GTK_WIDGET(tWedit));
  }
}

void set_time_env(ENVELOPE *tim, int do_dur)
{    

  if(NOTHING_SELECTED) {
    tim->ymax=(float)ats_sound->time[0][(int)atshed.fra-1];
    tim->ymin=0.;
  } else {
    tim->ymax=(float)ats_sound->time[0][selection.to];
    tim->ymin=(float)ats_sound->time[0][selection.from]; 
  }
   
  if(do_dur){
    tim->dur =tim->ymax - tim->ymin;  
//     *info=0;
//     sprintf(info," %12.5f", tim->dur);
//     gtk_entry_set_text(GTK_ENTRY (tim->durentry),info);
  }
  
//   *info=0;
//   sprintf(info," %12.5f",tim->ymin);
//   gtk_entry_set_text(GTK_ENTRY (tim->minentry),info);
  
//   *info=0;
//   sprintf(info," %12.5f",tim->ymax);
//   gtk_entry_set_text(GTK_ENTRY (tim->maxentry),info);    
}

void normalize_amplitude(GtkWidget *widget, gpointer data)
{
  int i, x, nValue=0;
  float normval= 1. / (float)atshed.ma;
  float maxamp=0.;
  int todo= aveclen * 2 * (int)atshed.par;

  if(floaded && !(NOTHING_SELECTED) && (view_type != RES_VIEW)) {
    backup_edition(AMP_EDIT);
    StartProgress("Normalizing selection...",FALSE);
       
    //find out max amplitude of selection
    for(i=selection.from; i < selection.from+aveclen; ++i)
      for(x=0; x < (int)atshed.par ; ++x) {
        if(ats_sound->amp[x][i] >= maxamp) maxamp=ats_sound->amp[x][i];  
        UpdateProgress(++nValue,todo);
      }

    //now do it
    normval= 1. / maxamp;
    for(i=selection.from; i < selection.from+aveclen; ++i) 
      for(x=0; x < (int)atshed.par ; ++x) {
        if(selected[x]== TRUE) ats_sound->amp[x][i]*=normval;  
        UpdateProgress(++nValue,todo);
      }
  
    atshed.ma=(double)1.;
    EndProgress();
    draw_pixm();
    smr_done=FALSE;
  }
}

void show_file_name(char *name)
{
  char str[263];

  sprintf(str, "ATSH - %s", name);
  gtk_window_set_title(GTK_WINDOW(win_main), str);
}
