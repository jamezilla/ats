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

void CreateMainWindow (char *cmdl_filename);
void h_scroll(GtkObject *adj,gpointer data);
void v_scroll(GtkObject *adj,gpointer data);
void v_setup();
GtkWidget *CreateMenuItem (GtkWidget *menu, char *szName, char *szAccel, char *szTip, GtkSignalFunc func,gpointer data);
void Create_menu (GtkWidget *menubar);

//GtkTooltips *tooltips;
GtkWidget *toolbar, *statusbar;
gint context_id;
GtkWidget *main_graph;
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
GtkAccelGroup *accel_group;
GtkWidget *win_main;
int floaded = FALSE;
SPARAMS sparams = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 44100, FALSE, FALSE};
short smr_done = FALSE;
//ANARGS *anargs;
extern ATS_SOUND *ats_sound;

/*
 * main
 * 
 * --- Program begins here
 */
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

/*
 * EndProgram
 *
 * Exit from the program
 */
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

/*
 * CreateMainWindow
 *
 * Create the main window and the menu/toolbar associated with it
 */
void CreateMainWindow (char *cmdl_filename)
{
  GtkWidget *vbox_main, *vbox_main2, *vbox_inf;
  GtkWidget *hbox_main, *hbox_main1, *hbox_main2;
  GtkWidget *menubar;   

  /* --- Create the top window and size it. --- */
  win_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize(win_main, APP_W, APP_H);
  gtk_window_set_position(GTK_WINDOW(win_main),GTK_WIN_POS_CENTER);
   
  /* --- Top level window should listen for the delete_event --- */
  gtk_signal_connect (GTK_OBJECT (win_main), "delete_event", GTK_SIGNAL_FUNC(EndProgram), NULL);

  /* --- Vbox and Hbox --- */
  hbox_main = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox_main);
  gtk_container_add (GTK_CONTAINER (win_main), hbox_main);
  
  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_main);
  gtk_box_pack_start (GTK_BOX (hbox_main), vbox_main, TRUE, TRUE, 0);
    
  vbox_main2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_main2);
  gtk_box_pack_start (GTK_BOX (hbox_main), vbox_main2, FALSE, FALSE,0); 

  hbox_main1 = gtk_hbox_new (FALSE, 0);////DO NOT CHANGE!!!
  gtk_widget_show (hbox_main1);
  gtk_box_pack_start (GTK_BOX (vbox_main2), hbox_main1, FALSE, FALSE,12); ///12 DO NOT CHANGE!!!
   
  hbox_main2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox_main2);
  gtk_box_pack_start (GTK_BOX (vbox_main2), hbox_main2, TRUE, TRUE, 0); 

  /* --- Menu Bar --- */
  menubar = gtk_menu_bar_new();
  gtk_box_pack_start (GTK_BOX(vbox_main), menubar, FALSE, TRUE, 0);
  Create_menu(GTK_WIDGET(menubar));
  gtk_widget_show(menubar);

  /* main graphic area */
  main_graph= gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(main_graph), APP_W, APP_H);
  gtk_box_pack_start (GTK_BOX (vbox_main),main_graph, TRUE, TRUE, 0);     
    
  gtk_signal_connect(GTK_OBJECT(main_graph), "expose-event", (GtkSignalFunc) expose_event,NULL);
  gtk_signal_connect(GTK_OBJECT(main_graph), "configure_event",(GtkSignalFunc) configure_event, NULL);
  gtk_signal_connect(GTK_OBJECT(main_graph), "motion_notify_event",(GtkSignalFunc) motion_notify ,NULL);
  gtk_signal_connect(GTK_OBJECT(main_graph), "button-press-event",(GtkSignalFunc)click,NULL);

  gtk_widget_set_events(main_graph, GDK_EXPOSURE_MASK
                        | GDK_LEAVE_NOTIFY_MASK
                        | GDK_BUTTON_PRESS_MASK
                        | GDK_POINTER_MOTION_MASK
                        | GDK_POINTER_MOTION_HINT_MASK
                        | GDK_BUTTON_RELEASE_MASK);
    
  draw_col = (GdkColor *) g_malloc (sizeof (GdkColor));

  /* scalers */
  vadj1 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  vadj2 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  hadj1 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
  hadj2 = gtk_adjustment_new (0.0, 0.0, 0.0, 1.0, 1.0, 0.0);
     
  /* RULERS go first */
  hruler = gtk_hruler_new ();
  gtk_box_pack_start (GTK_BOX (vbox_main), GTK_WIDGET(hruler), FALSE, FALSE, 0);
  gtk_ruler_set_range (GTK_RULER (hruler), 1., 1., 1., 1.);
  gtk_ruler_set_metric(GTK_RULER (hruler), GTK_PIXELS);

  vruler = gtk_vruler_new ();
  gtk_box_pack_start (GTK_BOX (hbox_main2), GTK_WIDGET(vruler), FALSE, FALSE,0);
  gtk_ruler_set_range (GTK_RULER (vruler), 1., 1., 1., 1.);
  gtk_ruler_set_metric(GTK_RULER (vruler), GTK_PIXELS);
     
  /************/
  hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (hadj1));
  gtk_scale_set_digits (GTK_SCALE(hscale1),0);
  gtk_signal_connect (GTK_OBJECT(hadj1), "value_changed", GTK_SIGNAL_FUNC (h_scroll), GINT_TO_POINTER(1));
  gtk_box_pack_start (GTK_BOX (vbox_main), hscale1, FALSE, TRUE, 0);
  /************/
  hscale2 = gtk_hscale_new (GTK_ADJUSTMENT (hadj2));
  gtk_scale_set_digits (GTK_SCALE(hscale2),0);
  gtk_signal_connect (GTK_OBJECT(hadj2), "value_changed", GTK_SIGNAL_FUNC (h_scroll),GINT_TO_POINTER(2) );
  gtk_scale_set_draw_value(GTK_SCALE(hscale2),FALSE);
  gtk_range_set_update_policy(GTK_RANGE(hscale2),GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start (GTK_BOX (vbox_main), hscale2, FALSE, TRUE, 0);
     
  /************/
  vscale1 = gtk_vscale_new (GTK_ADJUSTMENT (vadj1));
  gtk_scale_set_draw_value(GTK_SCALE(vscale1),FALSE);
  gtk_signal_connect (GTK_OBJECT(vadj1), "value_changed", GTK_SIGNAL_FUNC (v_scroll),GINT_TO_POINTER(1) );   
  gtk_box_pack_start (GTK_BOX (hbox_main2), vscale1, FALSE, FALSE, 0); //FF 8

  /************/
  vscale2 = gtk_vscale_new (GTK_ADJUSTMENT (vadj2));
  gtk_signal_connect (GTK_OBJECT(vadj2), "value_changed", GTK_SIGNAL_FUNC (v_scroll), GINT_TO_POINTER(2));
  gtk_scale_set_draw_value(GTK_SCALE(vscale2),FALSE);
  gtk_range_set_update_policy(GTK_RANGE(vscale2),GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start (GTK_BOX (hbox_main2), vscale2, FALSE, FALSE, 0); //FF8
    
  /************/
  valadj=gtk_adjustment_new(0., 0., 0., 1., 100., 0.);
     
  valscale = gtk_vscale_new (GTK_ADJUSTMENT (valadj));
  gtk_box_pack_start (GTK_BOX (hbox_main2), valscale, FALSE, FALSE,20); //FF5
  gtk_scale_set_digits (GTK_SCALE (valscale), 0);
  gtk_range_set_update_policy (GTK_RANGE (valscale), GTK_UPDATE_DISCONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE (valscale),FALSE);
  gtk_signal_connect (GTK_OBJECT(valadj), "value_changed", GTK_SIGNAL_FUNC(update_value), NULL);
     
  /***********/
  vbox_inf = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox_inf);
  gtk_box_pack_start (GTK_BOX (vbox_main2), vbox_inf, FALSE, FALSE,32); //DO NOT CHANGE!!!32

  statusbar =gtk_statusbar_new();
  context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "");
  gtk_box_pack_start (GTK_BOX (vbox_main), statusbar, FALSE, FALSE, 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Welcome to ATSH");
  gtk_widget_show(statusbar);

  init_scalers(FALSE);
  gtk_widget_show(main_graph);
  gtk_widget_show(win_main);
     
  if(cmdl_filename) atsin(cmdl_filename);
  else {
    //   draw_default();
    show_file_name(NULL);
  }

}

GtkWidget *CreateMenuItem (GtkWidget *menu, char *szName, char *szAccel, char *szTip, GtkSignalFunc func, gpointer data)
{
  GtkWidget *menuitem;
  GtkTooltips *tooltips = gtk_tooltips_new();

  /* If there's a name, create the item and put a Signal handler on it. */
  if (szName && strlen (szName)) {
    menuitem = gtk_menu_item_new_with_label (szName);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate", GTK_SIGNAL_FUNC(func), data);
  } else menuitem = gtk_menu_item_new ();  /* Create a separator */

  /* --- Add menu item to the menu and show it. --- */
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  if (accel_group == NULL) {
    accel_group = gtk_accel_group_new ();
    gtk_accel_group_attach (accel_group, GTK_OBJECT (win_main));
  }

  /* --- If there was an accelerator --- */
  if (szAccel && szAccel[0] == '^') 
    gtk_widget_add_accelerator (menuitem, "activate", accel_group, szAccel[1], GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  /* --- If there was a tool tip --- */
  if (szTip && strlen (szTip)) gtk_tooltips_set_tip (tooltips, menuitem, szTip, NULL);

  return (menuitem);
}


GtkWidget *CreateSubMenu (GtkWidget *menu, char *szName)
{
  GtkWidget *menuitem, *submenu;
 
  menuitem = gtk_menu_item_new_with_label (szName);   /* Create menu */

  gtk_menu_bar_append (GTK_MENU_BAR (menu), menuitem);   /* Add it to the menubar */
  gtk_widget_show (menuitem);

  submenu = gtk_menu_new ();   /* Get a menu and attach to the menuitem */
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);

  return (submenu);
}


void Create_menu (GtkWidget *menubar)
{   
  GtkWidget *menu, *menuitem;  

  /* Analysis */
  menu =(GtkWidget *)( CreateSubMenu (menubar, "Analysis"));
  menuitem = CreateMenuItem (menu, "Open ATS file", "^O", "Load Analysis File", GTK_SIGNAL_FUNC (filesel),"atsin");
  menuitem = CreateMenuItem (menu, "Save ATS file", "^S", "Save Analysis File", GTK_SIGNAL_FUNC (filesel),"atsave");
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "New ATS File", "^N", "Select and Analyze input soundfile", GTK_SIGNAL_FUNC (create_ana_dlg), NULL);
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Quit", "^Q", "Exit ATSH", GTK_SIGNAL_FUNC (EndProgram), NULL);

  /* Transformation menu */
  menu = (GtkWidget *)CreateSubMenu (menubar, "Transformation");
  menuitem = CreateMenuItem (menu, "Undo", "^0", "Undo last edition", GTK_SIGNAL_FUNC (do_undo),GINT_TO_POINTER(-1));
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Select All", "^1", "Select entire file", GTK_SIGNAL_FUNC (sel_all), NULL);
  menuitem = CreateMenuItem (menu, "Unselect All", "^2", "Unselect all", GTK_SIGNAL_FUNC (sel_un), NULL);
  menuitem = CreateMenuItem (menu, "Select even", "^3", "Select even Partials", GTK_SIGNAL_FUNC (sel_even), NULL);
  menuitem = CreateMenuItem (menu, "Select Odd", "^4", "Select Odd Partials", GTK_SIGNAL_FUNC (sel_odd), NULL);
  menuitem = CreateMenuItem (menu, "Invert Selection", "^5", "Invert Partial Selection", GTK_SIGNAL_FUNC (revert_sel), NULL);
  menuitem = CreateMenuItem (menu, "Smart Selection", "^6", "Select partials using rules", GTK_SIGNAL_FUNC (create_sel_dlg), NULL);
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Edit Amplitude", "^7", "Change Amplitude of Selection", GTK_SIGNAL_FUNC (edit_amp),NULL);
  menuitem = CreateMenuItem (menu, "Normalize Selection", "^8", "Reescale all amplitudes according to 1/max.amp.", GTK_SIGNAL_FUNC (normalize_amplitude), NULL);
  menuitem = CreateMenuItem (menu, "Edit Frequency", "^9", "Change Frequency of Selection", GTK_SIGNAL_FUNC (edit_freq), NULL);
    
  /* Synthesis  menu */
  menu = (GtkWidget *)CreateSubMenu (menubar, "Synthesis");
  menuitem = CreateMenuItem (menu, "Parameters", "^P", "Set Synthesis Parameters and Synthesize", GTK_SIGNAL_FUNC (get_sparams), "Parameters");
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Synthesize", "^R", "Render Audio File from Spectral data", GTK_SIGNAL_FUNC (do_synthesis), "Synthesize");
    
  /* View menu */
  menu = (GtkWidget *)CreateSubMenu (menubar, "View");
  menuitem = CreateMenuItem (menu, "List", "^T", "View Amp., Freq. and Phase on a list", GTK_SIGNAL_FUNC (list_view), NULL);
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Sinusoidal Amplitude", "^C", "View Sinusoidal Plot with Amplitude data", GTK_SIGNAL_FUNC (set_spec_view), NULL);
  menuitem = CreateMenuItem (menu, "Sinusoidal SMR", "", "View Sinusoidal Plot with SMR data", GTK_SIGNAL_FUNC (set_smr_view), NULL);
  menuitem = CreateMenuItem (menu, "Noise", "^D", "View Noise Plot", GTK_SIGNAL_FUNC (set_res_view), NULL);
  menuitem = CreateMenuItem (menu, "Lines / Dashes", "^M", "Toggles between Interpolated or Non Interpolated Frequency Plot", GTK_SIGNAL_FUNC (set_interpolated_view), NULL);
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "unzoom", "^U", "zoom out full", GTK_SIGNAL_FUNC (unzoom), NULL);
  menuitem = CreateMenuItem (menu, "zoom selection", "^Z", "zoom selected region", GTK_SIGNAL_FUNC (zoom_sel), NULL); 
  menuitem = CreateMenuItem (menu, "all / only selection", "^W", "switch between view only selection /view all", GTK_SIGNAL_FUNC (sel_only), NULL);
  menuitem = CreateMenuItem (menu, NULL, NULL, NULL, NULL, NULL);
  menuitem = CreateMenuItem (menu, "Header Data", "^E", "view ATS file header data", GTK_SIGNAL_FUNC (show_header), NULL);
   
  /* Help */
  menu = (GtkWidget *)CreateSubMenu (menubar, "Help");
  menuitem = CreateMenuItem (menu, "Help", "^H", "View help information", GTK_SIGNAL_FUNC (help), NULL);
  menuitem = CreateMenuItem (menu, "About ATSH", "", "About ATSH", GTK_SIGNAL_FUNC (about), NULL);
}





///////////////////////////////////////////////////////////
void init_scalers(gint show)
{
  if(floaded) {
    gtk_ruler_set_range (GTK_RULER (hruler),1.,atshed.fra, 1.,atshed.fra);
    //FROM frame value
    h.viewstart=1;
    h.viewend=(int)atshed.fra;
    h.prev = h.diff = h.viewend - h.viewstart;      
    h_setup();
   
    gtk_ruler_set_range (GTK_RULER (vruler),((atshed.mf*ATSH_FREQ_OFFSET) - 1.)/1000., 
                         1., 0.,((atshed.mf*ATSH_FREQ_OFFSET )- 1.)/1000.);
    v.viewstart=1;
    v.viewend=(int)(atshed.mf*ATSH_FREQ_OFFSET);
    v.prev = v.diff = (int)(atshed.mf*ATSH_FREQ_OFFSET);
    v_setup();
  }

  if(show) {
    gtk_widget_show(GTK_WIDGET(vscale1));
    gtk_widget_show(GTK_WIDGET(vscale2));
    gtk_widget_show(GTK_WIDGET(hscale1));
    gtk_widget_show(GTK_WIDGET(hscale2));
    gtk_widget_show(GTK_WIDGET(valscale));
    gtk_widget_show(GTK_WIDGET(hruler));
    gtk_widget_show(GTK_WIDGET(vruler));
  }
  else {
    gtk_widget_hide(GTK_WIDGET(vscale1));
    gtk_widget_hide(GTK_WIDGET(vscale2));
    gtk_widget_hide(GTK_WIDGET(hscale1));
    gtk_widget_hide(GTK_WIDGET(hscale2));
    gtk_widget_hide(GTK_WIDGET(valscale));
    gtk_widget_hide(GTK_WIDGET(hruler));
    gtk_widget_hide(GTK_WIDGET(vruler));
  }
}
///////////////////////////////////////////////////////////////////////////////// 
void h_scroll(GtkObject *adj, gpointer data)
{ 
  int whichscroll=GPOINTER_TO_INT(data); 
  
  switch(whichscroll) {
  case 1:
    {
      h.viewstart  = (int)GTK_ADJUSTMENT(hadj1)->value;
      h.viewend    = h.viewstart + ((int)GTK_ADJUSTMENT(hadj2)->value)-1;  
      h.diff       = h.viewend - h.viewstart;      
      
      GTK_ADJUSTMENT(hadj2)->upper= (atshed.fra - GTK_ADJUSTMENT(hadj1)->value) + 1;
      gtk_adjustment_changed(GTK_ADJUSTMENT(hadj2));
      set_hruler((double)h.viewstart, (double)h.viewend, (double)h.viewstart, (double)h.diff);
      break;
    }
  case 2:
    {
      h.viewend    = h.viewstart +((int)GTK_ADJUSTMENT(hadj2)->value)-1;
      h.diff=h.viewend - h.viewstart;
      GTK_ADJUSTMENT(hadj1)->upper=(atshed.fra - GTK_ADJUSTMENT(hadj2)->value) + 1;
      gtk_adjustment_changed (GTK_ADJUSTMENT(hadj1));
      set_hruler((double)h.viewstart, (double)h.viewend, (double)h.viewstart, (double)h.diff);
      break;
    }
  }
  draw_pixm();
}
////////////////////////////////////////////////////////////  
void h_setup(void)
{
  //from frame view
  GTK_ADJUSTMENT(hadj1)->lower=1.;
  GTK_ADJUSTMENT(hadj1)->upper=1.;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj1),1.);
  //size of view
  GTK_ADJUSTMENT(hadj2)->lower=1.;
  GTK_ADJUSTMENT(hadj2)->upper=atshed.fra;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj2),atshed.fra);
  draw_pixm();
}
/////////////////////////////////////////////////////////////////////////////////
void v_scroll(GtkObject *adj, gpointer data)
{  
  int whichscroll=GPOINTER_TO_INT(data);

  switch(whichscroll) {
  case 1:
    
      v.viewstart  = (int)GTK_ADJUSTMENT(vadj1)->value;
      v.viewend    = (v.viewstart + ((int)GTK_ADJUSTMENT(vadj2)->value));  
      v.diff       = ((int)GTK_ADJUSTMENT(vadj2)->value);      
      
      GTK_ADJUSTMENT(vadj2)->upper= (atshed.mf*ATSH_FREQ_OFFSET) - GTK_ADJUSTMENT(vadj1)->value ;
      gtk_adjustment_changed(GTK_ADJUSTMENT(vadj2));
      if(VIEWING_DET) {
	gtk_ruler_set_range(GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
			    (float)v.viewstart/1000.,(float)v.diff/1000.);
      }
      break;
    
  case 2: 
    v.viewend    = v.viewstart +((int)GTK_ADJUSTMENT(vadj2)->value);
    v.diff       = (int)GTK_ADJUSTMENT(vadj2)->value; 
    GTK_ADJUSTMENT(vadj1)->upper=(atshed.mf*ATSH_FREQ_OFFSET) - GTK_ADJUSTMENT(vadj2)->value;
    gtk_adjustment_changed (GTK_ADJUSTMENT(vadj1));
    if(VIEWING_DET) {
      gtk_ruler_set_range (GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
			   (float)v.viewstart/1000.,(float)v.diff/1000.);
    }
    break;
  
  }
  
  draw_pixm();
}
///////////////////////////////
void v_setup()
{
//from view
  GTK_ADJUSTMENT(vadj1)->lower=1.;
  GTK_ADJUSTMENT(vadj1)->upper=1.;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj1),1.);
  //size of view
  GTK_ADJUSTMENT(vadj2)->lower=1.;
  GTK_ADJUSTMENT(vadj2)->upper=(atshed.mf* ATSH_FREQ_OFFSET)-1.;//ojo
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj2),(atshed.mf* ATSH_FREQ_OFFSET));
  draw_pixm();
}

//////////////////////////////////////////////////////////
// void set_avec(int len)
// {  //selection.to - selection.from
//   aveclen=len + 1;
  
//   avec=(float*)realloc(avec, sizeof(float) * aveclen);   
//   fvec=(float*)realloc(fvec, sizeof(float) * aveclen); 
// }
// //////////////////////////////////////////////////////////
void edit_freq(GtkWidget *widget, gpointer data)
{
  if(NOTHING_SELECTED) return;
  if(floaded && view_type!=RES_VIEW) {
    if(fWedit == NULL) fWedit=create_edit_curve(FRE_EDIT, freenv);
    //  gtk_grab_add (fWedit);
    gtk_widget_show(GTK_WIDGET(fWedit));
  }
}
//////////////////////////////////////////////////////////
void edit_amp(GtkWidget *widget, gpointer data)
{

  if(floaded==FALSE || view_type==RES_VIEW) { return; }
  if(NOTHING_SELECTED) {return;}
  
  if(aWedit == NULL) {
    aWedit=create_edit_curve(AMP_EDIT, ampenv);
  }
  gtk_grab_add (aWedit);
  gtk_widget_show(GTK_WIDGET(aWedit));

  return;
}
//////////////////////////////////////////////////////////
void edit_tim(GtkWidget *widget, gpointer data)
{     
  if(floaded==FALSE) { return; }

  if(tWedit == NULL) {
    tWedit=create_edit_curve(TIM_EDIT, timenv);
    gtk_grab_add (tWedit);
    gtk_widget_show(GTK_WIDGET(tWedit));
    return;
  }

  set_time_env(timenv, FALSE);
  gtk_grab_add (tWedit);
  gtk_widget_show(GTK_WIDGET(tWedit));

  return;
}
///////////////////////////////////////////////
void set_time_env(ENVELOPE *tim, int do_dur)
{    

  if(NOTHING_SELECTED) {
    tim->ymax=(float)ats_sound->time[0][(int)atshed.fra-1];
    tim->ymin=0.;
  }
  else {
    tim->ymax=(float)ats_sound->time[0][selection.to];
    tim->ymin=(float)ats_sound->time[0][selection.from]; 
  }
   
  if(do_dur){
    tim->dur =tim->ymax - tim->ymin;  
    *info=0;
    sprintf(info," %12.5f", tim->dur);
    gtk_entry_set_text(GTK_ENTRY (tim->durentry),info);
  }
  
  *info=0;
  sprintf(info," %12.5f",tim->ymin);
  gtk_entry_set_text(GTK_ENTRY (tim->minentry),info);
  
  *info=0;
  sprintf(info," %12.5f",tim->ymax);
  gtk_entry_set_text(GTK_ENTRY (tim->maxentry),info);    
  
  return;
}
//////////////////////////////////////////////////////////
void normalize_amplitude(GtkWidget *widget, gpointer data)
{
  int i, x, nValue=0;
  float normval= 1. / (float)atshed.ma;
  float maxamp=0.;
  int todo= aveclen * 2 * (int)atshed.par;

  if(floaded==FALSE || view_type==RES_VIEW) { return; }
  if(NOTHING_SELECTED) {return;}

  backup_edition(AMP_EDIT);
  StartProgress("Normalizing selection...",FALSE);

  //find out max amplitude of selection
  for(i=selection.from; i < selection.from+aveclen; ++i) {
    for(x=0; x < (int)atshed.par ; ++x) {
      if(ats_sound->amp[x][i] >= maxamp) {
	maxamp=ats_sound->amp[x][i];  
      }
    ++nValue;
    UpdateProgress(nValue,todo);
    }
  }

  //now do it

  normval= 1. / maxamp;
    for(i=selection.from; i < selection.from+aveclen; ++i) {
      for(x=0; x < (int)atshed.par ; ++x) {
	if(selected[x]== TRUE) {
	  ats_sound->amp[x][i]*=normval;  
	}
      ++nValue;
      UpdateProgress(nValue,todo);
      }
    }

    atshed.ma=(double)1.;

    EndProgress();
    draw_pixm();
    smr_done=FALSE;
    return;
}
//////////////////////////////////////////////////////////
void show_file_name(char *name)
{
  char str[263] = "ATSH";

  if(name != NULL) {
    strcat(str, " (");
    strcat(str, name);
    strcat(str, ")");
  }
  gtk_window_set_title(GTK_WINDOW(win_main), str);
}
