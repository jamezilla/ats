/*
ATSH-FILES.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"

GtkWidget *filew;
char      *filt;
GtkWidget *outlabel;
GtkWidget *slabel;

typedef struct {
  void (*func)();
  GtkWidget *filesel;
  int opid;
} typFileSelectionData;
////////////////////////////////////////////////////////////
GtkWidget *create_men_snd(char *label, int ID, GtkWidget *parent)
{
  GtkWidget *menu;

  menu= gtk_menu_item_new_with_label (label);
  gtk_widget_show (menu);
  gtk_signal_connect (GTK_OBJECT (menu),
		      "activate",GTK_SIGNAL_FUNC (choose_output),GINT_TO_POINTER(ID));
  gtk_menu_append (GTK_MENU (parent), menu);

  return(menu);
}
///////////////////////////////////////////////////////////
double byte_swap(double *in)
{
  int size;
  double k;
  unsigned char *sw_in, *sw_out;
  double *out;
  
  size = sizeof(double);
  sw_out = (unsigned char*)malloc(size);
      
  k     = (double)in[0];
  sw_in = (unsigned char *)&k;
   
      sw_out[0] = sw_in[7];
      sw_out[1] = sw_in[6];
      sw_out[2] = sw_in[5];
      sw_out[3] = sw_in[4];
      sw_out[4] = sw_in[3];
      sw_out[5] = sw_in[2];
      sw_out[6] = sw_in[1];
      sw_out[7] = sw_in[0];
 
  out  =(double*)sw_out;

  return(*out);
}

///////////////////////////////////////////////////////////
void byte_swap_header(ATS_HEADER *hed, int flag)
{
double aux;

if(flag==TRUE) {  //may be already swapped
  aux=hed->mag;
  hed->mag=byte_swap(&aux);
}
			 
aux=hed->sr;
hed->sr = byte_swap(&aux);
aux=hed->fs;  
hed->fs= byte_swap(&aux);  
aux=hed->ws;
hed->ws= byte_swap(&aux);  
aux=hed->par;
hed->par= byte_swap(&aux); 
aux=hed->fra;
hed->fra= byte_swap(&aux); 
aux=hed->ma;
hed->ma= byte_swap(&aux);  
aux=hed->mf;
hed->mf= byte_swap(&aux);  
aux=hed->dur;
hed->dur= byte_swap(&aux); 
aux=hed->typ;
hed->typ= byte_swap(&aux); 

return;
}
///////////////////////////////////////////////////////////
void choose_output(GtkWidget *widget, gpointer data)
{

  int menu=0;
  char *str;

  str=(char*)malloc(32*sizeof(char));  
  *str=0; 
 
  menu=GPOINTER_TO_INT(data);   
  outype=menu;

  if(outype==WAV16 || outype==WAV32 ) {
    strcat(str,"*.wav");
  }
  if(outype==AIF16 || outype==AIF32 ) {
    strcat(str,"*.aif");
  }
  if(outype==SND16 || outype==SND32 ) {
    strcat(str,"*.snd");
  }

  gtk_file_selection_complete(GTK_FILE_SELECTION(filew),str);

  //g_print("\n out=%d ", outype);

  free(str);
  return;
}

///////////////////////////////////////////////////////////

int   my_filelength(FILE *fpt)
{
int   size=0;
	fseek(fpt,0,SEEK_END);
	size=(int)ftell(fpt);
	fseek(fpt,0,SEEK_SET);
return(size);
}
///////////////////////////////////////////////////////////
void set_filter(GtkWidget *widget, gpointer data)
{

gtk_file_selection_complete(GTK_FILE_SELECTION(filew),(gchar*)data);

return;
}
///////////////////////////////////////////////////////////
/*
 * --- Filename, remember it.
 */
static   char        *tittle = NULL; 

char *GetExistingFile ()
{
  gtk_label_set_text(GTK_LABEL(slabel),tittle);
  return (tittle);
    
}
    
/*
 * FileOk
 *
 * The "Ok" button has been clicked 
 * Call the function (func) to do what is needed
 * to the file.
 */
void FileOk (GtkWidget *w, gpointer data)
{
    char *sTempFile;
    typFileSelectionData *localdata;
    GtkWidget *filew;
 
    localdata = (typFileSelectionData *) data;
    filew = localdata->filesel;

    /* --- Which file? --- */
    sTempFile = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filew));

    /* --- Free old memory --- */
    if (tittle) g_free (tittle);

    /* --- Duplicate the string --- */
    tittle = g_strdup (sTempFile); 

    /* --- Call the function that does the work. --- */
    (*(localdata->func)) (tittle);
    switch(localdata->opid) {
    case SND_INSEL:
      gtk_entry_set_text(GTK_ENTRY (isfile_label), in_tittle);
      break;
    case ATS_OUTSEL:
      gtk_entry_set_text(GTK_ENTRY (afile_label), out_ats_tittle);
      break;
    case SND_OUTSEL:
      gtk_entry_set_text(GTK_ENTRY(osfile_label),out_tittle);
      break;
    case RES_OUTSEL:
      gtk_entry_set_text(GTK_ENTRY(rfile_label),res_tittle);
      break;
    }

    /* --- Close the dialog --- */
    gtk_widget_destroy (filew);
}

/*
 * destroy
 *
 * Function to handle the destroying of the dialog.  We must
 * release the focus that we grabbed.
 */
void destroy (GtkWidget *widget, gpointer *data)
{
    
    /* --- Remove the focus. --- */
    gtk_grab_remove (widget);
    g_free(filt);
    g_free (data);
}

/*
 * GetFilename
 *
 * Show a dialog with a title and if "Ok" is selected
 * call the function with the name of the file.
 */
void GetFilename (char *sTitle, void (*callback) (char *),char *selected, char *filter, int opid)
{
   
    typFileSelectionData *data;
    GtkWidget *filterbutton;
    GtkWidget *optionmenu1,  *optionmenu1_menu, *menuitem;
    filew=NULL;
    
    /* --- Create a new file selection widget --- */
    filew = gtk_file_selection_new (sTitle);

    gtk_file_selection_show_fileop_buttons(GTK_FILE_SELECTION(filew));
    data = g_malloc (sizeof (typFileSelectionData));
    data->func = callback;
    data->filesel = filew;
    data->opid    = opid;   
    
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(filew),selected);
    
    gtk_signal_connect (GTK_OBJECT (filew), "destroy",
            (GtkSignalFunc) destroy, data);

    /////////////////////////////////////////////////////////////////
    *filt=0;
    strcat(filt, "show only ");
    strcat(filt, filter);
    strcat(filt, " files");
   
    if (strcmp(filter,"*.ats")==0) {
    filterbutton= gtk_button_new_with_label (filt);
    *filt=0;
    filt = g_strdup (filter); 
    gtk_box_pack_start (GTK_BOX(GTK_FILE_SELECTION(filew)->action_area), filterbutton, TRUE, TRUE, 0);
    gtk_widget_show(filterbutton);
    gtk_signal_connect (GTK_OBJECT (filterbutton), "clicked",GTK_SIGNAL_FUNC(set_filter),(char *)filt);
    }

    if (strcmp(filter,"*.apf")==0) {
    filterbutton= gtk_button_new_with_label (filt);
    *filt=0;
    filt = g_strdup (filter); 
    gtk_box_pack_start (GTK_BOX(GTK_FILE_SELECTION(filew)->action_area), filterbutton, TRUE, TRUE, 0);
    gtk_widget_show(filterbutton);
    gtk_signal_connect (GTK_OBJECT (filterbutton), "clicked",GTK_SIGNAL_FUNC(set_filter),(char *)filt);
    }

    if (strcmp(filter,"*.wav")==0 || strcmp(filter,"*.aif")==0 || strcmp(filter,"*.snd")==0 ) {
      if(opid!=RES_OUTSEL) {
      ////////OPTION MENU///////////////////////////////////////
      optionmenu1 = gtk_option_menu_new ();
      gtk_widget_ref (optionmenu1);
      gtk_object_set_data_full (GTK_OBJECT (filew), "optionmenu1", optionmenu1,
				(GtkDestroyNotify) gtk_widget_unref);
      gtk_widget_show (optionmenu1);
      
      gtk_box_pack_start (GTK_BOX(GTK_FILE_SELECTION(filew)->action_area),optionmenu1, 
			  TRUE, TRUE, 0);
      optionmenu1_menu = gtk_menu_new ();

      menuitem = create_men_snd("16 bits shorts WAV file", 0, optionmenu1_menu);
      menuitem = create_men_snd("32 bits floats WAV file", 1, optionmenu1_menu);
      menuitem = create_men_snd("16 bits shorts AIF file", 2, optionmenu1_menu);
      menuitem = create_men_snd("16 bits shorts SND file", 3, optionmenu1_menu);

      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), (int)outype); 
      gtk_widget_set_usize (optionmenu1, 100, 25);
      }
      else {  //residual output soundfile can only be WAV
	filterbutton= gtk_button_new_with_label (filt);
	*filt=0;
	filt = g_strdup (filter); 
	gtk_box_pack_start (GTK_BOX(GTK_FILE_SELECTION(filew)->action_area), filterbutton, TRUE, TRUE, 0);
	gtk_widget_show(filterbutton);
	gtk_signal_connect (GTK_OBJECT (filterbutton), "clicked",GTK_SIGNAL_FUNC(set_filter),(char *)filt); 
      }
    }
    ///////////////////////////////////////////////////////////

    slabel= gtk_label_new (selected);
    gtk_box_pack_start (GTK_BOX(GTK_FILE_SELECTION(filew)->action_area), slabel, TRUE, TRUE, 0);
    gtk_widget_show(slabel);

    /* --- Connect the "ok" button --- */
    gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
            "clicked", (GtkSignalFunc) FileOk, data);
    
    /* --- Connect the cancel button --- */
    gtk_signal_connect_object (
             GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button),
             "clicked", (GtkSignalFunc) gtk_widget_destroy, 
             (gpointer) filew);
    gtk_window_set_position(GTK_WINDOW(filew),GTK_WIN_POS_CENTER);
    /* --- Show the dialog --- */
    gtk_widget_show (filew);

    /* --- Grab the focus. --- */
    gtk_grab_add (filew);
}
/////////////////////////////////////////////////////////////////////////////
void filesel(GtkWidget *widget,char *what)
{
  
  filt=(char*)malloc(64 * sizeof(char));
  
  if(strcmp(what,"atsin")==0){
    GetFilename("Select ATS file",atsin,ats_tittle,"*.ats",ATS_INSEL);  
  }  
  
  if(strcmp(what,"atsave")==0){
    GetFilename("Save ATS file",atsout,ats_tittle,"*.ats",ATS_INSEL);  
  }
  

  if(strcmp(what,"insel")==0){    
       GetFilename("Select Input Soundfile for analysis",setin,in_tittle,"*",SND_INSEL);
  }
  
  if(strcmp(what,"outsel")==0){
    if(outype==WAV16 || outype==WAV32 ) {
      GetFilename("Select Output Soundfile",setout,out_tittle,"*.wav",SND_OUTSEL);  
    }
    if(outype==AIF16 || outype==AIF32 ) {
       GetFilename("Select Output Soundfile",setout,out_tittle,"*.aif",SND_OUTSEL);  
     }
    if(outype==SND16 || outype==SND32) {
       GetFilename("Select Output Soundfile",setout,out_tittle,"*.snd",SND_OUTSEL);  
     }

  }
 
 if(strcmp(what,"atsoutsel")==0){    
       GetFilename("Select output ats file",setout_ats,out_ats_tittle,"*.ats",ATS_OUTSEL);  
  }

 if(strcmp(what,"getap")==0){    
       GetFilename("Load Analysis Parameters file",getap,apf_tittle,"*.apf",APF_INSEL);  
  }
 if(strcmp(what,"savap")==0){    
       GetFilename("Save Analysis Parameters file",savap,apf_tittle,"*.apf",APF_OUTSEL);  
  } 
  
 if(strcmp(what,"res_sel")==0){    
       GetFilename("Select Residual Output Soundfile",setres,res_tittle,"*.wav",RES_OUTSEL);  
  } 

return;
}
/////////////////////////////////////////////////////////////////////////////
void setres(char *pointer)
{
*res_tittle=0;

strcat(res_tittle,pointer);

return;
}
/////////////////////////////////////////////////////////////////////////////
void setaud(char *pointer)
{

return;
}
/////////////////////////////////////////////////////////////////////////////
void setin(char *pointer)
{
*in_tittle=0;

strcat(in_tittle,pointer);
//g_print ("%s\n", pointer);

return;
}
/////////////////////////////////////////////////////////////////////////////
void setout(char *pointer)
{
*out_tittle=0;

strcat(out_tittle,pointer);
//g_print ("%s\n", pointer);

return;
}

/////////////////////////////////////////////////////////////////////////////
void setout_ats(char *pointer)
{
*out_ats_tittle=0;

strcat(out_ats_tittle,pointer);
//g_print ("%s\n", pointer);

return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void atsin(char *pointer)
{
  int sizef=0, length=0, i, x;
  float  dt=0., normfac=0.;
  double aux=0., magic=123.00;
  int n1=2, n2=0;
  FILE *pundo;

  need_byte_swap=FALSE;  
  *info=0;

  //if(filew){gtk_widget_hide(GTK_WIDGET(filew));}

  if((pundo=fopen(undo_file,"wb"))!=0) {
    fclose(pundo);    
  }
  else {
    Popup("WARNING: Could not delete the UNDO backup file, UNDO disabled");
    undo=FALSE;
  }

  view_type=SON_VIEW;
  //the contrast of display
  valexp=.5;

  *ats_tittle=0;
  strcat(ats_tittle,pointer);    

  //g_print ("%s ", pointer);

 if((atsfin=fopen(ats_tittle,"rb"))==0) {
   strcat(info, "Could not open ");
   strcat(info, ats_tittle);
   Popup(info);
   *ats_tittle=0;
   floaded=FALSE;
   draw_default();
   init_scalers(FALSE);
   return;
 }

 length=my_filelength(atsfin);
 //g_print (" (%d BYTES) \n", length);
  
 fread(atshed,sizeof(ATS_HEADER),1,atsfin);

 //here we try to deal with the BIG/LITTLE ENDIAN stuff 
 if(atshed->mag != magic) { 
   aux=atshed->mag;		
   atshed->mag =byte_swap(&aux);

   if((int)atshed->mag != (int)magic) { 
	Popup("ERROR:ATS MAGIC NUMBER NOT FOUND IN HEADER...");
   	*ats_tittle=0;
   	fclose(atsfin);
   	floaded=FALSE;
	need_byte_swap=FALSE;
   	init_scalers(FALSE);
   	draw_default();
   	return;
   } 
   else {
      need_byte_swap=TRUE;               //now we know that we must swap the bytes 
	byte_swap_header(atshed,FALSE);    //swap bytes of header data first
   }  
 
 }
 

//////////////////////find out length of data and check it for consistence////////////////

 if(FILE_HAS_PHASE) {
   n1+=1;
 }
 if(FILE_HAS_NOISE) {
   n2+=1;
 }

 sizef =  sizeof(ATS_HEADER) +
   ((int)atshed->par * (int)atshed->fra * sizeof(double) * n1) +
   (sizeof(double) * (int)atshed->fra) + 
   (NB_RES * (int)atshed->fra * sizeof(double) * n2);



if(sizef != length) { //error: the file size does not match the header data
   Popup("ERROR: FILE SIZE DOES NOT MATCH HEADER DATA...");
   *ats_tittle=0;
   fclose(atsfin);
   floaded=FALSE;
   init_scalers(FALSE);
   draw_default();
   return; 
 }
/////////////////////////////////////////////////////////////////////////////////////////
 
if(!ats_sound) {
   Popup("Could not allocate enough memory for ATS data");
   *ats_tittle=0;
   fclose(atsfin);
   floaded=FALSE;
   init_scalers(FALSE);
   draw_default();
   return; 
 }

mem_realloc();
////////////////////////////////////////////////////////////////////////////////////////

 normfac= 1. / atshed->ma;
 ///EVERYTHING SEEMS TO BE OK, THEN LOAD THE DATA...
 fseek(atsfin,sizeof(ATS_HEADER),SEEK_SET);
  

 for(i=0; i<(int)atshed->fra; ++i) {
   
   fread(&aux,1,sizeof(double),atsfin);  //read just one time value per frame
   
   if(need_byte_swap==TRUE) {
	 ats_sound->time[0][i]=byte_swap(&aux); 
   } 
   else {
       ats_sound->time[0][i]=aux; 
   }

   for (x=0; x<(int)atshed->par; ++x) {
   
     fread(&aux,1,sizeof(double),atsfin);  //read amp value for each partial 
     if(need_byte_swap==TRUE) {
	 ats_sound->amp[x][i]=byte_swap(&aux); 
     } 
     else {
       ats_sound->amp[x][i]= aux; 
     } 
     fread(&aux,1,sizeof(double),atsfin); //read freq value for each partial 
     if(need_byte_swap==TRUE) {
       ats_sound->frq[x][i]=byte_swap(&aux); 
     } 
     else {
       ats_sound->frq[x][i]= aux; 
     }
   
     if(FILE_HAS_PHASE) {                 //read phase data if any...
       fread(&aux,1,sizeof(double),atsfin); 
       if(need_byte_swap==TRUE) {
	 ats_sound->pha[x][i]=byte_swap(&aux); 
       } 
       else {
	 ats_sound->pha[x][i]= aux; 
       }
     }
  }  
   
   if (FILE_HAS_NOISE) {
     for (x=0; x<NB_RES; x++) {//read residual analysis data if any...
       fread(&aux,1,sizeof(double),atsfin);
       if(need_byte_swap==TRUE) {
          ats_sound->band_energy[x][i]=byte_swap(&aux);  
       }
	 else {
          ats_sound->band_energy[x][i]=aux;
       }

     }
   }
   
 }

//g_print ("\n read OK \n");


/////////////// find out max. dt//////////////////////////////
 maxtim=0.;

 for(i=0; i<(int)atshed->fra - 1; ++i) {
  dt=ats_sound->time[0][1+i] - ats_sound->time[0][i];
  if(dt > maxtim) {maxtim=dt;} //find out max. dt.
}
 //g_print ("MAX. delta time is = %f secs.\n",maxtim );
 
 //the horizontal selection parameters 
 selection->from=selection->to=0;
 selection->f1=selection->f2=0;
 selection->x1=selection->y1=selection->x2=selection->y2=0;
 vertex1=vertex2=FALSE;
 

 if (FILE_HAS_PHASE) {
   sparams->upha=TRUE;
 }
 else{
   sparams->upha=FALSE;
 }
 
 //Default initialization of synthesis data...
 sparams->sr  = 44100.; 
 sparams->amp = 1.;
 if(FILE_HAS_NOISE) {
   sparams->ramp = 1.;
 }
 else {
   sparams->ramp = 0.;
 }
 sparams->frec = 1.;
 sparams->max_stretch= 1.;
 sparams->beg = 0.;
 sparams->end = ats_sound->time[0][(int)atshed->fra-1] + ats_sound->time[0][1];
 sparams->allorsel=FALSE;

//initialization of smart selection data

 sdata->from=  1;
 sdata->to=(int)atshed->par; 
 sdata->step=  1;
 sdata->tres=  -96.;
 sdata->met =  FALSE;

 floaded=TRUE;
 init_scalers(TRUE); 
 ned=led=0;
 undo=TRUE;

 //the contrast of display
 valexp=.5;
 GTK_ADJUSTMENT(valadj)->upper=100.;
 GTK_ADJUSTMENT(valadj)->lower=1.;
 gtk_adjustment_set_value(GTK_ADJUSTMENT(valadj), valexp * 100.);  
 

 if(tWedit == NULL) {
   tWedit=create_edit_curve(TIM_EDIT, timenv);
 }
 else {
   set_time_env(timenv, TRUE);
 }

 fclose(atsfin);
 draw_pixm(); 
 
 return;
}
/////////////////////////////////////////////////////////////////////////////
void atsout(char *pointer)
{
  int i,x, length;
  double aux;

*ats_tittle=0;
strcat(ats_tittle,pointer);
//g_print ("%s\n", pointer);

 if((atsfin=fopen(ats_tittle,"wb"))==0) {
   Popup("Could not open output file for writing");
   return;
 }

 //write header
 fwrite(atshed,1,sizeof(ATS_HEADER),atsfin);
 //write data
for(i=0; i<(int)atshed->fra; ++i) {
  
  aux=(double)ats_sound->time[0][i];
  fwrite(&aux,1,sizeof(double),atsfin);
 
   for (x=0; x<(int)atshed->par; ++x) {

     aux=(double)ats_sound->amp[x][i]; 
     fwrite(&aux,1,sizeof(double),atsfin); 
     aux=(double)ats_sound->frq[x][i];  
     fwrite(&aux,1,sizeof(double),atsfin); 

      if(FILE_HAS_PHASE) { //read phase data if any...
	aux=(double)ats_sound->pha[x][i]; 
	fwrite(&aux,1,sizeof(double),atsfin);
      }
   }  
   if (FILE_HAS_NOISE) {
     for (x=0; x<NB_RES; x++) {//read residual analysis data if any...
       aux=(double)ats_sound->band_energy[x][i];
       fwrite(&aux,1,sizeof(double),atsfin); 
     }
   }
   
 }

 length=my_filelength(atsfin); 
 //g_print ("%d bytes written OK \n", length);
 fclose(atsfin);


return;
}
////////////////////////////////////////////////////////////////////////////
void stringinit()
{
in_tittle     =(char*)malloc(1024*sizeof(char));
out_tittle    =(char*)malloc(1024*sizeof(char));
out_ats_tittle=(char*)malloc(1024*sizeof(char));
ats_tittle    =(char*)malloc(1024*sizeof(char));
undo_file     =(char*)malloc(1024*sizeof(char));
apf_tittle    =(char*)malloc(1024*sizeof(char));
res_tittle    =(char*)malloc(1024*sizeof(char));

*in_tittle=*out_tittle=*out_ats_tittle=*ats_tittle=*undo_file=*res_tittle=0;

return;
}
////////////////////////////////////////////////////////////////////////////
void stringfree()
{
free(in_tittle);
free(out_tittle);
free(out_ats_tittle);
free(ats_tittle);
free(undo_file);
free(apf_tittle);
free(res_tittle);
}
////////////////////////////////////////////////////////////////////////////
void edit_audio()
{



return;
}
////////////////////////////////////////////////////////////////////////////
void getap(char *pointer)
{
  FILE *in;

  *apf_tittle=0;
  strcat(apf_tittle,pointer);    

  if((in=fopen(apf_tittle,"rb"))==0) {
    *info=0; 
    strcat(info, "Could not open ");
    strcat(info, apf_tittle);
    Popup(info);
    *apf_tittle=0;
    return;
  }
  
  if(fread(atsh_anargs,1,sizeof(ANARGS),in)==0) {
    *info=0; 
    strcat(info, "Error: Could not read ");
    strcat(info, apf_tittle);
    Popup(info);
    *apf_tittle=0;
    fclose(in);
    return;
  }
  update_aparameters();

  fclose(in);
  return;
}
////////////////////////////////////////////////////////////////////////////
void savap(char *pointer)
{
  FILE *out;

  *apf_tittle=0;
  strcat(apf_tittle,pointer);    
  
  if((out=fopen(apf_tittle,"wb"))==0) {
    *info=0; 
    strcat(info, "Error: Could not open ");
    strcat(info, apf_tittle);
    Popup(info);
    *apf_tittle=0;
    return;
  }

  if(fwrite(atsh_anargs,1,sizeof(ANARGS),out)==0) {
    *info=0; 
    strcat(info, "Error: Could not write on ");
    strcat(info, apf_tittle);
    Popup(info);
    *apf_tittle=0;
    fclose(out);
    return;
  }
  fclose(out);
  return;
}
