/*
HELP.C
Oscar Pablo Di Liscia / Juan Pampin
*/
#include "atsh.h"
#include "help.h"

void help(void)
{
  GtkWidget *label, *window, *scrolled_window;

  window = gtk_window_new(GTK_WINDOW_DIALOG);
  //  set_window_icon(window);
  gtk_window_set_title(GTK_WINDOW(window), "Help for ATSH");
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize(GTK_WIDGET(scrolled_window), 500, 500);
  gtk_container_add(GTK_CONTAINER(window), scrolled_window);

  label = gtk_label_new(HELP_STR);
  gtk_widget_set_usize(GTK_WIDGET(label), 450, 1500);
  gtk_label_set_justify (GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), label);

  gtk_widget_show_all (window);
}




