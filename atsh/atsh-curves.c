/*
ATSH-CURVES.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"
#include "my_curve.h"
/////////////////////////////////////////////////////////////////////////////
int get_nbp(GtkWidget *curve) 
{
//returns the number of breakpoints of the curve
return(GTK_MY_CURVE (curve)->num_ctlpoints);
}
////////////////////////////////////////////////////////////////////////////
float get_x_value(GtkWidget *curve, int breakpoint) 
{
//returns the x value of the curve at the given breakpoint 

return((float)(GTK_MY_CURVE(curve)->ctlpoint[breakpoint][0]));
}
////////////////////////////////////////////////////////////////////////////
float get_y_value(GtkWidget *curve, int breakpoint) 
{
//returns the x value of the curve at the given breakpoint 

return((float)(GTK_MY_CURVE(curve)->ctlpoint[breakpoint][1]));
}
/////////////////////////////////////////////////////////////////////////////
void change_curve_type(GtkWidget *widget, ENVELOPE *data)
{
  if(data->cflag==TRUE) {
    data->cflag=FALSE;
    gtk_label_set_text(GTK_LABEL(data->clabel),"spline curve"); 
    gtk_my_curve_set_curve_type(GTK_MY_CURVE (data->curve),GTK_CURVE_TYPE_SPLINE);
  }
  
  else {
    gtk_label_set_text(GTK_LABEL(data->clabel),"linear curve"); 
    gtk_my_curve_set_curve_type(GTK_MY_CURVE (data->curve),GTK_CURVE_TYPE_LINEAR);
    data->cflag=TRUE;
  }
  
  return;
  
}
/////////////////////////////////////////////////////////////////////////////
void change_operation(GtkWidget *widget, ENVELOPE *data)
{

  if(data->tflag==TRUE) {
    data->tflag=FALSE;
    gtk_label_set_text(GTK_LABEL(data->tlabel),"Offset(add)"); 
  }

  else {
    gtk_label_set_text(GTK_LABEL(data->tlabel),"Scale(Multiply)"); 
    data->tflag=TRUE;
  }

  return;
}
/////////////////////////////////////////////////////////////////////////////
void hidewindow(GtkWidget *widget)
{
 
  gtk_grab_remove (GTK_WIDGET(widget));
  gtk_widget_hide(GTK_WIDGET(widget));
  return;
}
/////////////////////////////////////////////////////////////////////////////
void ismoving(GtkWidget *widget, GdkEventMotion *event, ENVELOPE *data)
{
  int xpos,ypos, fra=0;
  float range, val=0.;
  GdkModifierType state;
  float width, height;
  *info=0;

  width= widget->allocation.width;
  height=widget->allocation.height;
  gdk_window_get_pointer(event->window, &xpos, &ypos, &state); //get new position
  
  range=GTK_MY_CURVE(widget)->max_y - GTK_MY_CURVE(widget)->min_y;
  val= GTK_MY_CURVE(widget)->min_y +(range - (( range / height ) * (float)ypos));
  fra =selection->from + (int) (((float)aveclen / width) * (float)xpos);
  
  sprintf(info,"Frame=%d Time=%7.2f Value=%7.2f",fra+1,ats_sound->time[0][fra],val);
  gtk_label_set_text(GTK_LABEL(data->info_label),info);
  
  return;
}
/////////////////////////////////////////////////////////////////////////////
void ismoving_time(GtkWidget *widget, GdkEventMotion *event, ENVELOPE *data)
{
  int xpos,ypos; 
  float range, val=0.,time;
  GdkModifierType state;
  float width, height;
  
  *info=0;

  width= widget->allocation.width;
  height=widget->allocation.height;
  gdk_window_get_pointer(event->window, &xpos, &ypos, &state); //get the new position
  
  range=data->ymax - data->ymin;
  val= data->ymin + (range - (( range / height ) * (float)ypos));
  time =( (float)data->dur / width) * (float)xpos;
  
  sprintf(info,"Analysis Time=%7.2f Output Time=%7.2f",val,time);
  gtk_label_set_text(GTK_LABEL(data->info_label),info);
  
  return;
}
/////////////////////////////////////////////////////////////////////////////
void setmin_time(GtkWidget *widget, ENVELOPE *data)
{
  char *str=(char*)malloc(13 * sizeof(char)); 
  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(data->minentry),0,13);
  data->ymin=(float)atof(str);

  
  if(data->ymin < 0. || data->ymin >= data->ymax ) {
    data->ymin= 0.; //no negative values allowed for time
    *str=0;
    sprintf(str," %12.5f",data->ymin);      
    gtk_entry_set_text(GTK_ENTRY (data->minentry),str);
  }  

  free(str);

  return;
}
/////////////////////////////////////////////////////////////////////////////
void setmax_time(GtkWidget *widget, ENVELOPE *data)
{
 
  float val, stime;
  char *str=(char*)malloc(13 * sizeof(char)); 
  
  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(data->maxentry),0,13);

  val=(float)atof(str);
  data->ymax=val;
  *str=0;
  sprintf(str," %12.5f",ats_sound->time[0][(int)atshed->fra-1] );
  stime=(float)atof(str);
  
  if(val  > stime || val <= 0.) {
    data->ymax=stime; //no values beyond the lenght of the analysis allowed
    *str=0;
    sprintf(str," %12.5f",data->ymax);
    gtk_entry_set_text(GTK_ENTRY(data->maxentry),str);
  }
 
  free(str);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void change_duration(GtkWidget *widget, ENVELOPE *data)
{
  char *str=(char*)malloc(13 * sizeof(char)); 
  str=gtk_editable_get_chars(GTK_EDITABLE(widget),0,13);
  data->dur=atof(str);
  free(str);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void setmax(GtkWidget *widget, ENVELOPE *data)
{
  char *str=(char*)malloc(10 * sizeof(char)); 
  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(data->maxentry),0,9);
  data->ymax=(float)atof((char*)str);
  gtk_my_curve_set_range (GTK_MY_CURVE (data->curve),0.,CURVE_LENGTH, data->ymin, data->ymax);
  free(str);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void setmin(GtkWidget *widget, ENVELOPE *data)
{
  char *str=(char*)malloc(10 * sizeof(char)); 
  *str=0;        
  str=gtk_editable_get_chars(GTK_EDITABLE(data->minentry),0,9);
  data->ymin=(float)atof((char*)str);
  gtk_my_curve_set_range (GTK_MY_CURVE (data->curve),0.,CURVE_LENGTH, data->ymin, data->ymax);
  free(str);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void do_fredit (GtkWidget *widget, ENVELOPE *data)
{
  int i,x;
  float niq=atshed->sr / 2.;
  int nValue=0, todo=aveclen * (int)atshed->par;

  StartProgress("Changing Frequency...",FALSE);
  gtk_my_curve_get_vector(GTK_MY_CURVE(data->curve), aveclen, fvec);
  
  backup_edition(FRE_EDIT);
  if(FILE_HAS_NOISE) {
    atshed->typ = 3.;    //delete phase information
  }
  else {
    atshed->typ = 1.; 
  }
  
  sparams->upha=FALSE;
 
  for(i=0; i < (int)atshed->par;  ++i) {
    for(x=0; x < aveclen; ++x) {
      if(selected[i]== TRUE) {
	if(data->tflag==TRUE) { //SCALE
	  ats_sound->frq[i][x+selection->from]*=(double)fabs(fvec[x]); //no negative values allowed
	}
	else { //ADD
	   ats_sound->frq[i][x+selection->from]+=(double)fvec[x];
	}
	if(ats_sound->frq[i][x+selection->from] > niq)        
	  ats_sound->frq[i][x+selection->from]=niq; 
	//clip freq. if out of range 
	if(ats_sound->frq[i][x+selection->from] < 0.)
	  ats_sound->frq[i][x+selection->from]=0.;
	if(ats_sound->frq[i][x+selection->from] > atshed->mf) 
	  ats_sound->frq[i][x+selection->from]=atshed->mf;
	
      }
      if(ats_sound->frq[i][x+selection->from] > atshed->mf) {
	atshed->mf=ats_sound->frq[i][x+selection->from];
      } //update max.freq.
      ++nValue;
      UpdateProgress(nValue,todo);
    }
  }
    
  EndProgress();
  draw_selection();
  gtk_grab_remove (fWedit);
  gtk_widget_hide(GTK_WIDGET(fWedit));
  return;
}
/////////////////////////////////////////////////////////////////////////////
void do_amedit (GtkWidget *widget, ENVELOPE *data)
{
  int i, x;
  int nValue=0, todo=aveclen * (int)atshed->par;

  StartProgress("Changing Amplitude...", FALSE);
  gtk_my_curve_get_vector(GTK_MY_CURVE(data->curve),aveclen,avec);    

    backup_edition(AMP_EDIT);
    for(i=0; i < (int)atshed->par;  ++i) {
      for(x=0; x < aveclen; ++x) {
	if(selected[i]== TRUE) {
	  if(data->tflag==TRUE) { //SCALE
	    ats_sound->amp[i][x+selection->from]*=fabs(avec[x]); //no negative values alowwed
	  }
	  else { //ADD
	    ats_sound->amp[i][x+selection->from]+=avec[x];
	  }
	  if(ats_sound->amp[i][x+selection->from] > 1.) 
	    ats_sound->amp[i][x+selection->from]=1.; 
	  //clip to normalized values
	  if(ats_sound->amp[i][x+selection->from] < 0.) 
	    ats_sound->amp[i][x+selection->from]=0.;
	}
	if(ats_sound->amp[i][x+selection->from] > atshed->ma) {
	  atshed->ma=ats_sound->amp[i][x+selection->from];
	} //update max.amp.
	++nValue;
	UpdateProgress(nValue,todo);
      }
    }

    EndProgress();
    draw_selection();
    gtk_grab_remove (aWedit);
    gtk_widget_hide(GTK_WIDGET(aWedit));
    
    return;
}
/////////////////////////////////////////////////////////////////////////////
void cancel_wedit(GtkWidget *widget, gpointer *data)
{
 
  gtk_grab_remove (GTK_WIDGET(data));
  gtk_widget_hide(GTK_WIDGET(data)); //data is the Window 
}

/////////////////////////////////////////////////////////////////////////////

void curve_reset(GtkWidget *widget, ENVELOPE *data)
{
  char *str;  
  data->ymin=0.;
  data->ymax=1.; 
  str=(char*)malloc(10 * sizeof(char));  
  
  gtk_my_curve_set_range (GTK_MY_CURVE (data->curve), 0.,CURVE_LENGTH , 
			  data->ymin, data->ymax);
  *str=0;
  sprintf(str," %10.3f",data->ymax);      
  gtk_entry_set_text(GTK_ENTRY (data->maxentry),str);
  *str=0;
  sprintf(str," %10.3f",data->ymin);      
  gtk_entry_set_text(GTK_ENTRY (data->minentry),str);
 

  free(str); 
  return;
}
//////////////////////////////////////////////////////////////////////////
void curve_reset_time(GtkWidget *widget, ENVELOPE *data)
{

  set_time_env(data, TRUE);
  gtk_my_curve_set_range (GTK_MY_CURVE (data->curve), 
			    0.,CURVE_LENGTH, 0.,CURVE_LENGTH);
  return;
}
//////////////////////////////////////////////////////////////////////////
GtkWidget *create_edit_curve(int which_one, ENVELOPE *envelope)
{
  GtkWidget *Wedit;
  GtkWidget *vbox1,*vbox2, *vbox3, *vbox4;
  GtkWidget *hbox1,*hbox2, *hbox4,*hbox6,*hbox7, *hbox8 , *hbox9;
  GtkWidget *label8, *label9, *label10;
  GtkWidget *button1, *button2, *button3;
  
  
  
  envelope->tflag=TRUE;
  envelope->cflag=TRUE;

  Wedit = gtk_window_new (GTK_WINDOW_DIALOG);

  gtk_window_set_position(GTK_WINDOW(Wedit),GTK_WIN_POS_CENTER);
  gtk_object_set_data (GTK_OBJECT (Wedit), "Wedit", Wedit);
  gtk_signal_connect (GTK_OBJECT (Wedit), "delete_event",GTK_SIGNAL_FUNC (hidewindow), NULL);

  gtk_window_set_policy (GTK_WINDOW (Wedit), FALSE, TRUE, FALSE); 
    
  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);

  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (Wedit), vbox1);

  
  envelope->curve = gtk_my_curve_new ();
  gtk_widget_ref (envelope->curve);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "curve", envelope->curve,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (vbox1), envelope->curve, TRUE, TRUE, 0);
  gtk_widget_set_usize (envelope->curve, 2, 200);
  gtk_my_curve_reset (GTK_MY_CURVE (envelope->curve));
  
  //this function produces an error ("Glib-error: could not allocate -20 bytes")	 
  gtk_my_curve_set_curve_type(GTK_MY_CURVE (envelope->curve),GTK_CURVE_TYPE_LINEAR);
  

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2, FALSE, FALSE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox2", hbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 20);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox3, FALSE, FALSE, 0);

  hbox7 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox7);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox7", hbox7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox7);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox7, TRUE, FALSE, 0);
  
  ////////////////////////////////////////////////////////////////
  
  label8 = gtk_label_new (NULL);
  gtk_widget_ref (label8);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "label8", label8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (hbox7), label8, TRUE, FALSE, 0);

  envelope->minentry = gtk_entry_new ();
  gtk_widget_ref (envelope->minentry);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "entry2", envelope->minentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (envelope->minentry);
  gtk_box_pack_start (GTK_BOX (hbox7), envelope->minentry, FALSE, TRUE, 0);
  gtk_widget_set_usize (envelope->minentry, 75, 25);
 
  gtk_entry_set_editable (GTK_ENTRY (envelope->minentry),TRUE);

  hbox8 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox8);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox8", hbox8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox8);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox8, TRUE, TRUE, 0);

  label10 = gtk_label_new (NULL);
  gtk_widget_ref (label10);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "label10", label10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label10);
  gtk_box_pack_start (GTK_BOX (hbox8), label10, TRUE, FALSE, 0);

  envelope->maxentry = gtk_entry_new ();
  gtk_widget_ref (envelope->maxentry);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "entry1", envelope->maxentry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (envelope->maxentry);
  gtk_box_pack_start (GTK_BOX (hbox8), envelope->maxentry, FALSE, FALSE, 0);
  gtk_widget_set_usize (envelope->maxentry, 75, 25);
  gtk_entry_set_editable (GTK_ENTRY (envelope->maxentry),TRUE);

  ///here we manage some of the time envelope exceptions
  if (which_one != TIM_EDIT) {   
    envelope->ymin=0.;
    envelope->ymax=1.;
    gtk_label_set_text(GTK_LABEL(label8),"Min. Y Value");
      gtk_entry_set_text(GTK_ENTRY (envelope->minentry),"0.0");
    gtk_label_set_text(GTK_LABEL(label10),"Max. Y value");
      gtk_entry_set_text(GTK_ENTRY (envelope->maxentry),"1.0");
    gtk_signal_connect (GTK_OBJECT (envelope->minentry), "changed",
    			  GTK_SIGNAL_FUNC (setmin),envelope);
    gtk_signal_connect (GTK_OBJECT (envelope->maxentry), "changed",
    		      GTK_SIGNAL_FUNC (setmax),envelope); 
  }
  
  //////////////////////////////////////////////
  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox4);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "vbox4", vbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox4);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox4, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox4), 10);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox4);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox4", hbox4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox4);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox4, TRUE, TRUE, 0);

  ///////////////////////////////////
  
  envelope->info_label= gtk_label_new ("");
  gtk_widget_ref (envelope->info_label);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "info_label", envelope->info_label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (envelope->info_label);
  gtk_box_pack_start (GTK_BOX (hbox4), envelope->info_label, FALSE, FALSE, 0);
  
  ///////////////////////////////////

  hbox6 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox6);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox6", hbox6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox6);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox6, TRUE, TRUE, 0);

  button1 = gtk_button_new_with_label (("reset"));
  gtk_widget_ref (button1);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "button1", button1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button1);
   gtk_box_pack_start (GTK_BOX (hbox6), button1, TRUE, TRUE, 5);
 
  
  ////////////////////////////////////////////////////////////////
  ///BUTTONS
  if(which_one != TIM_EDIT) {

    button2 = gtk_button_new_with_label ("Forget it");
    gtk_widget_ref (button2);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "button2", button2,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (button2);
    gtk_box_pack_start (GTK_BOX (hbox6), button2, TRUE, TRUE, 5);
    gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			GTK_SIGNAL_FUNC (cancel_wedit),Wedit);
    ////////////// 
    envelope->tlabel = gtk_label_new ("Scale(multiply)");
    gtk_widget_ref (envelope->tlabel);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "tlabel", envelope->tlabel,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (envelope->tlabel);
    envelope->whatodo = gtk_toggle_button_new();
    gtk_widget_ref (envelope->whatodo);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "whatodo",envelope->whatodo,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (envelope->whatodo);
    gtk_box_pack_start (GTK_BOX (hbox2), envelope->whatodo, FALSE, FALSE, 5);
    gtk_container_add (GTK_CONTAINER (envelope->whatodo),envelope->tlabel);
    gtk_signal_connect (GTK_OBJECT (envelope->whatodo), "toggled",
			GTK_SIGNAL_FUNC (change_operation),envelope);
    
    //////////////////
    envelope->clabel = gtk_label_new ("linear curve");
    gtk_widget_ref (envelope->clabel);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "clabel", envelope->clabel,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (envelope->clabel);
    envelope->curve_type = gtk_toggle_button_new();
    gtk_widget_ref (envelope->curve_type);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "curve_type",envelope->curve_type,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (envelope->curve_type);
    gtk_box_pack_start (GTK_BOX (hbox2), envelope->curve_type, FALSE, FALSE, 5);
    gtk_container_add (GTK_CONTAINER (envelope->curve_type),envelope->clabel);
    gtk_signal_connect (GTK_OBJECT (envelope->curve_type), "toggled",
			GTK_SIGNAL_FUNC (change_curve_type),envelope);
    
    gtk_signal_connect(GTK_OBJECT(envelope->curve),"motion_notify_event",
		       (GtkSignalFunc)ismoving, envelope);
     gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			 GTK_SIGNAL_FUNC (curve_reset),envelope);
  }//we do this only for amplitude/freq. edit
  
  button3 = gtk_button_new_with_label (("OK"));
  gtk_widget_ref (button3);
  gtk_object_set_data_full (GTK_OBJECT (Wedit), "button3", button3,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button3);
  gtk_box_pack_start (GTK_BOX (hbox6), button3, TRUE, TRUE, 5);
  
 
  gtk_widget_add_events(GTK_WIDGET(GTK_DRAWING_AREA(envelope->curve)), GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK
			| GDK_BUTTON_RELEASE_MASK);

  
  switch(which_one) {

  case AMP_EDIT: 
    gtk_my_curve_set_range (GTK_MY_CURVE (envelope->curve), 0.,CURVE_LENGTH, envelope->ymin, envelope->ymax);
    gtk_window_set_title (GTK_WINDOW (Wedit), ("Amplitude  Edit"));
    gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			GTK_SIGNAL_FUNC (do_amedit),envelope);       
    break;
    
  case FRE_EDIT:
    gtk_my_curve_set_range (GTK_MY_CURVE (envelope->curve), 0.,CURVE_LENGTH, envelope->ymin, envelope->ymax);
    if(FILE_HAS_PHASE) {
      gtk_window_set_title (GTK_WINDOW (Wedit), ("Frequency Edit(WARNING:you will loose phase data)"));
    }
    else {  
      gtk_window_set_title (GTK_WINDOW (Wedit), ("Frequency Edit"));
    }
    gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			GTK_SIGNAL_FUNC (do_fredit),envelope);
    break;
  
  case TIM_EDIT:

    gtk_label_set_text(GTK_LABEL(label8),"Analysis start time(secs.)=");    
    gtk_label_set_text(GTK_LABEL(label10),"Analysis end time(secs.) =");   
   
    gtk_entry_set_editable (GTK_ENTRY (envelope->minentry),FALSE);
    gtk_entry_set_editable (GTK_ENTRY (envelope->maxentry),FALSE);
    //gtk_signal_connect (GTK_OBJECT (envelope->minentry), "changed",
    //			  GTK_SIGNAL_FUNC (setmin_time),envelope);
    //gtk_signal_connect (GTK_OBJECT (envelope->maxentry), "changed",
    //		      GTK_SIGNAL_FUNC (setmax_time),envelope);
  
    ///////////////////////////
    //here we set the range for the curve
    //the min vertical(y) and min horizontal(x) are set to 0.
    //the max vertical(y) and max horizontal(x) are set to CURVE_LENGTH 
    //we must interpolate BOTH dimentions when using their  values
    gtk_my_curve_set_range (GTK_MY_CURVE (envelope->curve), 
			    0.,CURVE_LENGTH, 0., CURVE_LENGTH);
    //////////////////////////
    gtk_window_set_title (GTK_WINDOW (Wedit), ("Time Synthesis function "));    
    gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			GTK_SIGNAL_FUNC (cancel_wedit),Wedit);//we just return doing nothing  
    gtk_signal_connect(GTK_OBJECT(envelope->curve),"motion_notify_event",
		       (GtkSignalFunc)ismoving_time, envelope);
    gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			GTK_SIGNAL_FUNC (curve_reset_time),envelope);
   
    label9 = gtk_label_new ("Output duration(secs.)    =");
    gtk_widget_ref (label9);
   
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "label9", label9,
			      (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label9);
    
    envelope->durentry= gtk_entry_new();
    gtk_widget_set_usize (envelope->durentry, 75, 25);
    gtk_widget_ref (envelope->durentry);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "durentry",envelope->durentry,
			      (GtkDestroyNotify) gtk_widget_unref);
    
    gtk_widget_show (envelope->durentry);
   
    set_time_env(envelope, TRUE);

    hbox9 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox9);
    gtk_object_set_data_full (GTK_OBJECT (Wedit), "hbox9", hbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox9);
    gtk_box_pack_start (GTK_BOX (vbox3), hbox9, TRUE, FALSE, 0);
    
    gtk_box_pack_start (GTK_BOX (hbox9), label9, TRUE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox9), envelope->durentry, FALSE, FALSE, 0);
    
    gtk_signal_connect (GTK_OBJECT (envelope->durentry), "changed",
			  GTK_SIGNAL_FUNC (change_duration),envelope);
    break;
  }

  gtk_widget_show (envelope->curve);

  return(Wedit);
}


