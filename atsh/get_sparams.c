/*
GET_SPARAMS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
GtkWidget     *osfile_label;
extern char *out_tittle;
extern char *ats_tittle;
extern GtkWidget **entry;
char **param_list;
GtkWidget *tlabel1, *tlabel2;

/////////////////////////////////////////////////////////////////////////////
void allorsel (GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) 
    {/*If control reaches here, the toggle button is down */                      
	sparams->allorsel=TRUE;
	if(SOMETHING_SELECTED) {
		sparams->beg=ats_sound->time[0][selection->from];
		sparams->end=ats_sound->time[0][selection->to]+ ats_sound->time[0][1];
	}
   	gtk_label_set_text(GTK_LABEL(tlabel1),"Use only selected partials"); 
    } 

  else {/* If control reaches here, the toggle button is up */
    	sparams->allorsel=FALSE;
 	if(SOMETHING_SELECTED) {
		sparams->beg=0.;
		sparams->end=ats_sound->time[0][(int)atshed->fra - 1] + ats_sound->time[0][1];

	}
    gtk_label_set_text(GTK_LABEL(tlabel1),"    Use all partials   ");
  }
  return;
}
/////////////////////////////////////////////////////////////////////////////
void upha(GtkWidget *widget, gpointer data) //is not used
{
  if (GTK_TOGGLE_BUTTON (widget)->active) 
    {/*If control reaches here, the toggle button is down */                      
      sparams->upha=TRUE;
      gtk_label_set_text(GTK_LABEL(tlabel2),"Use Phase Data       "); 
    } 
  else {/* If control reaches here, the toggle button is up */
    sparams->upha=FALSE; 
    gtk_label_set_text(GTK_LABEL(tlabel2),  "Do Not Use Phase Data");
  }
  return;
}
/////////////////////////////////////////////////////////////////////////////
GtkWidget *CreateEditField (char *name, int what)
{
GtkWidget *hbox,*label;
gchar *aux;   

 aux=(char*)malloc(10*sizeof(char));
 *aux=0;
 switch(what) {
 case 0:
   sprintf(aux,"%8.3f",sparams->amp);   
   break;
 case 1:
   sprintf(aux,"%8.3f",sparams->ramp);   
   break;
 case 2:
   sprintf(aux,"%8.3f",sparams->frec);   
   break;
 case 3:
   sprintf(aux,"%8.2f",sparams->sr);   
   break; 

}

    hbox = gtk_hbox_new (FALSE, 0);
    
    label = gtk_label_new (name);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE,10);
    gtk_widget_show (label);

    entry[what] = gtk_entry_new_with_max_length (10);
    gtk_widget_set_usize(GTK_WIDGET(entry[what]),60,20);
    gtk_box_pack_start (GTK_BOX (hbox), entry[what],FALSE,FALSE,0);
    gtk_entry_set_text(GTK_ENTRY (entry[what]),aux);
    gtk_widget_show (entry[what]);
    gtk_widget_show (hbox);  
    free(aux);
    return (hbox);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

gchar **alocstring(int np, int nc)
{
char **pointer;
int i;

pointer= (char**)malloc(np * sizeof(char*));

   for (i = 0; i <np; i++) {
     pointer[i]=(char*)malloc(nc * sizeof(char));
     pointer[i]=0;
   }    
return(pointer);

}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void set_params()
{
char *str;  

 str=(char*)malloc(10 * sizeof(char)); 
 *str=0;        

 str=gtk_editable_get_chars(GTK_EDITABLE(entry[0]),0,9);
 sparams->amp=(float)atof((char*)str);
 *str=0;

 if(FILE_HAS_NOISE) {
   str=gtk_editable_get_chars(GTK_EDITABLE(entry[1]),0,9);
   sparams->ramp=(float)atof((char*)str);
   *str=0;

 }

 str=gtk_editable_get_chars(GTK_EDITABLE(entry[2]),0,9);
 sparams->frec=(float)atof((char*)str);
 *str=0;
 
 str=gtk_editable_get_chars(GTK_EDITABLE(entry[3]),0,9);
 sparams->sr=(float)atof((char*)str);
 *str=0;


 //Phase information is not used for synthesis at present
 sparams->upha=FALSE;

free(str);
return;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void delete_event (GtkWidget *widget, gpointer data)
{

  set_params();//retrieve parameters from entry fields
  do_synthesis();
  
	//free(param_list);
  	//gtk_widget_destroy (GTK_WIDGET (data));

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void just_quit (GtkWidget *widget, gpointer data)
{
  free(param_list);
  gtk_widget_destroy (GTK_WIDGET (data));
}


///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
//

//
void get_sparams ()
{
    GtkWidget *window;
    GtkWidget *widget;
    GtkWidget *cancel;
    GtkWidget *ok, *fileb, *timeb;
    GtkWidget *tbpartials;
	GtkWidget *void1, *void2;
	
    int i;
    int nfields=4;

    if(*ats_tittle==0) { //no ATS file data alocated, just ignore the call
      return; 
    }

    param_list=alocstring(nfields,20);
    param_list[0]="Sinusoidal output gain";
    param_list[1]="Noise output gain";
    param_list[2]="Frequency scalar";
    param_list[3]="Sampling Rate";

    // --- Create the top level window
    window =gtk_dialog_new ();
    /* --- Add a title to the window --- */
    gtk_window_set_title (GTK_WINDOW (window), "Synthesis");
    gtk_window_set_policy(GTK_WINDOW (window),FALSE,FALSE,FALSE);                  
    // --- You should always remember to connect the destroy event
    //     to the main window.
    gtk_signal_connect (GTK_OBJECT (window), "destroy",
			GTK_SIGNAL_FUNC (ClosingDialog),&window);

    // --- Give the window a border
    gtk_container_border_width (GTK_CONTAINER (window), 5);

    // --- Create edit fields
    for(i=0; i<nfields; ++i) {
      if(i != 1) {
	widget = CreateEditField (param_list[i],i);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), widget, FALSE, FALSE, 0);
      }
      else{
	if(FILE_HAS_NOISE){
	  widget = CreateEditField (param_list[i],i);
	  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), widget, FALSE, FALSE, 0);  
	}
      }
    }
    
	//create a void box as separator
	void1 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), void1, FALSE, FALSE, 3);
	gtk_widget_show (void1);
    
	
     //
    /* Create labels for the toggle buttons */
    tlabel1 = gtk_label_new ("Selected Partials Only");
    //tlabel2 = gtk_label_new ("Use Phase Information");
    

	//-- Create the "set time pointer" button
     timeb = gtk_button_new_with_label ("Set Time Pointer");
     gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), timeb, FALSE, FALSE, 2);
     gtk_signal_connect (GTK_OBJECT (timeb), "clicked", GTK_SIGNAL_FUNC (edit_tim),NULL);
	 gtk_widget_show (timeb);


    // --- Create the toggle button for Partials
    tbpartials= gtk_toggle_button_new();
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(tbpartials),sparams->allorsel);
    gtk_signal_connect (GTK_OBJECT (tbpartials), "toggled",
			GTK_SIGNAL_FUNC (allorsel),NULL);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), tbpartials, FALSE, FALSE, 2);
    gtk_container_add (GTK_CONTAINER (tbpartials), tlabel1);
    gtk_widget_show (tlabel1);
    gtk_widget_show (tbpartials);
    allorsel(GTK_WIDGET(tbpartials), NULL);
    //
    
    // --- Create the toggle button for Phase
    /*
    if (FILE_HAS_PHASE) {
      tbphase= gtk_toggle_button_new();
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(tbphase),sparams->upha);
      gtk_signal_connect (GTK_OBJECT (tbphase), "toggled",
			GTK_SIGNAL_FUNC (upha),NULL);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), tbphase, TRUE, TRUE,0);
      gtk_container_add (GTK_CONTAINER (tbphase), tlabel2);
      gtk_widget_show (tlabel2);
      gtk_widget_show (tbphase);
      upha(GTK_WIDGET(tbphase), NULL);
    }
    */

    //-- Create the set output file button
    fileb = gtk_button_new_with_label ("Set Output File");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), fileb, FALSE, FALSE, 2);
    gtk_signal_connect (GTK_OBJECT (fileb), "clicked", GTK_SIGNAL_FUNC (filesel),"outsel");
    gtk_widget_show (fileb);
    
    osfile_label=gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(osfile_label), FALSE);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), osfile_label, FALSE, FALSE, 2);
    gtk_entry_set_text(GTK_ENTRY (osfile_label),out_tittle);
    gtk_widget_show (osfile_label);

    //create a void box as separator
    void2 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), void2, FALSE, FALSE, 5);
    gtk_widget_show (void2);

    // --- Create standard OK/CANCEL buttons
    ok = gtk_button_new_with_label ("Do it");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, TRUE, TRUE, 5);
    gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (delete_event),window);
    gtk_widget_show (ok);
    
    cancel = gtk_button_new_with_label ("Forget it");
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (just_quit),window);
    
    gtk_widget_show (cancel);

    // --- Make the main window visible
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show (window);
    gtk_grab_add (window);

    return;
}

