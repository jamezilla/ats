/*
ABOUT.C
Oscar Pablo Di Liscia / Juan Pampin
*/
#include "atsh.h"

void about(void)
{
  GtkWidget *label, *window, *box;
  gchar str[10] = "ATSH ";

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title(GTK_WINDOW(window), "About ATSH");
  gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 20);
  gtk_container_add(GTK_CONTAINER(window), box);

  label = gtk_label_new(strcat((char *)str, VERSION));
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  label = gtk_label_new("\nDeveloped by:\n" \
                        "Oscar Pablo Di Liscia - odiliscia@unq.edu.ar\n" \
                        "Pete Moss - petemoss@u.washington.edu\n"       \
                        "Juan Pampin - pampin@u.washington.edu\n\n"     \
                        "This Program is partially supported by:\n"     \
                        "Universidad Nacional de Quilmes, Argentina\n"  \
                        "http://www.unq.edu.ar/cme\n"                   \
                        "Center for Digital Arts and Experimental Media (DxArts)\n" \
                        "University of Washington, Seattle\n"          \
                        "http://www.washington.edu/dxarts");
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  gtk_widget_show (box);
  gtk_widget_show(window);
}




