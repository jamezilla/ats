/*
ATS-HEADER.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"
extern char ats_title[];
extern int floaded;

void show_header (void)
{
  GtkWidget *box, *window, *label;
  int siz, fra, len;
  char str[50];

  if(floaded) {
    siz = sizeof(double);
    fra = (int)atshed->fra;
    len = sizeof(ATS_HEADER) + (siz * fra) + 
      ((int)atshed->par * fra * siz * (FILE_HAS_PHASE ? 3 : 2)) +
      (FILE_HAS_NOISE ? (ATSA_CRITICAL_BANDS * fra * siz) : 0);

    window = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), "ATS File Header Data");
    gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    box = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);
    gtk_container_add(GTK_CONTAINER(window), box);

    sprintf(str, "Filename: %s", ats_title);
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"File Size: %d bytes",len); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Sample Rate: %d Hz",(int)atshed->sr); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Frame Size: %d samples (%4.3f s)", (int)atshed->fs, atshed->fs/atshed->sr); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Window Size: %d",(int)atshed->ws); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Partials per Frame: %d",(int)atshed->par); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Frames: %d",(int)atshed->fra); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Duration: %7.3f s",(float)atshed->dur); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Maximum Amplitude: %5.3f",(float)atshed->ma); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Maximum Frequency: %7.2f Hz",(float)atshed->mf); 
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    sprintf(str,"Type: %d",(int)atshed->typ); 
    switch ((int)atshed->typ) {
    case 1:
      strcat(str, " (Amplitude and Frequency)");
      break;
    case 2:
      strcat(str, " (Amplitude, Frequency, and Phase)");
      break;
    case 3:
      strcat(str, " (Amplitude, Frequency, and Noise)");
      break;
    case 4:
      strcat(str, " (Amplitude, Frequency, Phase, and Noise)");
      break;  
    }
    label = gtk_label_new(str);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
    gtk_widget_show(label);
      
    gtk_widget_show (box);
    gtk_widget_show(window);
  }
}

