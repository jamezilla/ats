
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

///////////////////////////////////////////////////////
void retrieve_file_names()
{
  char *str;

  str=(char*)malloc(1024*sizeof(char));

  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(afile_label),0,-1);
  *out_ats_title =0;
  strcat(out_ats_title, str);

  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(isfile_label),0,-1);
  *in_title =0;
  strcat(in_title, str);

  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(rfile_label),0,-1);
  *res_title =0;
  strcat(res_title, str);

  free(str);
  return;
}
///////////////////////////////////////////////////////
void update_aparameters()
{
char *str=(char*)malloc(32*sizeof(char));
  
 *str=0;
  sprintf(str, " %9.4f ",anargs.start);
  gtk_entry_set_text(GTK_ENTRY(entrys[0]), str);      
  *str=0;
  sprintf(str, " %9.4f ",anargs.duration);
  gtk_entry_set_text(GTK_ENTRY(entrys[1]), str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.lowest_freq);
  gtk_entry_set_text(GTK_ENTRY(entrys[2]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.highest_freq); 
  gtk_entry_set_text(GTK_ENTRY(entrys[3]),str);    
  *str=0;
  sprintf(str, " %9.4f ",anargs.freq_dev);
  gtk_entry_set_text(GTK_ENTRY(entrys[4]),str);
  *str=0;
  sprintf(str, " %9.4f ",(float)anargs.win_cycles);    
  gtk_entry_set_text(GTK_ENTRY(entrys[5]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.hop_size);  
  gtk_entry_set_text(GTK_ENTRY(entrys[6]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.lowest_mag);  
  gtk_entry_set_text(GTK_ENTRY(entrys[7]),str);
  *str=0;
  sprintf(str, " %d ",anargs.track_len); 
  gtk_entry_set_text(GTK_ENTRY(entrys[8]),str);
  *str=0;
  sprintf(str, " %d ",anargs.min_seg_len);  
  gtk_entry_set_text(GTK_ENTRY(entrys[9]),str);
  *str=0;
  sprintf(str, " %d ",anargs.min_gap_len);  
  gtk_entry_set_text(GTK_ENTRY(entrys[10]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.SMR_thres);
  gtk_entry_set_text(GTK_ENTRY(entrys[11]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.min_seg_SMR);
  gtk_entry_set_text(GTK_ENTRY(entrys[12]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.last_peak_cont);
  gtk_entry_set_text(GTK_ENTRY(entrys[13]),str);
  *str=0;
  sprintf(str, " %9.4f ",anargs.SMR_cont);
  gtk_entry_set_text(GTK_ENTRY(entrys[14]),str);    
  
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), anargs.win_type); 
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu2), anargs.type-1  ); 
  
  free(str);
  return;
}
///////////////////////////////////////////////////////
void get_ap(GtkWidget *widget, gpointer *data)
{
  
  filesel(NULL, "getap"); 
  return;
}
///////////////////////////////////////////////////////
void sav_ap(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "savap");

  return; 
}
///////////////////////////////////////////////////////
void select_out_atsfile(GtkWidget *widget, gpointer *data)
{

  filesel(NULL, "atsoutsel");

  return;
}

///////////////////////////////////////////////////////
void select_in_soundfile(GtkWidget *widget, gpointer *data)
{
  filesel(NULL, "insel");
  anargs.start          =0.0;
  gtk_entry_set_text(GTK_ENTRY(data[0]), " 0.0000");      
  anargs.duration       =0.0;
  gtk_entry_set_text(GTK_ENTRY(data[1]), " 0.0000");
  return;
}
///////////////////////////////////////////////////////
void set_defaults(GtkWidget *widget, gpointer *data)
{

    anargs.start          =0.0;
    gtk_entry_set_text(GTK_ENTRY(data[0]), " 0.0000");      
    anargs.duration       =0.0;
    gtk_entry_set_text(GTK_ENTRY(data[1]), " 0.0000");   
    anargs.lowest_freq    =20.;
    gtk_entry_set_text(GTK_ENTRY(data[2]), " 20.0000");   
    anargs.highest_freq   =20000.;
    gtk_entry_set_text(GTK_ENTRY(data[3]), " 20000.0000");    
    anargs.freq_dev       =0.1;
    gtk_entry_set_text(GTK_ENTRY(data[4]), " 0.1000");    
    anargs.win_cycles     =4.0;
    gtk_entry_set_text(GTK_ENTRY(data[5]), " 4.0000");    
    anargs.hop_size       =0.25;
    gtk_entry_set_text(GTK_ENTRY(data[6]), " 0.2500");    
    anargs.lowest_mag     =-60.000;
    gtk_entry_set_text(GTK_ENTRY(data[7]), " -60.00");   
    anargs.track_len      =3.0;
    gtk_entry_set_text(GTK_ENTRY(data[8]), " 3 ");    
    anargs.min_seg_len    =3.0;
    gtk_entry_set_text(GTK_ENTRY(data[9]), " 3 ");    
    anargs.min_gap_len    =3.0;
    gtk_entry_set_text(GTK_ENTRY(data[10]), " 3 ");    
    anargs.SMR_thres      =30.0;
    gtk_entry_set_text(GTK_ENTRY(data[11]), " 30.0000");   
    anargs.min_seg_SMR    =60.0;
    gtk_entry_set_text(GTK_ENTRY(data[12]), " 60.0000");    
    anargs.last_peak_cont =0.0;
    gtk_entry_set_text(GTK_ENTRY(data[13]), " 0.0000");   
    anargs.SMR_cont       =0.0;
    gtk_entry_set_text(GTK_ENTRY(data[14]), " 0.0000");    
    
    gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), 1); 
    anargs.win_type       =BLACKMAN_H;
    gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu2), 3); 
    anargs.type           =4;
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
  int menu;
  menu=GPOINTER_TO_INT(data); 
  anargs.type=(int)menu+1; //change here
}
///////////////////////////////////////////////////////
void change_win_type(GtkWidget *widget, gpointer data)
{
  int menu;
  menu=GPOINTER_TO_INT(data); 
  anargs.win_type=(float)menu;
}
///////////////////////////////////////////////////////
GtkWidget *create_itemen(char *label, int ID, GtkWidget *parent, int which)
{
  GtkWidget *menu;

  menu= gtk_menu_item_new_with_label (label);
  gtk_widget_show (menu);
  
  if(which==0) {
    gtk_signal_connect (GTK_OBJECT (menu),
			"activate",GTK_SIGNAL_FUNC (change_win_type),GINT_TO_POINTER(ID));
  }
  else {
    gtk_signal_connect (GTK_OBJECT (menu),
			"activate",GTK_SIGNAL_FUNC (change_ats_type),GINT_TO_POINTER(ID));
  }
  gtk_menu_append (GTK_MENU (parent), menu);
  return(menu);
}
///////////////////////////////////////////////////////
void set_aparam(GtkWidget *widget, gpointer data)
{
  char *str;
  int whichparam;

  str=(char*)malloc(32*sizeof(char));
  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(widget),0,12);
  
  whichparam=GPOINTER_TO_INT(data); 

  switch(whichparam) {

  case 1:
    anargs.start=atof(str);
    break;
  case 2:  
    anargs.duration=atof(str);
    break;
  case 3:  
    anargs.lowest_freq= atof(str);
    break;
  case 4:
    anargs.highest_freq=atof(str);
    break;
  case 5:  
    anargs.freq_dev=atof(str);
    break;
  case 6:
    anargs.win_cycles=atof(str);
    break;
  case 7:
    anargs.hop_size=atof(str);
    break;
  case 8:
    anargs.lowest_mag=atof(str);
    break;
  case 10:
    anargs.track_len=atoi(str);
    break;
  case 11:
    anargs.min_seg_len=atoi(str);
    break;
  case 12:
    anargs.min_gap_len=atoi(str);
    break;  
  case 13:
    anargs.SMR_thres=atof(str);
    break;
  case 14:
    anargs.min_seg_SMR=atof(str);
    break;
  case 15:
    anargs.last_peak_cont=atof(str);
    break;
  case 16:
    anargs.SMR_cont=atof(str);
    break;
  }

  free(str);
  return;
}
///////////////////////////////////////////////////////
void do_analysis (GtkWidget *widget, gpointer data)
{
  char *previous;
 /* create pointers and structures */
  ATS_SOUND *sound = NULL;
  FILE *outfile;
  
  //store previous ats_file
  previous=(char*)malloc(1024*sizeof(char));
  *previous=0;
  strcat(previous, ats_title);

  retrieve_file_names();

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
    free(previous);
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
    free(previous);
    return;
  }
  
  //load new analysis file  
    atsin(out_ats_title);
 
  //kill window and return
    free(previous);
    gtk_widget_destroy (GTK_WIDGET(data));
    return;

}
///////////////////////////////////////////////////////
void destroy_wanalysis (GtkWidget *widget, gpointer data)
{
  retrieve_file_names();
  gtk_widget_destroy (GTK_WIDGET (data));
  return;

}
//////////////////////////////////////////////////////
GtkWidget *create_label(char *winfo, int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID)
{
  GtkWidget *label;
  char *str;
  str=(char*)malloc(32*sizeof(char));
  
  *str=0;
  strcat(str, "label");
  strcat(str, ID);
  
  label = gtk_label_new (NULL);
  gtk_widget_ref (label);
  gtk_object_set_data_full (GTK_OBJECT (window), str, label,
                            (GtkDestroyNotify) gtk_widget_unref); 
 //gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL (label),TRUE);
  gtk_label_set_text(GTK_LABEL (label),winfo);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
 
  free(str);
  return(label);
}
//////////////////////////////////////////////////////
GtkWidget *create_button(char *winfo, int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID)
{
  GtkWidget *button;
  char *str;
  str=(char*)malloc(32*sizeof(char));
  
  *str=0;
  strcat(str, "button");
  strcat(str, ID);
  
  button = gtk_button_new_with_label (winfo);
  gtk_widget_ref (button);
  gtk_object_set_data_full (GTK_OBJECT (window), str, button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (button, 130, 32);
free(str);
return(button);
}
//////////////////////////////////////////////////////
GtkWidget *create_entry(int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID, float value, char *strbuf, int isint, int width)
{
  GtkWidget *entry;
  char *str;
  int flag;

  str=(char*)malloc(32*sizeof(char));
  
  *str=0;
  strcat(str, "entry");
  strcat(str, ID);

  entry = gtk_entry_new ();
  gtk_widget_ref (entry);
  gtk_object_set_data_full (GTK_OBJECT (window), str, entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_widget_set_usize (entry, width, 22);
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  if(!strbuf) {  //is a number
    
    *str=0;
    if(isint==0) { //is a float
      sprintf(str, " %9.4f ",value);
    }
    else { //is int
      sprintf(str, " %d ",(int)value);
    }
    gtk_entry_set_editable (GTK_ENTRY (entry),TRUE);
    gtk_entry_set_text(GTK_ENTRY (entry), str);
    flag=atoi(ID);
    gtk_signal_connect (GTK_OBJECT (entry), "changed",GTK_SIGNAL_FUNC(set_aparam),GINT_TO_POINTER(flag));
  }
  else { //is a text
    gtk_entry_set_editable (GTK_ENTRY (entry),TRUE);
    gtk_entry_set_text(GTK_ENTRY (entry), strbuf);
  }  
  
  free(str);
  return(entry);
}
//////////////////////////////////////////////////////
void create_ana_dlg (void)
{
  GtkWidget *window1;
  GtkWidget *table1;
////////////////////
  GtkWidget *the_label, *the_butt;
///////////////////
  GtkWidget *hseparator1;
//////////////////
  GtkWidget *optionmenu1_menu, *optionmenu2_menu;
  GtkWidget *menuitem;
//////////////////
  entrys=(GtkWidget**)malloc(15*sizeof(GtkWidget*));

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_container_set_border_width (GTK_CONTAINER (window1), 10);
  gtk_window_set_title (GTK_WINDOW (window1),("Analysis Parameters "));
  gtk_window_set_position (GTK_WINDOW (window1), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window1), 180, 180);
  gtk_window_set_policy (GTK_WINDOW (window1), FALSE, FALSE, TRUE);
  gtk_signal_connect (GTK_OBJECT (window1), "destroy",
			GTK_SIGNAL_FUNC (destroy_smartsel),window1);

  table1 = gtk_table_new (16, 4, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (window1), table1);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);

/////////LABELS////////////////////////////////////
  the_label=create_label("Start(secs.)",              0,1,0,1,window1,table1,"1");
  the_label=create_label("Duration(secs.)",           0,1,1,2,window1,table1,"2");
  the_label=create_label("Lowest Freq.(Hz.)",         0,1,2,3,window1,table1,"3");
  the_label=create_label("Highest Freq.(Hz.)",        0,1,3,4,window1,table1,"4");
  the_label=create_label("Frequency Deviation(Ratio)",0,1,4,5,window1,table1,"5");
  the_label=create_label("Number of Cycles of Lowest Frequency\nto fit in Analysis Window",         
		                                      0,1,5,6,window1,table1,"6");
  the_label=create_label("Hop size(ratio of Window size)",
		                                      0,1,6,7,window1,table1,"7"); 
  the_label=create_label("Amplitude threshold (dB)",  0,1,7,8,window1,table1,"8");
 
  the_label=create_label("Window Type",               0,1,8,9,window1,table1,"9");
  

  the_label=create_label("Track length(frames)",     2,3,0,1,window1,table1,"10"); /*int*/
  the_label=create_label("Min. Seg. length(frames)", 2,3,1,2,window1,table1,"11"); /*int*/
  the_label=create_label("Min. Gap length(frames)",  2,3,2,3,window1,table1,"12"); /*int*/
  the_label=create_label("SMR threshold(dB SPL)",    2,3,3,4,window1,table1,"13"); 
  the_label=create_label("Min. Seg. SMR(dB SPL)",    2,3,4,5,window1,table1,"14");
  the_label=create_label("Last peak contribution",   2,3,5,6,window1,table1,"15");
  the_label=create_label("SMR contribution",         2,3,6,7,window1,table1,"16");
  the_label=create_label("ATS output file type",     2,3,7,8,window1,table1,"17");
 
  ////////////////////////////////////////////////////////////////////////////////

  isfile_label=create_entry (1,4,12,13,window1,table1,"18",0.0,in_title,0,405);
  afile_label=create_entry (1,4,13,14,window1,table1,"19",0.0,out_ats_title,0,405);
  rfile_label=create_entry (1,4,14,15,window1,table1,"20",0.0,res_title,0,405);

  //statlab=create_label("READY",0,1,10,11,window1,table1,"18");
/////////ENTRYS////////////////////////////////////

  entrys[0] =create_entry (1,2,0,1,window1,table1,"1",anargs.start,NULL,0,90);
  entrys[1] =create_entry (1,2,1,2,window1,table1,"2",anargs.duration,NULL,0,90);
  entrys[2] =create_entry (1,2,2,3,window1,table1,"3",anargs.lowest_freq,NULL,0,90);
  entrys[3] =create_entry (1,2,3,4,window1,table1,"4",anargs.highest_freq,NULL,0,90);
  entrys[4] =create_entry (1,2,4,5,window1,table1,"5",anargs.freq_dev,NULL,0,90);
  entrys[5] =create_entry(1,2,5,6,window1,table1, "6" ,anargs.win_cycles,NULL,0,90);
  entrys[6] =create_entry(1,2,6,7,window1,table1, "7" ,anargs.hop_size,NULL,0,90); 
  entrys[7] =create_entry (1,2,7,8,window1,table1,"8",anargs.lowest_mag,NULL,0,90);

  entrys[8] =create_entry(3,4,0,1,window1,table1,"10" ,anargs.track_len,NULL,1,90); /*int*/
  entrys[9] =create_entry(3,4,1,2,window1,table1,"11",anargs.min_seg_len,NULL,1,90); /*int*/
  entrys[10]=create_entry(3,4,2,3,window1,table1,"12",anargs.min_gap_len,NULL,1,90); /*int*/
  entrys[11]=create_entry(3,4,3,4,window1,table1,"13",anargs.SMR_thres,NULL,0,90); 
  entrys[12]=create_entry(3,4,4,5,window1,table1,"14",anargs.min_seg_SMR,NULL,0,90);
  entrys[13]=create_entry(3,4,5,6,window1,table1,"15",anargs.last_peak_cont,NULL,0,90);
  entrys[14]=create_entry(3,4,6,7,window1,table1,"16",anargs.SMR_cont,NULL,0,90);


/////////SEPARATORS//////////////////////////////// 
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 4, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 4, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);
/////////BUTTONS/////////////////////////////////// 
  the_butt=create_button("Set input Soundfile",0,1,12,13,window1,table1,"1");
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (select_in_soundfile),entrys);
  the_butt=create_button("Load Parameters"    ,1,2,10,11,window1,table1,"2");
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (get_ap),entrys);
  the_butt=create_button("Save Parameters"    ,2,3,10,11,window1,table1,"3");
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (sav_ap),entrys);
  the_butt=create_button("Set output ATS file",0,1,13,14,window1,table1,"4");
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (select_out_atsfile),entrys);
  the_butt=create_button("Set Default Values" ,3,4, 10, 11,window1,table1,"5");
   gtk_signal_connect (GTK_OBJECT (the_butt),"clicked",GTK_SIGNAL_FUNC (set_defaults),entrys);
  the_butt=create_button("Do Analysis"        ,1,2,16,17,window1,table1,"6");
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (do_analysis),window1);
  the_butt=create_button("Forget It"          ,2,3,16,17,window1,table1,"7");
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (destroy_wanalysis),window1);
  the_butt=create_button("Set Residual Output",0,1,14,15,window1,table1,"8");
   gtk_signal_connect (GTK_OBJECT (the_butt) , "clicked",GTK_SIGNAL_FUNC (filesel),"res_sel");
///////////////////////////////////////////////////////
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 4, 15, 16,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5); 

  gtk_grab_add (window1);
  gtk_widget_show (window1);
////////WINDOW TYPE OPTION MENU///////////////////////////////////////
  optionmenu1 = gtk_option_menu_new ();
  gtk_widget_ref (optionmenu1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "optionmenu1", optionmenu1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (optionmenu1);
  gtk_table_attach (GTK_TABLE (table1), optionmenu1, 1, 2, 8, 9,
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
  gtk_object_set_data_full (GTK_OBJECT (window1), "optionmenu2", optionmenu2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (optionmenu2);
  gtk_table_attach (GTK_TABLE (table1), optionmenu2, 3, 4, 7, 8,
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

  return;
}

