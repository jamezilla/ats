/*
ATSH.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#ifdef HAVE_CONFIG_H    //WE NEED THIS TO MAKE UNDER WINDOWS 
#  include <config.h>
#endif

#include "atsh.h"
#include "my_curve.h"

void CreateMainWindow (char *cmdl_filename);
void h_scroll(GtkObject *adj,gpointer data);
void v_scroll(GtkObject *adj,gpointer data);
void v_setup();

GtkTooltips   *tooltips;
GtkWidget     *toolbar;
GtkWidget     *main_graph;
GtkWidget     *label;
char *out_tittle;
char *out_ats_tittle;
float *sine_table;
ANARGS *atsh_anargs;
GtkObject *hadj1,*hadj2;
GtkObject *vadj1,*vadj2, *valadj;
GtkWidget *hscale1, *hscale2;
GtkWidget *vscale1, *vscale2, *valscale;
GtkWidget *hruler, *vruler;
VIEW_PAR *h, *v;
UNDO_DATA *undat;
GtkWidget **entry;
GdkPixmap *pixmap;

/*
 * EndProgram
 *
 * Exit from the program
 */
gint EndProgram ()
{
  
  free(atshed);
  free(sparams);
  free(sine_table);
  if(!entry)g_free(entry);

  //if(times) free(times);
  free(selected);
  free(info);
  free(h);
  free(v);
  free(selection);
  free(position);
  free(undat);
  free(sdata);
  free(avec);
  free(fvec);
  
  //kill UNDO file
  remove(undo_file);
  
  gdk_pixmap_unref(pixmap);
  gdk_gc_unref(draw_pen);
  stringfree();
  
  if(fWedit)
    gtk_widget_destroy (GTK_WIDGET (fWedit)); //Kill Frequency edit Window
  if(aWedit)
    gtk_widget_destroy (GTK_WIDGET (aWedit)); //Kill Amplitude edit Window
  if(tWedit)
    gtk_widget_destroy (GTK_WIDGET (tWedit)); //Kill Time edit Window
  
  free(ampenv);
  free(freenv);
  free(timenv);
  free(atsh_anargs);
  free_sound(ats_sound);
  
  /* --- End gtk event loop processing --- */
  gtk_main_quit ();
  
  /* --- Ok to close the app. --- */
  return (FALSE);
}

/*
 * main
 * 
 * --- Program begins here
 */
int main (int argc, char *argv[])
{
  unsigned int npid=0;
  char *suffix;
  

    /* --- Initialize GTK --- */
    gtk_init (&argc, &argv);
    /* --- SINE TABLE --- */ 
    
    /* initialize sndlib */
    mus_sound_initialize();

    make_sine_table();
    //////////////////
    floaded=FALSE;
    pixmap=NULL;
    view_type=NULL_VIEW;
    draw=TRUE;
    ned=0;
    led=1;
    undo=TRUE;
    h=(VIEW_PAR*)malloc(sizeof(VIEW_PAR));
    v=(VIEW_PAR*)malloc(sizeof(VIEW_PAR));
    selection=(SELECTION*)malloc(sizeof(SELECTION));
    position=(SELECTION*)malloc(sizeof(SELECTION));
    
    stringinit();
 
    strcat(out_ats_tittle,DEFAULT_OUTPUT_ATS_FILE);
    
    avec=(float*)malloc(sizeof(float));
    fvec=(float*)malloc(sizeof(float));
    ats_sound = NULL;
    //    ats_sound=(ATS_SOUND*)malloc(sizeof(ATS_SOUND));
    //    init_sound(ats_sound, 44100 , 2205, 2205, 1,.05,1);

    selected=(short*)malloc(sizeof(short));
    info=(char*)malloc(sizeof(char)*1024);


    //set backup file name
    suffix=(char*)malloc(20*sizeof(char));
    npid=getpid();
    sprintf(suffix,"_%u",npid);
    strcat(undo_file,UNDO_FILE);
    strcat(undo_file,suffix);
    free(suffix);   
    
    entry=(GtkWidget**)g_malloc(sizeof(GtkWidget*)*6);
    /* --- Initialize tooltips. --- */
    tooltips = gtk_tooltips_new();
    /* --- allocate memory for global data --- */
    atshed =(ATS_HEADER*)malloc(sizeof(ATS_HEADER));
    sparams=(SPARAMS*)malloc(sizeof(SPARAMS));
    undat =(UNDO_DATA*)malloc(sizeof(UNDO_DATA));

    ampenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
    freenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
    timenv=(ENVELOPE*)malloc(sizeof(ENVELOPE));
    sdata= (SMSEL_DATA*)malloc(sizeof(SMSEL_DATA));


    //Arbitrary initialization of synthesis data...
    sparams->sr  = 44100.; 
    sparams->amp = 1.;
    sparams->ramp = 1.; 
    sparams->frec = 1.;
    sparams->max_stretch= 1.;
    sparams->beg = 0.;
    sparams->end = 0.;
    sparams->allorsel=FALSE;
    
    vertex1=FALSE;
    vertex2=FALSE;
    outype =WAV16;

    depth=MAX_COLOR_VALUE;
    interpolated=TRUE;
    need_byte_swap=FALSE; 

    *out_tittle=*res_tittle=0;
    strcat(out_tittle, DEFAULT_OUTPUT_FILE);
    strcat(res_tittle, DEFAULT_OUTPUT_RES_FILE);
    *apf_tittle=0;

    atsh_anargs=(ANARGS*)malloc(sizeof(ANARGS));

    // default values for analysis args
    atsh_anargs->start = ATSA_START; 
    atsh_anargs->duration = ATSA_DUR;
    atsh_anargs->lowest_freq = ATSA_LFREQ; 
    atsh_anargs->highest_freq = ATSA_HFREQ; 
    atsh_anargs->freq_dev = ATSA_FREQDEV; 
    atsh_anargs->win_cycles = ATSA_WCYCLES; 
    atsh_anargs->win_type = ATSA_WTYPE; 
    atsh_anargs->hop_size = ATSA_HSIZE; 
    atsh_anargs->lowest_mag = ATSA_LMAG;
    atsh_anargs->track_len = ATSA_TRKLEN; 
    atsh_anargs->min_seg_len = ATSA_MSEGLEN; 
    atsh_anargs->min_gap_len = ATSA_MGAPLEN; 
    atsh_anargs->SMR_thres = ATSA_SMRTHRES; 
    atsh_anargs->min_seg_SMR = ATSA_MSEGSMR; 
    atsh_anargs->last_peak_cont = ATSA_LPKCONT; 
    atsh_anargs->SMR_cont = ATSA_SMRCONT ; 
    atsh_anargs->type=ATSA_TYPE;

    /* --- Create the window with menus/toolbars. --- */
    if(argc ==2) {   //see if we have an ats file delivered by command line
	CreateMainWindow (argv[1]); //pass the filename to main function
    }
    else {
	CreateMainWindow (NULL); //no filename delivered by command line
    }
    gtk_main();
    return 0;
}
/*
 * PrintFunc
 * 
 */
void PrintFunc (GtkWidget *widget, gpointer data)
{
  //g_print ("%s\n", data);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
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
    gtk_widget_set_usize(win_main, GRAPH_W, GRAPH_H);
    gtk_container_border_width (GTK_CONTAINER (win_main),0);
    gtk_window_set_position(GTK_WINDOW(win_main),GTK_WIN_POS_CENTER);
    //the user can shrink the window, and/or enlarge it...
    gtk_window_set_policy (GTK_WINDOW (win_main), TRUE, TRUE, FALSE);
   
    /* --- Top level window should listen for the delete_event --- */
    gtk_signal_connect (GTK_OBJECT (win_main), "delete_event",GTK_SIGNAL_FUNC (EndProgram), NULL);
    /* --- Vbox and Hbox --- */
    hbox_main = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox_main);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "hbox_main", hbox_main,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox_main);
    gtk_container_add (GTK_CONTAINER (win_main), hbox_main);

     /////////////////////////////////
    vbox_main = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox_main);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "vbox_main", vbox_main,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox_main);
    gtk_box_pack_start (GTK_BOX (hbox_main), vbox_main, TRUE, TRUE, 0);
    
    ////////////////////////////////
    vbox_main2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox_main2);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "vbox_main2", vbox_main2,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox_main2);
    gtk_box_pack_start (GTK_BOX (hbox_main), vbox_main2, FALSE, FALSE,0); 
    ///////////////////////////////
    ///////////////////////////////
    hbox_main1 = gtk_hbox_new (FALSE, 0);////DO NOT CHANGE!!!
    gtk_widget_ref (hbox_main1);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "hbox_main1", hbox_main1,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox_main1);
    gtk_box_pack_start (GTK_BOX (vbox_main2), hbox_main1, FALSE, FALSE,13); ///12 DO NOT CHANGE!!!
    ///////////////////////////////
    ///////////////////////////////
    hbox_main2 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox_main2);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "hbox_main2", hbox_main2,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox_main2);
    gtk_box_pack_start (GTK_BOX (vbox_main2), hbox_main2, TRUE, TRUE, 0); 
    /////////////////////////////////
    /* --- Menu Bar --- */
    menubar = gtk_menu_bar_new ();
    gtk_box_pack_start (GTK_BOX (vbox_main), menubar, FALSE, TRUE, 0);
    gtk_widget_show (menubar);
    Create_menu(GTK_WIDGET(menubar));
/////////////main graphic area///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
     
    main_graph= gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(main_graph),(int)((float)GRAPH_W ), (int)((float)GRAPH_H ));
    gtk_widget_ref (main_graph);
    gtk_object_set_data_full (GTK_OBJECT (win_main), "main_graph", main_graph,(GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (vbox_main),main_graph, TRUE, TRUE, 0);     
    
    gtk_signal_connect(GTK_OBJECT(main_graph), "expose-event", (GtkSignalFunc) expose_event,NULL);
    gtk_signal_connect(GTK_OBJECT(main_graph), "configure_event",(GtkSignalFunc) configure_event,NULL);
    gtk_signal_connect(GTK_OBJECT(main_graph), "motion_notify_event",(GtkSignalFunc) motion_notify ,NULL);
    gtk_signal_connect(GTK_OBJECT(main_graph), "button-press-event",(GtkSignalFunc)click,NULL);

    gtk_widget_set_events(main_graph, GDK_EXPOSURE_MASK
			  | GDK_LEAVE_NOTIFY_MASK
			  | GDK_BUTTON_PRESS_MASK
			  | GDK_POINTER_MOTION_MASK
			  | GDK_POINTER_MOTION_HINT_MASK
			  | GDK_BUTTON_RELEASE_MASK);
    
    draw_col = (GdkColor *) g_malloc (sizeof (GdkColor));

/////////////scalers/////////////////////
/////////////////////////////////////////
     vadj1 = gtk_adjustment_new (0.,0.,0.,1., 1., 0.);
     vadj2 = gtk_adjustment_new (0.,0.,0.,1., 1., 0.);

     hadj1 = gtk_adjustment_new (0.,0.,0.,1., 1., 0.);
     hadj2 = gtk_adjustment_new (0.,0.,0.,1., 1., 0.);
     
     ///////////RULERS go first///////////
     hruler = gtk_hruler_new ();
     gtk_widget_ref (GTK_WIDGET(hruler));
     gtk_object_set_data_full (GTK_OBJECT (win_main), "hruler", hruler,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_box_pack_start (GTK_BOX (vbox_main), GTK_WIDGET(hruler), FALSE, FALSE, 0);
     gtk_ruler_set_range (GTK_RULER (hruler), 1., 1., 1., 1.);
     gtk_ruler_set_metric(GTK_RULER (hruler), GTK_PIXELS);

     vruler = gtk_vruler_new ();
     gtk_widget_ref (GTK_WIDGET(vruler));
     gtk_object_set_data_full (GTK_OBJECT (win_main), "vruler", vruler,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_box_pack_start (GTK_BOX (hbox_main2), GTK_WIDGET(vruler), FALSE, FALSE,0);
     //gtk_widget_set_usize (GTK_WIDGET(vruler), 20., (int)((float)GRAPH_H * .75));
     gtk_ruler_set_range (GTK_RULER (vruler), 1., 1., 1., 1.);
     gtk_ruler_set_metric(GTK_RULER (vruler), GTK_PIXELS);
     
     /************/
     hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (hadj1));
     gtk_widget_ref (hscale1);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "hscale1", hscale1,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_scale_set_digits (GTK_SCALE(hscale1),0);
     gtk_signal_connect (GTK_OBJECT(hadj1), "value_changed", GTK_SIGNAL_FUNC (h_scroll), GINT_TO_POINTER(1));
     gtk_box_pack_start (GTK_BOX (vbox_main), hscale1, FALSE, TRUE, 0);
     /************/
     hscale2 = gtk_hscale_new (GTK_ADJUSTMENT (hadj2));
     gtk_widget_ref (hscale2);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "hscale2", hscale2,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_scale_set_digits (GTK_SCALE(hscale2),0);
     gtk_signal_connect (GTK_OBJECT(hadj2), "value_changed", GTK_SIGNAL_FUNC (h_scroll),GINT_TO_POINTER(2) );
     gtk_scale_set_draw_value(GTK_SCALE(hscale2),FALSE);
	gtk_range_set_update_policy(GTK_RANGE(hscale2),GTK_UPDATE_DISCONTINUOUS);
     gtk_box_pack_start (GTK_BOX (vbox_main), hscale2, FALSE, TRUE, 0);
     
     /************/
     vscale1 = gtk_vscale_new (GTK_ADJUSTMENT (vadj1));
     gtk_widget_ref (vscale1);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "vscale1", vscale1,
			       (GtkDestroyNotify) gtk_widget_unref);
     gtk_scale_set_draw_value(GTK_SCALE(vscale1),FALSE);
     gtk_scale_set_digits (GTK_SCALE(vscale1),0);  
     gtk_signal_connect (GTK_OBJECT(vadj1), "value_changed", GTK_SIGNAL_FUNC (v_scroll),GINT_TO_POINTER(1) );   
     gtk_box_pack_start (GTK_BOX (hbox_main2), vscale1, FALSE, FALSE, 8); //FF 8

     /************/
     vscale2 = gtk_vscale_new (GTK_ADJUSTMENT (vadj2));
     gtk_widget_ref (vscale2);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "vscale2", vscale2,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_signal_connect (GTK_OBJECT(vadj2), "value_changed", GTK_SIGNAL_FUNC (v_scroll), GINT_TO_POINTER(2));
     gtk_scale_set_digits (GTK_SCALE(vscale2),0);
     gtk_scale_set_draw_value(GTK_SCALE(vscale2),FALSE);
     gtk_range_set_update_policy(GTK_RANGE(vscale2),GTK_UPDATE_DISCONTINUOUS);
     gtk_box_pack_start (GTK_BOX (hbox_main2), vscale2, FALSE, FALSE, 0); //FF8
    
     /************/
     valadj=gtk_adjustment_new(0., 0., 0., 1., 100., 0.);
     
     valscale = gtk_vscale_new (GTK_ADJUSTMENT (valadj));
     gtk_widget_ref (valscale);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "valscale", valscale,
                           (GtkDestroyNotify) gtk_widget_unref);
     gtk_box_pack_start (GTK_BOX (hbox_main2), valscale, FALSE, FALSE,20); //FF5
     gtk_scale_set_digits (GTK_SCALE (valscale), 0);
     gtk_range_set_update_policy (GTK_RANGE (valscale), GTK_UPDATE_DISCONTINUOUS);
     gtk_scale_set_draw_value(GTK_SCALE (valscale),FALSE);
     gtk_signal_connect (GTK_OBJECT(valadj), "value_changed", GTK_SIGNAL_FUNC(update_value), NULL);
     
     /////////////////////////////////
     vbox_inf = gtk_vbox_new (TRUE, 0);
     gtk_widget_ref (vbox_inf);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "vbox_inf", vbox_inf,
                            (GtkDestroyNotify) gtk_widget_unref);
     gtk_widget_show (vbox_inf);
     gtk_box_pack_start (GTK_BOX (vbox_main2), vbox_inf, FALSE, FALSE,7); //DO NOT CHANGE!!!7
     ///////////////////////////////
     label= gtk_label_new("");//("\n FRAME=  \n =\n FREQ.=  ");
     gtk_widget_ref (label);
     gtk_object_set_data_full (GTK_OBJECT (win_main), "label",label,
                           (GtkDestroyNotify) gtk_widget_unref);
     gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
     gtk_box_pack_start (GTK_BOX (vbox_inf), label, FALSE, FALSE, 0);
     /////////////////////////////////////////    
     init_scalers(FALSE);
     gtk_widget_show(main_graph);
     gtk_widget_show (win_main);
     
     draw_default();

     if(cmdl_filename) { //DO WE HAVE ANY ATS FILE NAME REQUESTED BY COMMAND LINE...???
		atsin(cmdl_filename);
     }
     else {
       show_file_name(win_main,NULL);
     }
     //check the GTK Version
     /*
     g_print("\nGTK Version(Library)= %d.%d.%d  \n", gtk_major_version, gtk_minor_version, gtk_micro_version );
     g_print("GTK Version(Headers)= %d.%d.%d  \n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION );
     */
return;
}
///////////////////////////////////////////////////////////
void init_scalers(gint how)
{
  if(floaded==TRUE) {

   gtk_ruler_set_range (GTK_RULER (hruler),1.,atshed->fra, 1.,atshed->fra);
   //FROM frame value
   h->viewstart=1;
   h->viewend=(int)atshed->fra;
   h->diff=h->viewend - h->viewstart;      
   h->prev=h->diff; 
   h_setup();
   
   gtk_ruler_set_range (GTK_RULER (vruler),(atshed->mf - 1.)/1000., 1., 0.,(atshed->mf - 1.)/1000.);
   v->viewstart=1;
   v->viewend=(int)atshed->mf;
   v->diff=(int)atshed->mf;
   v->prev=v->diff;
   v_setup();
}

 if(how==FALSE) {
   gtk_widget_hide(GTK_WIDGET(vscale1));
   gtk_widget_hide(GTK_WIDGET(vscale2));
   gtk_widget_hide(GTK_WIDGET(hscale1));
   gtk_widget_hide(GTK_WIDGET(hscale2));
   gtk_widget_hide(GTK_WIDGET(valscale));
   gtk_widget_hide(GTK_WIDGET(hruler));
   gtk_widget_hide(GTK_WIDGET(vruler));
   gtk_widget_hide(GTK_WIDGET(label));
   
  
}
 else {
   gtk_widget_show(GTK_WIDGET(vscale1));
   gtk_widget_show(GTK_WIDGET(vscale2));
   gtk_widget_show(GTK_WIDGET(hscale1));
   gtk_widget_show(GTK_WIDGET(hscale2));
   gtk_widget_show(GTK_WIDGET(valscale));
   gtk_widget_show(GTK_WIDGET(hruler));
   gtk_widget_show(GTK_WIDGET(vruler));
   gtk_widget_show(GTK_WIDGET(label));
   
}
return;
}
///////////////////////////////////////////////////////////////////////////////// 
void h_scroll(GtkObject *adj, gpointer data)
{ 
  int whichscroll=GPOINTER_TO_INT(data); 
  
  switch(whichscroll) {
  case 1:
    {
      h->viewstart  = (int)GTK_ADJUSTMENT(hadj1)->value;
      h->viewend    = h->viewstart + ((int)GTK_ADJUSTMENT(hadj2)->value)-1;  
      h->diff       = h->viewend - h->viewstart;      
      
      GTK_ADJUSTMENT(hadj2)->upper= (atshed->fra - GTK_ADJUSTMENT(hadj1)->value) + 1;
      gtk_adjustment_changed(GTK_ADJUSTMENT(hadj2));
      set_hruler((double)h->viewstart, (double)h->viewend, (double)h->viewstart, (double)h->diff);
      break;
    }
  case 2:
    {
      h->viewend    = h->viewstart +((int)GTK_ADJUSTMENT(hadj2)->value)-1;
      h->diff=h->viewend - h->viewstart;
      GTK_ADJUSTMENT(hadj1)->upper=(atshed->fra - GTK_ADJUSTMENT(hadj2)->value) + 1;
      gtk_adjustment_changed (GTK_ADJUSTMENT(hadj1));
      set_hruler((double)h->viewstart, (double)h->viewend, (double)h->viewstart, (double)h->diff);
      break;
    }
  }
  if(draw) {
  draw_pixm();
  }
  return;
}
////////////////////////////////////////////////////////////  
void h_setup()
{
  //from frame view
  draw=FALSE; 
  GTK_ADJUSTMENT(hadj1)->lower=1.;
  GTK_ADJUSTMENT(hadj1)->upper=1.;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj1),1.);
  //size of view
  GTK_ADJUSTMENT(hadj2)->lower=1.;
  GTK_ADJUSTMENT(hadj2)->upper=atshed->fra;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj2),atshed->fra);
  draw=TRUE;
  draw_pixm();
  return;
}
/////////////////////////////////////////////////////////////////////////////////
void v_scroll(GtkObject *adj, gpointer data)
{  
  int whichscroll=GPOINTER_TO_INT(data);

  switch(whichscroll) {
  case 1:
    
      v->viewstart  = (int)GTK_ADJUSTMENT(vadj1)->value;
      v->viewend    = (v->viewstart + ((int)GTK_ADJUSTMENT(vadj2)->value));  
      v->diff       = ((int)GTK_ADJUSTMENT(vadj2)->value);      
      
      GTK_ADJUSTMENT(vadj2)->upper= atshed->mf - GTK_ADJUSTMENT(vadj1)->value ;
      gtk_adjustment_changed(GTK_ADJUSTMENT(vadj2));
      if(VIEWING_DET) {
	gtk_ruler_set_range(GTK_RULER (vruler),(float)v->viewend/1000.,(float)v->viewstart/1000.,
			    (float)v->viewstart/1000.,(float)v->diff/1000.);
      }
      break;
    
  case 2: 
    v->viewend    = v->viewstart +((int)GTK_ADJUSTMENT(vadj2)->value);
    v->diff       = (int)GTK_ADJUSTMENT(vadj2)->value; 
    GTK_ADJUSTMENT(vadj1)->upper=atshed->mf - GTK_ADJUSTMENT(vadj2)->value;
    gtk_adjustment_changed (GTK_ADJUSTMENT(vadj1));
    if(VIEWING_DET) {
      gtk_ruler_set_range (GTK_RULER (vruler),(float)v->viewend/1000.,(float)v->viewstart/1000.,
			   (float)v->viewstart/1000.,(float)v->diff/1000.);
    }
    break;
  
  }
  
  if(draw) {
    draw_pixm();
  }
return;
}
///////////////////////////////
void v_setup()
{
//from view
  draw=FALSE;
  GTK_ADJUSTMENT(vadj1)->lower=1.;
  GTK_ADJUSTMENT(vadj1)->upper=1.;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj1),1.);
  //size of view
  GTK_ADJUSTMENT(vadj2)->lower=1.;
  GTK_ADJUSTMENT(vadj2)->upper=atshed->mf-1.;//ojo
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vadj2),atshed->mf);
  draw=TRUE;
  draw_pixm();
return;
}

//////////////////////////////////////////////////////////
void set_avec()
{

 aveclen=(selection->to - selection->from) + 1;

 avec=(float*)realloc(avec, sizeof(float) * aveclen);   
 fvec=(float*)realloc(fvec, sizeof(float) * aveclen); 

return;
}
//////////////////////////////////////////////////////////
void edit_freq(GtkWidget *widget, gpointer data)
{

  if(floaded==FALSE || view_type==RES_VIEW) { return; }
  if(NOTHING_SELECTED) {return;}
  
  if(fWedit == NULL) {
    fWedit=create_edit_curve(FRE_EDIT, freenv);
  }
  gtk_grab_add (fWedit);
  gtk_widget_show(GTK_WIDGET(fWedit));
  return;
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
    tim->ymax=(float)ats_sound->time[0][(int)atshed->fra-1];
    tim->ymin=0.;
  }
  else {
    tim->ymax=(float)ats_sound->time[0][selection->to];
    tim->ymin=(float)ats_sound->time[0][selection->from]; 
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
  float normval= 1. / (float)atshed->ma;
  float maxamp=0.;
  int todo= aveclen * 2 * (int)atshed->par;

  if(floaded==FALSE || view_type==RES_VIEW) { return; }
  if(NOTHING_SELECTED) {return;}

  backup_edition(AMP_EDIT);
  StartProgress("Normalizing selection...",FALSE);

  //find out max amplitude of selection
  for(i=selection->from; i < selection->from+aveclen; ++i) {
    for(x=0; x < (int)atshed->par ; ++x) {
      if(ats_sound->amp[x][i] >= maxamp) {
	maxamp=ats_sound->amp[x][i];  
      }
    ++nValue;
    UpdateProgress(nValue,todo);
    }
  }

  //now do it

  normval= 1. / maxamp;
    for(i=selection->from; i < selection->from+aveclen; ++i) {
      for(x=0; x < (int)atshed->par ; ++x) {
	if(selected[x]== TRUE) {
	  ats_sound->amp[x][i]*=normval;  
	}
      ++nValue;
      UpdateProgress(nValue,todo);
      }
    }

    atshed->ma=(double)1.;

    EndProgress();
    draw_pixm();
    return;
}
//////////////////////////////////////////////////////////
void show_file_name(GtkWidget *window,char *name)
{
  char *str;

  if(name == NULL) {
    gtk_window_set_title(GTK_WINDOW(window), "ATSH - NONE");
    
  }
  else {
    str=(char*)malloc(1200*sizeof(char));
    *str=0;
    strcat(str, "ATSH - ");
    strcat(str, name);
    gtk_window_set_title(GTK_WINDOW(window), str);
    free(str);
}
  
  return;
}
