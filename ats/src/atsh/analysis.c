
/*
ATSH-ANALYSIS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
//GtkWidget *statlab;
GtkWidget     *isfile_label;
GtkWidget     *afile_label;
GtkWidget     *rfile_label;

extern char in_title[];
extern char out_ats_title[];
extern char ats_title[];
extern char res_title[];
ANARGS anargs = {{NULL, NULL}, ATSA_START, ATSA_DUR, ATSA_LFREQ, ATSA_HFREQ, ATSA_FREQDEV, ATSA_WCYCLES, ATSA_WTYPE, ATSA_WSIZE, ATSA_HSIZE, ATSA_LMAG, ATSA_TRKLEN, ATSA_MSEGLEN, ATSA_MGAPLEN, ATSA_LPKCONT, ATSA_SMRCONT, ATSA_SMRTHRES, ATSA_MSEGSMR, ATSA_TYPE};
extern int floaded;

void update_aparameters()
{
  char str[32];
  
  sprintf(str, "%9.4f",anargs.start);
  gtk_entry_set_text(GTK_ENTRY(entrys[0]), str);      

  sprintf(str, "%9.4f",anargs.duration);
  gtk_entry_set_text(GTK_ENTRY(entrys[1]), str);

  sprintf(str, "%9.4f",anargs.lowest_freq);
  gtk_entry_set_text(GTK_ENTRY(entrys[2]),str);

  sprintf(str, "%9.4f",anargs.highest_freq); 
  gtk_entry_set_text(GTK_ENTRY(entrys[3]),str);    

  sprintf(str, "%9.4f",anargs.freq_dev);
  gtk_entry_set_text(GTK_ENTRY(entrys[4]),str);

  sprintf(str, "%9.4f",(float)anargs.win_cycles);    
  gtk_entry_set_text(GTK_ENTRY(entrys[5]),str);

  sprintf(str, "%9.4f",anargs.hop_size);  
  gtk_entry_set_text(GTK_ENTRY(entrys[6]),str);

  sprintf(str, "%9.4f",anargs.lowest_mag);  
  gtk_entry_set_text(GTK_ENTRY(entrys[7]),str);

  sprintf(str, "%d",anargs.track_len); 
  gtk_entry_set_text(GTK_ENTRY(entrys[8]),str);

  sprintf(str, "%d",anargs.min_seg_len);  
  gtk_entry_set_text(GTK_ENTRY(entrys[9]),str);

  sprintf(str, "%d",anargs.min_gap_len);  
  gtk_entry_set_text(GTK_ENTRY(entrys[10]),str);

  sprintf(str, "%9.4f",anargs.SMR_thres);
  gtk_entry_set_text(GTK_ENTRY(entrys[11]),str);

  sprintf(str, "%9.4f",anargs.min_seg_SMR);
  gtk_entry_set_text(GTK_ENTRY(entrys[12]),str);

  sprintf(str, "%9.4f",anargs.last_peak_cont);
  gtk_entry_set_text(GTK_ENTRY(entrys[13]),str);

  sprintf(str, "%9.4f",anargs.SMR_cont);
  gtk_entry_set_text(GTK_ENTRY(entrys[14]),str);    
  
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), anargs.win_type); 
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu2), anargs.type-1  ); 
}
///////////////////////////////////////////////////////
void get_ap(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "getap"); 
}
///////////////////////////////////////////////////////
void sav_ap(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "savap");
}
///////////////////////////////////////////////////////
void select_out_atsfile(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "atsoutsel");
}

///////////////////////////////////////////////////////
void select_in_soundfile(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "insel");
//   anargs.start = ATSA_START;
//   gtk_entry_set_text(GTK_ENTRY(data[0]), " 0.0000");      
//   anargs.duration = ATSA_DUR;
//   gtk_entry_set_text(GTK_ENTRY(data[1]), " 0.0000");
}
///////////////////////////////////////////////////////
void set_defaults(GtkWidget *widget, gpointer *data)
{
  anargs.start = ATSA_START;
  anargs.duration = ATSA_DUR;
  anargs.lowest_freq = ATSA_LFREQ;
  anargs.highest_freq = ATSA_HFREQ;
  anargs.freq_dev = ATSA_FREQDEV;
  anargs.win_cycles = ATSA_WCYCLES;
  anargs.hop_size = ATSA_HSIZE;
  anargs.lowest_mag = ATSA_LMAG;
  anargs.track_len = ATSA_TRKLEN;
  anargs.min_seg_len = ATSA_MSEGLEN;
  anargs.min_gap_len = ATSA_MGAPLEN;
  anargs.SMR_thres = ATSA_SMRTHRES;
  anargs.min_seg_SMR = ATSA_MSEGSMR;
  anargs.last_peak_cont = ATSA_LPKCONT;
  anargs.SMR_cont = ATSA_SMRCONT;
  anargs.win_type = ATSA_WTYPE;
  anargs.type = ATSA_TYPE;

  update_aparameters();
}

///////////////////////////////////////////////////////
void unload_ats_file()
{
  floaded=0;
  selected=(short*)realloc(selected, sizeof(short));
  //init_sound(ats_sound, 44100 , 2205, 2205, 1,.1,1);
}
///////////////////////////////////////////////////////
void change_ats_type(GtkWidget *widget, gpointer data)
{
  anargs.type=(int)GPOINTER_TO_INT(data)+1; //change here
}
///////////////////////////////////////////////////////
void change_win_type(GtkWidget *widget, gpointer data)
{
  anargs.win_type=(float)GPOINTER_TO_INT(data);
}
///////////////////////////////////////////////////////
GtkWidget *create_itemen(char *label, int ID, GtkWidget *parent, int which)
{
  GtkWidget *menu;

  menu= gtk_menu_item_new_with_label (label);
  gtk_widget_show (menu);
  
  if(which==0) 
    gtk_signal_connect (GTK_OBJECT (menu),"activate",GTK_SIGNAL_FUNC (change_win_type),GINT_TO_POINTER(ID));
  else 
    gtk_signal_connect (GTK_OBJECT (menu),"activate",GTK_SIGNAL_FUNC (change_ats_type),GINT_TO_POINTER(ID));
  gtk_menu_append (GTK_MENU (parent), menu);
  return(menu);
}
///////////////////////////////////////////////////////
void set_aparam(GtkWidget *widget, gpointer data)
{
  gchar *str = gtk_editable_get_chars(GTK_EDITABLE(widget),0,12);
  switch(GPOINTER_TO_INT(data)) {
  case 1: anargs.start=atof(str); break;
  case 2: anargs.duration=atof(str); break;
  case 3: anargs.lowest_freq= atof(str); break;
  case 4: anargs.highest_freq=atof(str); break;
  case 5: anargs.freq_dev=atof(str); break;
  case 6: anargs.win_cycles=atof(str); break;
  case 7: anargs.hop_size=atof(str); break;
  case 8: anargs.lowest_mag=atof(str); break;
  case 10: anargs.track_len=atoi(str); break;
  case 11: anargs.min_seg_len=atoi(str); break;
  case 12: anargs.min_gap_len=atoi(str); break;  
  case 13: anargs.SMR_thres=atof(str); break;
  case 14: anargs.min_seg_SMR=atof(str); break;
  case 15: anargs.last_peak_cont=atof(str); break;
  case 16: anargs.SMR_cont=atof(str); break;
  }
  g_free(str);
}
///////////////////////////////////////////////////////
void do_analysis (GtkWidget *widget, gpointer data)
{
  char previous[256];
  gchar *str;
 /* create pointers and structures */
  ATS_SOUND *sound = NULL;
  FILE *outfile;
  
  //store previous ats_file
  //  previous=(char*)malloc(1024*sizeof(char));
  //  *previous=0;
  strcpy(previous, ats_title);

  str=gtk_editable_get_chars(GTK_EDITABLE(afile_label),0,-1);
  strcpy(out_ats_title, str);
  g_free(str);

  str=gtk_editable_get_chars(GTK_EDITABLE(isfile_label),0,-1);
  strcat(in_title, str);
  g_free(str);

  str=gtk_editable_get_chars(GTK_EDITABLE(rfile_label),0,-1);
  strcat(res_title, str);
  g_free(str);


  //  retrieve_file_names();

  if(*out_ats_title==0) {
    Popup("ERROR: ATS Output file undefined, select it first");
    return; 
  }
  if(*in_title==0) {
    Popup("ERROR: Input Soundfile undefined, select it first");  
    return; 
  }     
  if(*res_title==0) {
    Popup("ERROR: Output Residual/Deterministic Soundfile undefined, select it first");  
    return; 
  }  
   
  //relase memory of previous file if any 
  unload_ats_file();

  /* open output file */
  outfile = fopen(out_ats_title, "wb");
  if (outfile == NULL) {
    Popup("Could not open analysis output file for writing:\nAnalysis aborted");
    //  free(previous);
    gtk_widget_destroy (GTK_WIDGET(data));
    return;
  }

  /* call tracker */
  fprintf(stderr, "tracking...\n");
  //gtk_label_set_text(GTK_LABEL (statlab),"ANALYZING...WAIT...");  
  sound = tracker(&anargs, in_title, res_title);

  /* save sound */
  if(sound != NULL) {
    fprintf(stderr,"done!\n");
    fprintf(stderr,"saving sound...\n");
    ats_save(sound, outfile, anargs.SMR_thres, anargs.type);
    fprintf(stderr, "done!\n");
    /* close output file */
    fclose(outfile);
    /* free ATS_SOUND memory */
    free_sound(sound);
  }
  else{
    /* file I/O error */
    Popup("Non valid Input Soundfile:\nAnalysis aborted");
    atsin(previous);
    //  free(previous);
    return;
  }
  
  //load new analysis file  
    atsin(out_ats_title);
 
  //kill window and return
    //    free(previous);
    gtk_widget_destroy (GTK_WIDGET(data));

}
//////////////////////////////////////////////////////
GtkWidget *create_label(char *winfo, int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table)
{
  GtkWidget *label = gtk_label_new (winfo);
  gtk_label_set_line_wrap(GTK_LABEL (label),TRUE);
  gtk_table_attach (GTK_TABLE (table), label, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_widget_show (label);
  return(label);
}
//////////////////////////////////////////////////////
GtkWidget *create_button(char *winfo, int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table)
{
  GtkWidget *button = gtk_button_new_with_label (winfo);
  gtk_table_attach (GTK_TABLE (table), button, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (button, 130, 32);
  gtk_widget_show (button);
  return(button);
}
//////////////////////////////////////////////////////
GtkWidget *create_entry(int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID, float value, char *strbuf, int isint, int width)
{
  GtkWidget *entry;
  char str[32] = "entry";
  int flag;

  strcat(str, ID);

  entry = gtk_entry_new ();
//   gtk_widget_ref (entry);
//   gtk_object_set_data_full (GTK_OBJECT (window), str, entry,
//                             (GtkDestroyNotify) gtk_widget_unref);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_widget_set_usize (entry, width, 22);
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  if(!strbuf) {  //is a number
    
    if(isint) sprintf(str, "%d",(int)value); //is int
    else sprintf(str, "%9.4f",value); //is a float


    gtk_entry_set_editable (GTK_ENTRY (entry),TRUE);
    gtk_entry_set_text(GTK_ENTRY (entry), str);
    flag=atoi(ID);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",GTK_SIGNAL_FUNC(set_aparam),GINT_TO_POINTER(flag));
  }
  else { //is a text
    gtk_entry_set_editable (GTK_ENTRY (entry),TRUE);
    gtk_entry_set_text(GTK_ENTRY (entry), strbuf);
  }  
  
  return(entry);
}
//////////////////////////////////////////////////////
void create_ana_dlg (void)
{
  GtkWidget *window, *table, *the_label, *the_butt, *hseparator1, *optionmenu1_menu, *optionmenu2_menu, *menuitem;

  entrys=(GtkWidget**)malloc(15*sizeof(GtkWidget*));

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "window", window);
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  gtk_window_set_title (GTK_WINDOW (window),("Analysis Parameters "));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window), 180, 180);
  gtk_window_set_policy (GTK_WINDOW (window), FALSE, FALSE, TRUE);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
			GTK_SIGNAL_FUNC (destroy_smartsel),window);

  table = gtk_table_new (16, 4, FALSE);
  gtk_widget_ref (table);
  gtk_object_set_data_full (GTK_OBJECT (window), "table", table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (window), table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

/////////LABELS////////////////////////////////////
  the_label=create_label("Start(secs.)",              0,1,0,1,window,table);
  the_label=create_label("Duration(secs.)",           0,1,1,2,window,table);
  the_label=create_label("Lowest Freq.(Hz.)",         0,1,2,3,window,table);
  the_label=create_label("Highest Freq.(Hz.)",        0,1,3,4,window,table);
  the_label=create_label("Frequency Deviation(Ratio)",0,1,4,5,window,table);
  the_label=create_label("Number of Cycles of Lowest Frequency\nto fit in Analysis Window",         
		                                      0,1,5,6,window,table);
  the_label=create_label("Hop size(ratio of Window size)",
		                                      0,1,6,7,window,table); 
  the_label=create_label("Amplitude threshold (dB)",  0,1,7,8,window,table);
 
  the_label=create_label("Window Type",               0,1,8,9,window,table);
  

  the_label=create_label("Track length(frames)",     2,3,0,1,window,table); /*int*/
  the_label=create_label("Min. Seg. length(frames)", 2,3,1,2,window,table); /*int*/
  the_label=create_label("Min. Gap length(frames)",  2,3,2,3,window,table); /*int*/
  the_label=create_label("SMR threshold(dB SPL)",    2,3,3,4,window,table); 
  the_label=create_label("Min. Seg. SMR(dB SPL)",    2,3,4,5,window,table);
  the_label=create_label("Last peak contribution",   2,3,5,6,window,table);
  the_label=create_label("SMR contribution",         2,3,6,7,window,table);
  the_label=create_label("ATS output file type",     2,3,7,8,window,table);
 
  ////////////////////////////////////////////////////////////////////////////////

  isfile_label=create_entry (1,4,12,13,window,table,"18",0.0,in_title,0,405);
  afile_label=create_entry (1,4,13,14,window,table,"19",0.0,out_ats_title,0,405);
  rfile_label=create_entry (1,4,14,15,window,table,"20",0.0,res_title,0,405);

  //statlab=create_label("READY",0,1,10,11,window,table);
/////////ENTRYS////////////////////////////////////

  entrys[0] =create_entry (1,2,0,1,window,table,"1",anargs.start,NULL,0,90);
  entrys[1] =create_entry (1,2,1,2,window,table,"2",anargs.duration,NULL,0,90);
  entrys[2] =create_entry (1,2,2,3,window,table,"3",anargs.lowest_freq,NULL,0,90);
  entrys[3] =create_entry (1,2,3,4,window,table,"4",anargs.highest_freq,NULL,0,90);
  entrys[4] =create_entry (1,2,4,5,window,table,"5",anargs.freq_dev,NULL,0,90);
  entrys[5] =create_entry(1,2,5,6,window,table, "6" ,anargs.win_cycles,NULL,0,90);
  entrys[6] =create_entry(1,2,6,7,window,table, "7" ,anargs.hop_size,NULL,0,90); 
  entrys[7] =create_entry (1,2,7,8,window,table,"8",anargs.lowest_mag,NULL,0,90);

  entrys[8] =create_entry(3,4,0,1,window,table,"10" ,anargs.track_len,NULL,1,90); /*int*/
  entrys[9] =create_entry(3,4,1,2,window,table,"11",anargs.min_seg_len,NULL,1,90); /*int*/
  entrys[10]=create_entry(3,4,2,3,window,table,"12",anargs.min_gap_len,NULL,1,90); /*int*/
  entrys[11]=create_entry(3,4,3,4,window,table,"13",anargs.SMR_thres,NULL,0,90); 
  entrys[12]=create_entry(3,4,4,5,window,table,"14",anargs.min_seg_SMR,NULL,0,90);
  entrys[13]=create_entry(3,4,5,6,window,table,"15",anargs.last_peak_cont,NULL,0,90);
  entrys[14]=create_entry(3,4,6,7,window,table,"16",anargs.SMR_cont,NULL,0,90);


/////////SEPARATORS//////////////////////////////// 
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table), hseparator1, 0, 4, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table), hseparator1, 0, 4, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);
/////////BUTTONS/////////////////////////////////// 
  the_butt=create_button("Set input Soundfile",0,1,12,13,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (select_in_soundfile),entrys);
  the_butt=create_button("Load Parameters"    ,1,2,10,11,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (get_ap),entrys);
  the_butt=create_button("Save Parameters"    ,2,3,10,11,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (sav_ap),entrys);
  the_butt=create_button("Set output ATS file",0,1,13,14,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (select_out_atsfile),entrys);
  the_butt=create_button("Set Default Values" ,3,4, 10, 11,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (set_defaults),entrys);
  the_butt=create_button("Do Analysis"        ,1,2,16,17,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (do_analysis),window);
  the_butt=create_button("Forget It"          ,2,3,16,17,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (cancel_dialog),window);
  the_butt=create_button("Set Residual Output",0,1,14,15,window,table);
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (filesel),"res_sel");
///////////////////////////////////////////////////////
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table), hseparator1, 0, 4, 15, 16,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5); 

  gtk_grab_add (window);
  gtk_widget_show (window);
////////WINDOW TYPE OPTION MENU///////////////////////////////////////
  optionmenu1 = gtk_option_menu_new ();
  gtk_widget_ref (optionmenu1);
  gtk_object_set_data_full (GTK_OBJECT (window), "optionmenu1", optionmenu1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (optionmenu1);
  gtk_table_attach (GTK_TABLE (table), optionmenu1, 1, 2, 8, 9,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 5, 0);
  
  optionmenu1_menu = gtk_menu_new ();

  menuitem = create_itemen ("Blackman",        0, optionmenu1_menu, 0);
  menuitem = create_itemen ("Blackman-Harris", 1, optionmenu1_menu, 0);
  menuitem = create_itemen ("Hamming",         2, optionmenu1_menu, 0);
  menuitem = create_itemen ("Von Hann",        3, optionmenu1_menu, 0); 

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), (int)anargs.win_type); 
  gtk_widget_set_usize (optionmenu1, 130, 30);
////////ATS FILE TYPE OPTION MENU///////////////////////////////////////
  optionmenu2 = gtk_option_menu_new ();
  gtk_widget_ref (optionmenu2);
  gtk_object_set_data_full (GTK_OBJECT (window), "optionmenu2", optionmenu2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (optionmenu2);
  gtk_table_attach (GTK_TABLE (table), optionmenu2, 3, 4, 7, 8,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 5, 0);
  
  optionmenu2_menu = gtk_menu_new ();

  menuitem = create_itemen ("type1",0, optionmenu2_menu, 1);
  menuitem = create_itemen ("type2",1, optionmenu2_menu, 1);
  menuitem = create_itemen ("type3",2, optionmenu2_menu, 1);
  menuitem = create_itemen ("type4",3, optionmenu2_menu, 1); 

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu2), optionmenu2_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu2),anargs.type-1); 
  gtk_widget_set_usize (optionmenu2, 130, 30);
}

