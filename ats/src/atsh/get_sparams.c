/*
GET_SPARAMS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"
GtkWidget *freq_entry, *sine_entry, *noise_entry, *sr_entry, *file_entry;
extern char out_title[];
extern char ats_title[];
extern SPARAMS sparams;
extern int floaded;
extern SELECTION selection, position;
extern ATS_SOUND *ats_sound;
extern ATS_HEADER atshed;

void allorsel(GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) {
    sparams.allorsel=TRUE;
    if(SOMETHING_SELECTED) {
      sparams.beg=ats_sound->time[0][selection.from];
      sparams.end=ats_sound->time[0][selection.to]+ ats_sound->time[0][1];
    }
  } else {
    sparams.allorsel=FALSE;
    if(SOMETHING_SELECTED) {
      sparams.beg=0.;
      sparams.end=ats_sound->time[0][(int)atshed.fra - 1] + ats_sound->time[0][1];
    }
  }
}

void set_params(void)
{
  gchar *str;

  str=gtk_editable_get_chars(GTK_EDITABLE(sine_entry),0,9);
  sparams.amp=(float)atof((char*)str);
  g_free(str);

  if(FILE_HAS_NOISE) {
    str=gtk_editable_get_chars(GTK_EDITABLE(noise_entry),0,9);
    sparams.ramp=(float)atof((char*)str);
    g_free(str);
  }

  str=gtk_editable_get_chars(GTK_EDITABLE(freq_entry),0,9);
  sparams.freq=(float)atof((char*)str);
  g_free(str);
 
  str=gtk_editable_get_chars(GTK_EDITABLE(sr_entry),0,9);
  sparams.sr=(float)atof((char*)str);
  g_free(str);

  str=gtk_editable_get_chars(GTK_EDITABLE(file_entry),0,-1);
  strcpy(out_title, str);
  g_free(str);

  /* Phase information is not used for synthesis at present */
  sparams.upha=FALSE;
}

//void delete_event (GtkWidget *widget, gpointer data)
//{
// set_params();//retrieve parameters from entry fields
//  // do_synthesis();
//}

void ok_button(GtkWidget *widget, gpointer data)
{
  set_params();  /* retrieve parameters from entry fields */
  gtk_widget_destroy(GTK_WIDGET(data));
  do_synthesis();
}

void cancel_dialog(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
}

void get_sparams(void)
{
  GtkWidget *window, *hbox, *label, *voidbox, *button;
  char str[50];
  
  if(floaded) {
    /* Create the top level window */
    window = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(window), "Synthesis");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);                  
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    /* create the various text entry boxes */
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Sinusoidal output gain");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    sine_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(sine_entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(sine_entry),60,20);
    sprintf(str, "%2.3f", sparams.amp);
    gtk_entry_set_text(GTK_ENTRY(sine_entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), sine_entry, FALSE, FALSE, 0);
    gtk_widget_show(sine_entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Noise output gain");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    noise_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(noise_entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(noise_entry),60,20);
    sprintf(str, "%2.3f", sparams.ramp);
    gtk_entry_set_text(GTK_ENTRY(noise_entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), noise_entry, FALSE, FALSE, 0);
    gtk_editable_set_editable(GTK_EDITABLE(noise_entry), FILE_HAS_NOISE);
    gtk_widget_show(noise_entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Frequency Scalar");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    freq_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(freq_entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(freq_entry),60,20);
    sprintf(str, "%2.3f", sparams.freq);
    gtk_entry_set_text(GTK_ENTRY(freq_entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), freq_entry, FALSE, FALSE, 0);
    gtk_widget_show(freq_entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Sampling Rate");
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,10);
    gtk_widget_show(label);
    sr_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(sr_entry), 10);
    gtk_widget_set_usize(GTK_WIDGET(sr_entry),60,20);
    sprintf(str, "%d", (int)sparams.sr);
    gtk_entry_set_text(GTK_ENTRY(sr_entry), str);
    gtk_box_pack_start(GTK_BOX(hbox), sr_entry, FALSE, FALSE, 0);
    gtk_widget_show(sr_entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);  
    
    /* create a void box as separator */
    voidbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), voidbox, FALSE, FALSE, 3);
    gtk_widget_show(voidbox);
    
//     /* Create the toggle button for Partials */
//     tlabel1 = gtk_label_new ("Selected Partials Only");
//     button= gtk_toggle_button_new();
//     gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button),sparams.allorsel);
//     gtk_signal_connect (GTK_OBJECT (button), "toggled",
//                         GTK_SIGNAL_FUNC (allorsel),NULL);
//     gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), button, FALSE, FALSE, 2);
//     gtk_container_add (GTK_CONTAINER (button), tlabel1);
//     gtk_widget_show (tlabel1);
//     gtk_widget_show (button);
//     allorsel(GTK_WIDGET(button), NULL);

    /* Create the check button for Partials */
    button = gtk_check_button_new_with_label("Selected Partials Only");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), sparams.allorsel);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(allorsel), NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), button, FALSE, FALSE, 2);
    gtk_widget_show (button);
    //    allorsel(GTK_WIDGET(button), NULL);

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
    
    file_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), file_entry, FALSE, FALSE, 2);
    gtk_entry_set_text(GTK_ENTRY(file_entry), out_title);
    gtk_widget_show (file_entry);
    
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
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(cancel_dialog), window);
    gtk_widget_show(button);
    
    /* Make the main window visible */
    gtk_widget_show(window);
  }
}

