/*
PROGRESS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"

GtkWidget *progressbar, *progresswindow;
int nLastPct;


void StartProgress(char *message, int canstop)
{
  GtkWidget *label, *table, *sbut, *hseparator2;
  //GtkWidget *align;
  // fprintf(stderr,"progress\n");
  nLastPct = -1;
  //  bProgressUp = TRUE;

  progresswindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  //  g_signal_connect(G_OBJECT (progresswindow), "delete_event", G_CALLBACK((gint)CanWindowClose), pdata);
  g_signal_connect (G_OBJECT (progresswindow), "destroy", G_CALLBACK (EndProgress), NULL);
  gtk_container_border_width(GTK_CONTAINER (progresswindow),10);

  table = gtk_table_new(4,2,TRUE);
  gtk_container_add(GTK_CONTAINER(progresswindow),table);

  label = gtk_label_new(message);
  gtk_table_attach_defaults (GTK_TABLE(table),label,0,2,0,1);
  gtk_widget_show(label);

  //  align = gtk_alignment_new (0.5, 0.5, 0, 0);
  progressbar = gtk_progress_bar_new();
  gtk_table_attach_defaults(GTK_TABLE(table),progressbar,0,2,1,2);
  gtk_widget_show(progressbar);

  if(canstop) { 
    hseparator2 = gtk_hseparator_new ();
    //    gtk_widget_ref (hseparator2);
//     gtk_object_set_data_full (GTK_OBJECT (progresswindow), "hseparator2", hseparator2,
//                               (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hseparator2);
    gtk_table_attach (GTK_TABLE (table), hseparator2, 0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
   
    sbut = gtk_button_new_with_label ("stop me...!");
    //    gtk_widget_ref (sbut);
//     gtk_object_set_data_full (GTK_OBJECT (progresswindow), "sbut", sbut,
//                               (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sbut);
    gtk_table_attach (GTK_TABLE (table), sbut, 0, 2, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    g_signal_connect (G_OBJECT (sbut), "clicked",G_CALLBACK(stop_process),NULL);
  }
  
  gtk_window_set_position(GTK_WINDOW(progresswindow),GTK_WIN_POS_CENTER);
  gtk_widget_show(table);
  //  gtk_grab_add (progresswindow);
  gtk_widget_show(progresswindow);

}
////////////////////////////////////////////////////////////////////
void UpdateProgress(int pos, int len)
{
  gfloat pvalue;
  int pct;
  if(pos <= len) {
    //    fprintf(stderr,"%d %d\n", pos, len);
    pvalue = (gfloat)pos / (gfloat)len;
    pct = pvalue * 100;
    if(nLastPct != pct) {
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progressbar),pvalue);
      while(gtk_events_pending()) gtk_main_iteration();
      nLastPct = pct;
    }
  }
}
////////////////////////////////////////////////////////////////////
void EndProgress (void)
{
  //bProgressUp = FALSE;
  gtk_widget_destroy(progresswindow);
  //free(pdata);
}
////////////////////////////////////////////////////////////////////
// gint CanWindowClose(GtkWidget *widget)
// {
//   return(bProgressUp);
// }
/////////////////////////////////////////////////////////////////
void stop_process(void)
{
  stopper=TRUE;
}
