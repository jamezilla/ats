
/*
ATSH-ANALYSIS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
//GtkWidget *statlab;
GtkWidget *isfile_label;
GtkWidget *afile_label;
GtkWidget *rfile_label;
GtkWidget *entrys[15];
GtkWidget *win_menu, *type_menu;

extern char in_title[];
extern char out_ats_title[];
extern char ats_title[];
extern char res_title[];
ANARGS anargs = {{NULL, NULL}, ATSA_START, ATSA_DUR, ATSA_LFREQ, ATSA_HFREQ, ATSA_FREQDEV, ATSA_WCYCLES, ATSA_WTYPE, ATSA_WSIZE, ATSA_HSIZE, ATSA_LMAG, ATSA_TRKLEN, ATSA_MSEGLEN, ATSA_MGAPLEN, ATSA_LPKCONT, ATSA_SMRCONT, ATSA_SMRTHRES, ATSA_MSEGSMR, ATSA_TYPE};
extern int floaded;

void update_aparameters(void)
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
  
  gtk_option_menu_set_history (GTK_OPTION_MENU (win_menu), anargs.win_type); 
  gtk_option_menu_set_history (GTK_OPTION_MENU (type_menu), anargs.type-1  ); 
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
///////////////////////////////////////////////////////
// void set_aparam(GtkWidget *widget, gpointer data)
// {
//   gchar *str = gtk_editable_get_chars(GTK_EDITABLE(widget),0,12);
//   switch(GPOINTER_TO_INT(data)) {
//   case 1: anargs.start=atof(str); break;
//   case 2: anargs.duration=atof(str); break;
//   case 3: anargs.lowest_freq= atof(str); break;
//   case 4: anargs.highest_freq=atof(str); break;
//   case 5: anargs.freq_dev=atof(str); break;
//   case 6: anargs.win_cycles=atof(str); break;
//   case 7: anargs.hop_size=atof(str); break;
//   case 8: anargs.lowest_mag=atof(str); break;
//   case 10: anargs.track_len=atoi(str); break;
//   case 11: anargs.min_seg_len=atoi(str); break;
//   case 12: anargs.min_gap_len=atoi(str); break;  
//   case 13: anargs.SMR_thres=atof(str); break;
//   case 14: anargs.min_seg_SMR=atof(str); break;
//   case 15: anargs.last_peak_cont=atof(str); break;
//   case 16: anargs.SMR_cont=atof(str); break;
//   }
//   g_free(str);
// }

void get_aparam(void)
{
  gchar *str;
  
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[0]),0,12);
  anargs.start=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[1]),0,12);
  anargs.duration=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[2]),0,12);
  anargs.lowest_freq= atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[3]),0,12);
  anargs.highest_freq=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[4]),0,12);
  anargs.freq_dev=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[5]),0,12);
  anargs.win_cycles=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[6]),0,12);
  anargs.hop_size=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[7]),0,12);
  anargs.lowest_mag=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[8]),0,12);
  anargs.track_len=atoi(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[9]),0,12);
  anargs.min_seg_len=atoi(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[10]),0,12);
  anargs.min_gap_len=atoi(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[11]),0,12);
  anargs.SMR_thres=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[12]),0,12);
  anargs.min_seg_SMR=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[13]),0,12);
  anargs.last_peak_cont=atof(str);
  g_free(str);
  str = gtk_editable_get_chars(GTK_EDITABLE(entrys[14]),0,12);
  anargs.SMR_cont=atof(str);
  g_free(str);
}


///////////////////////////////////////////////////////
void do_analysis (GtkWidget *widget, gpointer data)
{
  char previous[256];
  //  gchar *str;
  int frame_n;
  ATS_SOUND *sound = NULL;
  FILE *outfile;
  
  //store previous ats_file
  //  previous=(char*)malloc(1024*sizeof(char));
  //  *previous=0;
  strcpy(previous, ats_title);

  //  out_ats_title[]=(char*)gtk_editable_get_chars(GTK_EDITABLE(afile_label),0,-1);
  strcpy(out_ats_title, (char*)gtk_editable_get_chars(GTK_EDITABLE(afile_label),0,-1));
//   g_free(str);

//  in_title[]=(char*)gtk_editable_get_chars(GTK_EDITABLE(isfile_label),0,-1);
  strcpy(in_title, (char*)gtk_editable_get_chars(GTK_EDITABLE(isfile_label),0,-1));
//   g_free(str);

//  res_title[]=(char*)gtk_editable_get_chars(GTK_EDITABLE(rfile_label),0,-1);
  strcpy(res_title, (char*)gtk_editable_get_chars(GTK_EDITABLE(rfile_label),0,-1));
//   g_free(str);


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
  get_aparam();
  //relase memory of previous file if any 
  unload_ats_file();

  gtk_widget_destroy (GTK_WIDGET(data));

  /* open output file */
  outfile = fopen(out_ats_title, "wb");
  if (outfile == NULL) {
    Popup("Could not open analysis output file for writing:\nAnalysis aborted");
    //  free(previous);
    //gtk_widget_destroy (GTK_WIDGET(data));
    return;
  }

  /* call tracker */
  fprintf(stderr, "tracking...\n");

  tracker_init (&anargs, in_title);
  StartProgress("Tracking Spundfile...",FALSE);
  for (frame_n=0; frame_n<anargs.frames; frame_n++) {
    tracker(&anargs, frame_n);
    UpdateProgress(frame_n+1, anargs.frames);
  }
  sound = tracker_finish(&anargs, res_title);
  EndProgress();

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
  } else {
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
    //gtk_widget_destroy (GTK_WIDGET(data));

}

void create_label(char *text, int col, int row, GtkWidget *table)
{
  GtkWidget *label = gtk_label_new(text);
  gtk_table_attach(GTK_TABLE(table), label, col-1, col, row-1, row, 0, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);
}


void create_button(char *text, int col, int row, GtkWidget *table, GCallback func, gpointer func_data)
{
  GtkWidget *button = gtk_button_new_with_label(text);
  gtk_table_attach(GTK_TABLE(table), button, col-1, col, row-1, row, 0, 0, 0, 0);
  gtk_widget_set_size_request (button, 130, 32);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(func), func_data);
  gtk_widget_show (button);
}


void create_menuitem(char *label, int ID, GtkWidget *parent, GCallback func)
{
  GtkWidget *menu = gtk_menu_item_new_with_label(label);
  gtk_widget_show (menu);
  g_signal_connect(G_OBJECT (menu),"activate",G_CALLBACK(func),GINT_TO_POINTER(ID));
  gtk_menu_append(GTK_MENU(parent), menu);
}


GtkWidget *create_text_entry(int p1,int p2,int p3,int p4, GtkWidget *window ,GtkWidget *table, char *ID, float value, char *strbuf, int isint, int width)
{
  GtkWidget *entry = gtk_entry_new ();;
  char str[32] = "entry";
  //  int flag;

  strcat(str, ID);

  //  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_widget_set_size_request (entry, width, 22);
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_entry_set_text(GTK_ENTRY (entry), strbuf);
  
  return(entry);
}

GtkWidget *create_entry(int col, int row, GtkWidget *table, int ID)
{
  GtkWidget *entry = gtk_entry_new ();
  //  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_widget_set_size_request (entry, 90, 22);
  gtk_table_attach (GTK_TABLE (table), entry, col-1, col, row-1, row, 0, GTK_EXPAND, 0, 0);
  //  g_signal_connect(G_OBJECT(entry), "changed",G_CALLBACK(set_aparam),GINT_TO_POINTER(ID));
  gtk_widget_show (entry);
  return(entry);
}



//////////////////////////////////////////////////////
void create_ana_dlg (void)
{
  GtkWidget *window, *table, *button, *hseparator1, *optionmenu;

  window = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(window), "Analysis Parameters");
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  //hhmmm, do we need this?
  gtk_window_set_default_size (GTK_WINDOW (window), 180, 180);

  /* table creation */
  table = gtk_table_new (16, 4, FALSE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);

  /* LABELS */
  create_label("Start(secs.)",              1,1, table);
  create_label("Duration(secs.)",           1,2, table);
  create_label("Lowest Freq.(Hz.)",         1,3, table);
  create_label("Highest Freq.(Hz.)",        1,4, table);
  create_label("Frequency Deviation(Ratio)",1,5, table);
  create_label("Lowest Frequency Cycles",   1,6, table);
  create_label("Hop size(ratio)",           1,7, table); 
  create_label("Amplitude threshold (dB)",  1,8, table);
  create_label("Window Type",               1,9, table);
  create_label("Track length(frames)",     3,1, table);
  create_label("Min. Seg. length(frames)", 3,2, table);
  create_label("Min. Gap length(frames)",  3,3, table);
  create_label("SMR threshold(dB SPL)",    3,4, table); 
  create_label("Min. Seg. SMR(dB SPL)",    3,5, table);
  create_label("Last peak contribution",   3,6, table);
  create_label("SMR contribution",         3,7, table);
  create_label("ATS file type", 3,8, table);
 
  isfile_label=create_text_entry (1,4,12,13,window,table,"18",0.0,in_title,0,405);
  afile_label=create_text_entry (1,4,13,14,window,table,"19",0.0,out_ats_title,0,405);
  rfile_label=create_text_entry (1,4,14,15,window,table,"20",0.0,res_title,0,405);

  /* entrys */
  entrys[0] =create_entry(2, 1, table, 1);
  entrys[1] =create_entry(2, 2, table, 2);
  entrys[2] =create_entry(2, 3, table, 3);
  entrys[3] =create_entry(2, 4, table, 4);
  entrys[4] =create_entry(2, 5, table, 5);
  entrys[5] =create_entry(2, 6, table, 6);
  entrys[6] =create_entry(2, 7, table, 7);
  entrys[7] =create_entry(2, 8, table, 8);
  entrys[8] =create_entry(4, 1, table,10);
  entrys[9] =create_entry(4, 2, table,11);
  entrys[10]=create_entry(4, 3, table,12);
  entrys[11]=create_entry(4, 4, table,13);
  entrys[12]=create_entry(4, 5, table,14);
  entrys[13]=create_entry(4, 6, table,15);
  entrys[14]=create_entry(4, 7, table,16);

  /* separators */
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table), hseparator1, 0, 4, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);
  hseparator1 = gtk_hseparator_new ();
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table), hseparator1, 0, 4, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 5);

  /* buttons */
  create_button("Set input Soundfile",1,13,table, G_CALLBACK(select_in_soundfile), entrys);
  create_button("Load Parameters"    ,2,11,table, G_CALLBACK(get_ap), entrys);
  create_button("Save Parameters"    ,3,11,table, G_CALLBACK(sav_ap), entrys);
  create_button("Set output ATS file",1,14,table, G_CALLBACK(select_out_atsfile), entrys);
  create_button("Default Parameters" ,4,11,table, G_CALLBACK(set_defaults), entrys);
  create_button("Set Residual Output",1,15,table, G_CALLBACK(filesel), "res_sel");
   
  /* WINDOW TYPE OPTION MENU */
  win_menu = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), win_menu, 1, 2, 8, 9, 0, 0, 5, 0);
  optionmenu = gtk_menu_new ();
  create_menuitem ("Blackman",        0, optionmenu, G_CALLBACK(change_win_type));
  create_menuitem ("Blackman-Harris", 1, optionmenu, G_CALLBACK(change_win_type));
  create_menuitem ("Hamming",         2, optionmenu, G_CALLBACK(change_win_type));
  create_menuitem ("Von Hann",        3, optionmenu, G_CALLBACK(change_win_type)); 
  gtk_option_menu_set_menu (GTK_OPTION_MENU (win_menu), optionmenu);
  //  gtk_option_menu_set_history (GTK_OPTION_MENU (win_menu), (int)anargs.win_type); 
  //  gtk_widget_set_size_request (win_menu, 130, 30);
  gtk_widget_show (win_menu);

  /* ATS FILE TYPE OPTION MENU */
  type_menu = gtk_option_menu_new ();
  gtk_table_attach(GTK_TABLE(table), type_menu, 3, 4, 7, 8, 0, 0, 5, 0);
  optionmenu = gtk_menu_new ();
  create_menuitem ("1 - amp freq",0, optionmenu, G_CALLBACK(change_ats_type));
  create_menuitem ("2 - amp freq pha",1, optionmenu, G_CALLBACK(change_ats_type));
  create_menuitem ("3 - amp freq nz",2, optionmenu, G_CALLBACK(change_ats_type));
  create_menuitem ("4 - amp freq pha nz",3, optionmenu, G_CALLBACK(change_ats_type)); 
  gtk_option_menu_set_menu (GTK_OPTION_MENU (type_menu), optionmenu);
  //  gtk_option_menu_set_history (GTK_OPTION_MENU (type_menu),anargs.type-1); 
  //  gtk_widget_set_size_request (type_menu, 130, 30);
  gtk_widget_show(type_menu);

  /* Create standard OK/CANCEL buttons */
  button = gtk_button_new_with_label("Do Analysis");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), button, TRUE, TRUE, 5);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(do_analysis), window);
  gtk_widget_show(button);
  button = gtk_button_new_with_label ("Forget it");
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), button, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(cancel_dialog), window);
  gtk_widget_show(button);

  /* Make the main window visible */
  update_aparameters();
  gtk_widget_show (window);
}

