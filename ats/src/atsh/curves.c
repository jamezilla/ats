/*
ATSH-CURVES.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"
//#include "my_curve.h"

extern SPARAMS sparams;
extern short smr_done;
extern int scale_type;
extern SELECTION selection, position;
float *avec = NULL, *fvec = NULL;
int aveclen = 0;
extern ATS_SOUND *ats_sound;
extern ATS_HEADER atshed;

void set_avec(int len);
void hidewindow(GtkWidget *widget);
void cancel_wedit(GtkWidget *widget, gpointer *data);

void set_avec(int len)
{  //selection.to - selection.from
  aveclen=len + 1;
  
  avec=(float*)realloc(avec, sizeof(float) * aveclen);   
  fvec=(float*)realloc(fvec, sizeof(float) * aveclen); 
}

/////////////////////////////////////////////////////////////////////////////
int get_nbp(GtkWidget *curve) 
{
  //returns the number of breakpoints of the curve
  return(GTK_CURVE (curve)->num_ctlpoints);
}
////////////////////////////////////////////////////////////////////////////
float get_x_value(GtkWidget *curve, int breakpoint) 
{
  //returns the x value of the curve at the given breakpoint 
  return((float)(GTK_CURVE(curve)->ctlpoint[breakpoint][0]));
}
////////////////////////////////////////////////////////////////////////////
float get_y_value(GtkWidget *curve, int breakpoint) 
{
  //returns the x value of the curve at the given breakpoint 
  return((float)(GTK_CURVE(curve)->ctlpoint[breakpoint][1]));
}
/////////////////////////////////////////////////////////////////////////////
void change_curve_type(GtkWidget *widget, ENVELOPE *data)
{
  if(data->type_eat) data->type_eat = 0;
  else {
    data->type_eat = 1;
    if(data->type_flag) {
      data->type_flag=FALSE;
      gtk_curve_set_curve_type(GTK_CURVE (data->curve),GTK_CURVE_TYPE_SPLINE);
    } else {
      data->type_flag=TRUE;
      gtk_curve_set_curve_type(GTK_CURVE (data->curve),GTK_CURVE_TYPE_LINEAR);
    }
  }
}
/////////////////////////////////////////////////////////////////////////////
void change_operation(GtkWidget *widget, ENVELOPE *data)
{
  if(data->op_eat) data->op_eat = 0;
  else {
    data->op_eat = 1;
    if(data->op_flag) data->op_flag = FALSE;
    else data->op_flag = TRUE;
  }
}
/////////////////////////////////////////////////////////////////////////////
void hidewindow(GtkWidget *widget)
{
  gtk_widget_hide(GTK_WIDGET(widget));
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
  
  range=GTK_CURVE(widget)->max_y - GTK_CURVE(widget)->min_y;
  val= GTK_CURVE(widget)->min_y +(range - (( range / height ) * (float)ypos));
  fra =selection.from + (int) (((float)aveclen / width) * (float)xpos);
  
  sprintf(info,"Frame=%d Time=%7.2f Value=%7.2f",fra+1,ats_sound->time[0][fra],val);
  //  gtk_label_set_text(GTK_LABEL(data->info_label),info);
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
  //  gtk_label_set_text(GTK_LABEL(data->info_label),info);
}
/////////////////////////////////////////////////////////////////////////////
void setmin_time(GtkWidget *widget, ENVELOPE *data)
{
  gchar *str;
  // char *str=(char*)malloc(13 * sizeof(char)); 
  str = gtk_editable_get_chars(GTK_EDITABLE(data->minentry),0,13);
  data->ymin=(float)atof(str);
  
  if(data->ymin < 0. || data->ymin >= data->ymax ) {
    data->ymin= 0.; //no negative values allowed for time
    sprintf(str," %12.5f",data->ymin);      
    gtk_entry_set_text(GTK_ENTRY (data->minentry),str);
  }  

  g_free(str);
}
/////////////////////////////////////////////////////////////////////////////
void setmax_time(GtkWidget *widget, ENVELOPE *data)
{
 
  float val, stime;
  //  char *str=(char*)malloc(13 * sizeof(char)); 
  gchar *str; // = NULL;
  
  str=gtk_editable_get_chars(GTK_EDITABLE(data->maxentry),0,13);

  val=(float)atof(str);
  data->ymax=val;
  sprintf(str," %12.5f",ats_sound->time[0][(int)atshed.fra-1] );
  stime=(float)atof(str);
  
  if(val  > stime || val <= 0.) {
    //    if(str == NULL) str=(gchar*)malloc(13 * sizeof(gchar));
    data->ymax=stime; //no values beyond the lenght of the analysis allowed
    sprintf(str," %12.5f",data->ymax);
    gtk_entry_set_text(GTK_ENTRY(data->maxentry),str);
  }
 
  g_free(str);
}
/////////////////////////////////////////////////////////////////////////////
void change_duration(GtkWidget *widget, ENVELOPE *data)
{
  gchar *str;
  // char *str=(char*)malloc(13 * sizeof(char)); 
  str=gtk_editable_get_chars(GTK_EDITABLE(widget),0,13);
  data->dur = atof(str);
  g_free(str);
}
/////////////////////////////////////////////////////////////////////////////
void setmax(GtkWidget *widget, ENVELOPE *data)
{
  gchar *str;
  //  char *str=(char*)malloc(10 * sizeof(char)); 
  str = gtk_editable_get_chars(GTK_EDITABLE(data->maxentry), 0, 9);
  data->ymax=(float)atof((char*)str);
  gtk_curve_set_range (GTK_CURVE (data->curve),0.,CURVE_LENGTH, data->ymin, data->ymax);
  g_free(str);
}
/////////////////////////////////////////////////////////////////////////////
void setmin(GtkWidget *widget, ENVELOPE *data)
{
  gchar *str;
  //  char *str=(char*)malloc(10 * sizeof(char)); 
  str = gtk_editable_get_chars(GTK_EDITABLE(data->minentry), 0, 9);
  data->ymin=(float)atof((char*)str);
  gtk_curve_set_range (GTK_CURVE (data->curve), 0., CURVE_LENGTH, data->ymin, data->ymax);
  g_free(str);
}
/////////////////////////////////////////////////////////////////////////////
void do_fredit (GtkWidget *widget, ENVELOPE *data)
{
  int i,x;
  float nyq = atshed.sr / 2.;
  int nValue=0, todo;

  StartProgress("Changing Frequency...",FALSE);
  set_avec(selection.to - selection.from);
  todo = aveclen * (int)atshed.par;
  gtk_curve_get_vector(GTK_CURVE(data->curve), aveclen, fvec);
  
  backup_edition(FRE_EDIT);
  if(FILE_HAS_NOISE) atshed.typ = 3.;    //delete phase information
  else atshed.typ = 1.; 
  
  sparams.upha=FALSE;
 
  for(i=0; i < (int)atshed.par;  ++i) 
    for(x=0; x < aveclen; ++x) {
      if(selected[i]) {
	if(data->op_flag) ats_sound->frq[i][x+selection.from]*=(double)fabs(fvec[x]); //scale - no negative values allowed
	else ats_sound->frq[i][x+selection.from]+=(double)fvec[x]; //ADD

	if(ats_sound->frq[i][x+selection.from] > nyq) ats_sound->frq[i][x+selection.from]=nyq; 
	//clip freq. if out of range 
	if(ats_sound->frq[i][x+selection.from] < 0.) ats_sound->frq[i][x+selection.from]=0.;
        // if(ats_sound->frq[i][x+selection.from] > atshed.mf) ats_sound->frq[i][x+selection.from]=atshed.mf;
	// either this one or the one below stays
      }
      if(ats_sound->frq[i][x+selection.from] > atshed.mf) atshed.mf=ats_sound->frq[i][x+selection.from]; //update max.freq.
      UpdateProgress(++nValue,todo);
    }
    
  EndProgress();
  if(scale_type==SMR_SCALE)  //smr values are computed only if the user is viewing them
    atsh_compute_SMR(ats_sound,selection.from,selection.to);
  else smr_done=FALSE;

  draw_selection();
  gtk_widget_hide(GTK_WIDGET(fWedit));
}
/////////////////////////////////////////////////////////////////////////////
void do_amedit (GtkWidget *widget, ENVELOPE *data)
{
  int i, x;
  int nValue=0, todo;

  StartProgress("Changing Amplitude...", FALSE);
  set_avec(selection.to - selection.from);
  todo = aveclen * (int)atshed.par;
  gtk_curve_get_vector(GTK_CURVE(data->curve),aveclen,avec);    

  backup_edition(AMP_EDIT);
  for(i=0; i < (int)atshed.par;  ++i) 
    for(x=0; x < aveclen; ++x) {
      if(selected[i]) {
        if(data->op_flag) ats_sound->amp[i][x+selection.from]*=fabs(avec[x]); //SCALE - no negative values alowwed
        else ats_sound->amp[i][x+selection.from]+=avec[x]; //ADD

        if(ats_sound->amp[i][x+selection.from] > 1.) ats_sound->amp[i][x+selection.from]=1.; 
        //clip to normalized values
        if(ats_sound->amp[i][x+selection.from] < 0.) ats_sound->amp[i][x+selection.from]=0.;
      }
      if(ats_sound->amp[i][x+selection.from] > atshed.ma) atshed.ma=ats_sound->amp[i][x+selection.from];  //update max.amp.
      UpdateProgress(++nValue,todo);
    }
  
  EndProgress();
  if(scale_type==SMR_SCALE)  //smr values are computed only if the user is viewing them
    atsh_compute_SMR(ats_sound,selection.from , selection.to);
  else smr_done=FALSE;

  draw_selection();
  gtk_widget_hide(GTK_WIDGET(aWedit));
}
/////////////////////////////////////////////////////////////////////////////
void cancel_wedit(GtkWidget *widget, gpointer *data)
{
  gtk_widget_hide(GTK_WIDGET(data)); //data is the Window 
}

/////////////////////////////////////////////////////////////////////////////

void curve_reset(GtkWidget *widget, ENVELOPE *data)
{
  char str[10];  
  data->ymin=0.;
  data->ymax=1.; 
  
  gtk_curve_set_range (GTK_CURVE (data->curve), 0.,CURVE_LENGTH, data->ymin, data->ymax);
  sprintf(str," %10.3f",data->ymax);      
  gtk_entry_set_text(GTK_ENTRY (data->maxentry),str);
  sprintf(str," %10.3f",data->ymin);      
  gtk_entry_set_text(GTK_ENTRY (data->minentry),str);
}
//////////////////////////////////////////////////////////////////////////
void curve_reset_time(GtkWidget *widget, ENVELOPE *data)
{
  set_time_env(data, TRUE);
  gtk_curve_set_range (GTK_CURVE (data->curve), 
			    0.,CURVE_LENGTH, 0.,CURVE_LENGTH);
}
//////////////////////////////////////////////////////////////////////////
GtkWidget *create_edit_curve(int which_one, ENVELOPE *envelope)
{
  GtkWidget *Wedit, *main_box, *radio1, *radio2, *frame;
  GtkWidget *vbox_ops, *vbox_data, *hbox_buttons;
  GtkWidget *hbox, *vbox, *hbox4,*hbox6,*hbox7, *hbox8 , *hbox9;
  GtkWidget *label8, *label9, *label10;
  GtkWidget *button, *button2, *button3;

  envelope->op_flag = envelope->type_flag = TRUE;
  envelope->type_eat = envelope->op_eat = TRUE;

  Wedit = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (GTK_WIDGET (Wedit), 400, 400);
  gtk_window_set_position(GTK_WINDOW(Wedit),GTK_WIN_POS_CENTER);
  g_signal_connect (G_OBJECT (Wedit), "delete_event",G_CALLBACK(hidewindow), NULL);

  main_box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (main_box);
  gtk_container_add (GTK_CONTAINER (Wedit), main_box);

  envelope->statusbar = gtk_statusbar_new();
  envelope->context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(envelope->statusbar), "");
  gtk_box_pack_end (GTK_BOX (main_box), envelope->statusbar, FALSE, FALSE, 0);
  //  gtk_statusbar_push(GTK_STATUSBAR(envelope->statusbar), context_id, "");
  gtk_widget_show(envelope->statusbar);
 
  envelope->curve = gtk_curve_new ();
  //  gtk_widget_set_size_request (envelope->curve, 2, 200);
  gtk_box_pack_start (GTK_BOX (main_box), envelope->curve, TRUE, TRUE, 0);
  gtk_curve_reset (GTK_CURVE (envelope->curve));
  
  //this function produces an error ("Glib-error: could not allocate -20 bytes")	 
  //  gtk_curve_set_curve_type(GTK_CURVE (envelope->curve),GTK_CURVE_TYPE_LINEAR);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_end(GTK_BOX (main_box), hbox, FALSE, FALSE, 0);

  vbox_ops = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_ops);
  gtk_box_pack_start (GTK_BOX (hbox), vbox_ops, FALSE, FALSE, 0);

  vbox_data = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_data);
  gtk_box_pack_start (GTK_BOX (hbox), vbox_data, FALSE, FALSE, 0);

  hbox_buttons = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox_buttons);
  gtk_box_pack_end (GTK_BOX (hbox), hbox_buttons, FALSE, FALSE, 0);

//   hbox2 = gtk_hbox_new (FALSE, 0);
//   gtk_widget_show (hbox2);
//   gtk_box_pack_start (GTK_BOX (vbox_ops), hbox2, FALSE, FALSE, 20);

//   vbox3 = gtk_vbox_new (FALSE, 0);
//   gtk_widget_show (vbox3);
//   gtk_box_pack_start (GTK_BOX (hbox), vbox3, FALSE, FALSE, 0);

//   hbox7 = gtk_hbox_new (FALSE, 0);
//   gtk_widget_show (hbox7);
//   gtk_box_pack_start (GTK_BOX (vbox3), hbox7, TRUE, FALSE, 0);
  
  ////////////////////////////////////////////////////////////////
  
  frame = gtk_frame_new("Operation");
  gtk_box_pack_start (GTK_BOX (vbox_ops), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);
  radio1 = gtk_radio_button_new_with_label (NULL, "Multiply");
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Add");
  gtk_box_pack_start (GTK_BOX(vbox), radio1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), radio2, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radio1), "toggled", G_CALLBACK(change_operation), envelope);
  g_signal_connect (G_OBJECT (radio2), "toggled", G_CALLBACK(change_operation), envelope);
  gtk_widget_show(radio1);
  gtk_widget_show(radio2);

  frame = gtk_frame_new("Curve Type");
  gtk_box_pack_start (GTK_BOX (vbox_ops), frame, FALSE, FALSE, 2);
  gtk_widget_show(frame);
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);
  radio1 = gtk_radio_button_new_with_label (NULL, "Linear");
  radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Spline");
  gtk_box_pack_start (GTK_BOX(vbox), radio1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), radio2, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (radio1), "toggled", G_CALLBACK(change_curve_type), envelope);
  g_signal_connect (G_OBJECT (radio2), "toggled", G_CALLBACK(change_curve_type), envelope);
  gtk_widget_show(radio1);
  gtk_widget_show(radio2);

  button = gtk_button_new_with_label ("Reset");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX(hbox_buttons), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(curve_reset), Wedit);

  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX(hbox_buttons), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(cancel_wedit), Wedit);

  button = gtk_button_new_with_label ("OK");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX(hbox_buttons), button, FALSE, FALSE, 5);
  //  g_signal_connect (G_OBJECT(button2), "clicked", G_CALLBACK(cancel_wedit), Wedit);

  switch(which_one) {
  case AMP_EDIT: 
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (do_amedit),envelope);       
    break;
  case FRE_EDIT:
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (do_fredit),envelope);
    break;
  case TIM_EDIT:
    g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(cancel_wedit), Wedit);
    break;
  }



//   label8 = gtk_label_new (NULL);
//   gtk_widget_show (label8);
//   gtk_box_pack_start (GTK_BOX (hbox7), label8, TRUE, FALSE, 0);

//   envelope->minentry = gtk_entry_new ();
//   gtk_widget_show (envelope->minentry);
//   gtk_box_pack_start (GTK_BOX (hbox7), envelope->minentry, FALSE, TRUE, 0);
//   gtk_widget_set_size_request (envelope->minentry, 75, 25);
//   gtk_entry_set_editable (GTK_ENTRY (envelope->minentry),TRUE);

// //   hbox8 = gtk_hbox_new (FALSE, 0);
// //   gtk_widget_show (hbox8);
// //   gtk_box_pack_start (GTK_BOX (vbox3), hbox8, TRUE, TRUE, 0);

//   label10 = gtk_label_new (NULL);
//   gtk_widget_show (label10);
//   gtk_box_pack_start (GTK_BOX (hbox8), label10, TRUE, FALSE, 0);

//   envelope->maxentry = gtk_entry_new ();
//   gtk_widget_show (envelope->maxentry);
//   gtk_box_pack_start (GTK_BOX (hbox8), envelope->maxentry, FALSE, FALSE, 0);
//   gtk_widget_set_size_request (envelope->maxentry, 75, 25);
//   gtk_entry_set_editable (GTK_ENTRY (envelope->maxentry),TRUE);

//   ///here we manage some of the time envelope exceptions
//   if (which_one != TIM_EDIT) {   
//     envelope->ymin=0.;
//     envelope->ymax=1.;
//     gtk_label_set_text(GTK_LABEL(label8),"Minimum");
//     gtk_entry_set_text(GTK_ENTRY (envelope->minentry),"0.0");
//     gtk_label_set_text(GTK_LABEL(label10),"Maximum");
//     gtk_entry_set_text(GTK_ENTRY (envelope->maxentry),"1.0");
//     g_signal_connect (G_OBJECT (envelope->minentry), "changed", G_CALLBACK (setmin),envelope);
//     g_signal_connect (G_OBJECT (envelope->maxentry), "changed", G_CALLBACK (setmax),envelope); 
//   }
  
//   //////////////////////////////////////////////
// //   vbox4 = gtk_vbox_new (FALSE, 0);
// //   gtk_widget_show (vbox4);
// //   gtk_box_pack_start (GTK_BOX (hbox), vbox4, TRUE, TRUE, 0);
// //   gtk_container_set_border_width (GTK_CONTAINER (vbox4), 10);

// //   hbox4 = gtk_hbox_new (FALSE, 0);
// //   gtk_widget_show (hbox4);
// //   gtk_box_pack_start (GTK_BOX (vbox4), hbox4, TRUE, TRUE, 0);

//   ///////////////////////////////////
  
// //   envelope->info_label= gtk_label_new ("");
// //   gtk_widget_show (envelope->info_label);
// //   gtk_box_pack_start (GTK_BOX (hbox4), envelope->info_label, FALSE, FALSE, 0);
  
//   ///////////////////////////////////

//   hbox6 = gtk_hbox_new (FALSE, 0);
//   gtk_widget_show (hbox6);
//   gtk_box_pack_start (GTK_BOX (vbox4), hbox6, TRUE, TRUE, 0);

//   button1 = gtk_button_new_with_label ("reset");
//   gtk_widget_show (button1);
//   gtk_box_pack_start (GTK_BOX (hbox6), button1, TRUE, TRUE, 5);
 
  
//   ////////////////////////////////////////////////////////////////
//   ///BUTTONS
//   if(which_one != TIM_EDIT) {

//     button2 = gtk_button_new_with_label ("Forget it");
//     gtk_widget_show (button2);
//     gtk_box_pack_start (GTK_BOX (hbox6), button2, TRUE, TRUE, 5);
//     g_signal_connect (G_OBJECT (button2), "clicked",
// 			G_CALLBACK (cancel_wedit),Wedit);
//     ////////////// 
//     envelope->tlabel = gtk_label_new ("Scale(multiply)");
//     gtk_widget_show (envelope->tlabel);
//     envelope->whatodo = gtk_toggle_button_new();
//     gtk_widget_show (envelope->whatodo);
//     gtk_box_pack_start (GTK_BOX (hbox2), envelope->whatodo, FALSE, FALSE, 5);
//     gtk_container_add (GTK_CONTAINER (envelope->whatodo),envelope->tlabel);
//     g_signal_connect (G_OBJECT (envelope->whatodo), "toggled",
// 			G_CALLBACK (change_operation),envelope);
    
//     //////////////////
//     envelope->clabel = gtk_label_new ("spline curve");
//     gtk_widget_show (envelope->clabel);
//     envelope->curve_type = gtk_toggle_button_new();
//     gtk_widget_show (envelope->curve_type);
//     gtk_box_pack_start (GTK_BOX (hbox2), envelope->curve_type, FALSE, FALSE, 5);
//     gtk_container_add (GTK_CONTAINER (envelope->curve_type),envelope->clabel);
//     g_signal_connect (G_OBJECT (envelope->curve_type), "toggled",
// 			G_CALLBACK (change_curve_type),envelope);
    
//     g_signal_connect(G_OBJECT(envelope->curve),"motion_notify_event",
//                      G_CALLBACK(ismoving), envelope);
//     g_signal_connect (G_OBJECT (button1), "clicked",
//                       G_CALLBACK (curve_reset),envelope);
//   }//we do this only for amplitude/freq. edit
  
//   button3 = gtk_button_new_with_label (("OK"));
//   gtk_widget_show (button3);
//   gtk_box_pack_start (GTK_BOX (hbox6), button3, TRUE, TRUE, 5);
  
 
//   gtk_widget_add_events(GTK_WIDGET(GTK_DRAWING_AREA(envelope->curve)), GDK_EXPOSURE_MASK
// 			| GDK_LEAVE_NOTIFY_MASK
// 			| GDK_BUTTON_PRESS_MASK
// 			| GDK_POINTER_MOTION_MASK
// 			| GDK_POINTER_MOTION_HINT_MASK
// 			| GDK_BUTTON_RELEASE_MASK);

  
//  switch(which_one) {

//   case AMP_EDIT: 
//     gtk_curve_set_range (GTK_CURVE (envelope->curve), 0.,CURVE_LENGTH, envelope->ymin, envelope->ymax);
//     gtk_window_set_title (GTK_WINDOW (Wedit), ("Amplitude  Edit"));
//     g_signal_connect (G_OBJECT (button3), "clicked", G_CALLBACK (do_amedit),envelope);       
//     break;
    
//   case FRE_EDIT:
//     gtk_curve_set_range (GTK_CURVE (envelope->curve), 0.,CURVE_LENGTH, envelope->ymin, envelope->ymax);
//     if(FILE_HAS_PHASE) gtk_window_set_title (GTK_WINDOW (Wedit), "Frequency Edit (WARNING:you will loose phase data)");
//     else gtk_window_set_title (GTK_WINDOW (Wedit), "Frequency Edit");
//     g_signal_connect (G_OBJECT (button3), "clicked", G_CALLBACK (do_fredit),envelope);
//     break;
  
//   case TIM_EDIT:
//     gtk_label_set_text(GTK_LABEL(label8),"Analysis start time(secs.)=");    
//     gtk_label_set_text(GTK_LABEL(label10),"Analysis end time(secs.) =");   
   
//     gtk_entry_set_editable (GTK_ENTRY (envelope->minentry),FALSE);
//     gtk_entry_set_editable (GTK_ENTRY (envelope->maxentry),FALSE);
//     //g_signal_connect (G_OBJECT (envelope->minentry), "changed",
//     //			  G_CALLBACK (setmin_time),envelope);
//     //g_signal_connect (G_OBJECT (envelope->maxentry), "changed",
//     //		      G_CALLBACK (setmax_time),envelope);
  
//     ///////////////////////////
//     //here we set the range for the curve
//     //the min vertical(y) and min horizontal(x) are set to 0.
//     //the max vertical(y) and max horizontal(x) are set to CURVE_LENGTH 
//     //we must interpolate BOTH dimentions when using their  values
//     gtk_curve_set_range (GTK_CURVE (envelope->curve), 
// 			    0.,CURVE_LENGTH, 0., CURVE_LENGTH);
//     //////////////////////////
//     gtk_window_set_title (GTK_WINDOW (Wedit), ("Time Synthesis function "));    
//     g_signal_connect (G_OBJECT (button3), "clicked", G_CALLBACK (cancel_wedit),Wedit);//we just return doing nothing  
//     g_signal_connect(G_OBJECT(envelope->curve),"motion_notify_event", G_CALLBACK(ismoving_time), envelope);
//     g_signal_connect (G_OBJECT (button1), "clicked", G_CALLBACK (curve_reset_time),envelope);
   
//     label9 = gtk_label_new ("Output duration(secs.)    =");
//     gtk_widget_show (label9);
    
//     envelope->durentry= gtk_entry_new();
//     gtk_widget_set_size_request (envelope->durentry, 75, 25);
//     gtk_widget_show (envelope->durentry);
   
//     set_time_env(envelope, TRUE);

//     hbox9 = gtk_hbox_new (FALSE, 0);
//     gtk_widget_show (hbox9);
//     gtk_box_pack_start (GTK_BOX (vbox3), hbox9, TRUE, FALSE, 0);
    
//     gtk_box_pack_start (GTK_BOX (hbox9), label9, TRUE, FALSE, 0);
//     gtk_box_pack_start (GTK_BOX (hbox9), envelope->durentry, FALSE, FALSE, 0);
    
//     g_signal_connect (G_OBJECT (envelope->durentry), "changed", G_CALLBACK (change_duration),envelope);
//     break;
//   }

  gtk_widget_show (envelope->curve);
  gtk_widget_show(Wedit);
  gtk_curve_set_curve_type(GTK_CURVE (envelope->curve), GTK_CURVE_TYPE_LINEAR);
  return(Wedit);
}


