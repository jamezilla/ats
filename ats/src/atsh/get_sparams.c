/*
GET_SPARAMS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
GtkWidget     *osfile_label;
extern char *out_tittle;
extern char *ats_tittle;

void allorsel (GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) {
    sparams->allorsel=TRUE;
    if(SOMETHING_SELECTED) {
      sparams->beg=ats_sound->time[0][selection->from];
      sparams->end=ats_sound->time[0][selection->to]+ ats_sound->time[0][1];
    }
  } else {
    sparams->allorsel=FALSE;
    if(SOMETHING_SELECTED) {
      sparams->beg=0.;
      sparams->end=ats_sound->time[0][(int)atshed->fra - 1] + ats_sound->time[0][1];
    }
  }
}

void set_params()
{
// char *str;  

//  str=(char*)malloc(1024 * sizeof(char)); 
//  *str=0;        

//  str=gtk_editable_get_chars(GTK_EDITABLE(entry[0]),0,9);
//  sparams->amp=(float)atof((char*)str);
//  *str=0;

//  if(FILE_HAS_NOISE) {
//    str=gtk_editable_get_chars(GTK_EDITABLE(entry[1]),0,9);
//    sparams->ramp=(float)atof((char*)str);
//    *str=0;

//  }

//  str=gtk_editable_get_chars(GTK_EDITABLE(entry[2]),0,9);
//  sparams->frec=(float)atof((char*)str);
//  *str=0;
 
//  str=gtk_editable_get_chars(GTK_EDITABLE(entry[3]),0,9);
//  sparams->sr=(float)atof((char*)str);
//  *str=0;

//  str=gtk_editable_get_chars(GTK_EDITABLE(osfile_label),0,-1);
//  *out_tittle=0;
//  strcat(out_tittle, str);
//  *str=0;

//  //Phase information is not used for synthesis at present
//  sparams->upha=FALSE;

// free(str);
// return;
}

void delete_event (GtkWidget *widget, gpointer data)
{

  set_params();//retrieve parameters from entry fields
  // do_synthesis();
  
	//free(param_list);
  	//gtk_widget_destroy (GTK_WIDGET (data));

}

void ok_button(GtkWidget *widget, gpointer data)
{
  set_params();  //retrieve parameters from entry fields
  // do_synthesis();
}

void just_quit (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

void get_sparams(void)
{
  GtkWidget *window, *hbox, *label, *voidbox, *button, *entry;
  char str[50];
  
  if(floaded) {
    /* Create the top level window */
    window = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(window), "Synthesis");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);                  
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    
    // --- You should always remember to connect the destroy event
    //     to the main window.
    //    gtk_signal_connect (GTK_OBJECT (window), "destroy",
    //                  GTK_SIGNAL_FUNC (ClosingDialog),&window);
    
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Sinusoidal output gain");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(entry),60,20);
    sprintf(str, "%2.3f", sparams->amp);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Noise output gain");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(entry),60,20);
    sprintf(str, "%2.3f", sparams->ramp);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FILE_HAS_NOISE);
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Frequency Scalar");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(entry),60,20);
    sprintf(str, "%2.3f", sparams->frec);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Sampling Rate");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(entry),60,20);
    sprintf(str, "%d", (int)sparams->sr);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    /* create a void box as separator */
    voidbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), voidbox, FALSE, FALSE, 3);
    gtk_widget_show(voidbox);
    
//     /* Create the toggle button for Partials */
//     tlabel1 = gtk_label_new ("Selected Partials Only");
//     button= gtk_toggle_button_new();
//     gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),sparams->allorsel);
//     gtk_signal_connect (GTK_OBJECT (button), "toggled",
//                         GTK_SIGNAL_FUNC (allorsel),NULL);
//     gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), button, FALSE, FALSE, 2);
//     gtk_container_add (GTK_CONTAINER (button), tlabel1);
//     gtk_widget_show (tlabel1);
//     gtk_widget_show (button);
//     allorsel(GTK_WIDGET(button), NULL);

    /* Create the check button for Partials */
    button = gtk_check_button_new_with_label("Selected Partials Only");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), sparams->allorsel);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(allorsel), NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), button, FALSE, FALSE, 2);
    gtk_widget_show (button);
    allorsel(GTK_WIDGET(button), NULL);

    /* Create the "set time pointer" button */
    button = gtk_button_new_with_label("Set Time Pointer");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), button, FALSE, FALSE, 2);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(edit_tim), NULL);
    gtk_widget_show(button);
    
    /* Create the set output file button */
    button = gtk_button_new_with_label ("Set Output File");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), button, FALSE, FALSE, 2);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(filesel), "outsel");
    gtk_widget_show(button);
    
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), entry, FALSE, FALSE, 2);
    gtk_entry_set_text(GTK_ENTRY(entry), out_tittle);
    gtk_widget_show (entry);
    
    /* create a void box as separator */
    voidbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), voidbox, FALSE, FALSE, 5);
    gtk_widget_show(voidbox);
    
    /* Create standard OK/CANCEL buttons */
    button = gtk_button_new_with_label("Do it");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), button, TRUE, TRUE, 5);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(ok_button), window);
    gtk_widget_show(button);
    
    button = gtk_button_new_with_label ("Forget it");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), button, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(just_quit), window);
    gtk_widget_show(button);
    
    /* Make the main window visible */
    gtk_widget_show(window);
  }
}

