/*
ABOUT.C
Oscar Pablo Di Liscia / Juan Pampin
*/
#include "atsh.h"

///////////////////////////////////////////////////////////////////////
void about(void)
{

  GtkWidget *vbox1;
  GtkWidget *label1;


  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window1),GTK_WIN_POS_CENTER);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_window_set_title (GTK_WINDOW (window1), ("About ATSH"));
  gtk_window_set_policy (GTK_WINDOW (window1), TRUE, TRUE, FALSE);
  gtk_widget_show(window1);
  gtk_signal_connect (GTK_OBJECT (window1), "destroy",
			GTK_SIGNAL_FUNC (hclose),NULL);


  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window1), vbox1);

 
  //////////////////////////////////////////////////////////////
 
  label1 = gtk_label_new ("\n \n  ATSH (Analysis-Transformation-Synthesis-Shell)  \n
  V.2.0, 22/08/2003 \n \n
  Oscar Pablo Di Liscia - odiliscia@unq.edu.ar \n
  Juan Pampin - pampin@u.washington.edu \n 
  Pete Moss - petemoss@u.washington.edu \n
  This Program is partially supported by: \n
  Universidad Nacional de Quilmes (Argentina) \n
  http://www.unq.edu.ar/cme \n
  Center for Digital Arts and Experimental Media (University of Washington, Seattle, USA) \n
  http://www.dxarts.washington.edu/ \n \n
  ");

  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);

  gtk_grab_add (window1);

  return;
}




