/*
PROGRESS.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"


/////////////////////////////////////////////////////////////////

void StartProgress(char *message, int canstop)
{
GtkWidget *label;
GtkWidget *table;
GtkWidget *window;
GtkAdjustment *adj;
GtkWidget *sbut;
GtkWidget *hseparator2;

pdata= g_malloc (sizeof (typProgressData));
pdata->nLastPct= -1;
pdata->bProgressUp=TRUE;

window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
pdata->window=window;

gtk_signal_connect(GTK_OBJECT (window), "delete_event",
              GTK_SIGNAL_FUNC ((gint)CanWindowClose), pdata);
gtk_container_border_width(GTK_CONTAINER (window),10);

table=gtk_table_new(4,2,TRUE);
gtk_container_add(GTK_CONTAINER(window),table);

label=gtk_label_new(message);
gtk_table_attach_defaults (GTK_TABLE(table),label,0,2,0,1);
gtk_widget_show(label);

adj= (GtkAdjustment *) gtk_adjustment_new (0,0,400,0,0,0);
pdata->progressbar= gtk_progress_bar_new_with_adjustment(adj);
gtk_table_attach_defaults(GTK_TABLE(table),pdata->progressbar,0,2,1,2);
gtk_widget_show(pdata->progressbar);

 if(canstop==TRUE) { 
   hseparator2 = gtk_hseparator_new ();
   gtk_widget_ref (hseparator2);
   gtk_object_set_data_full (GTK_OBJECT (window), "hseparator2", hseparator2,
			     (GtkDestroyNotify) gtk_widget_unref);
   gtk_widget_show (hseparator2);
   gtk_table_attach (GTK_TABLE (table), hseparator2, 0, 2, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
   
   sbut = gtk_button_new_with_label (("stop me...!"));
   gtk_widget_ref (sbut);
   gtk_object_set_data_full (GTK_OBJECT (window), "sbut", sbut,
			     (GtkDestroyNotify) gtk_widget_unref);
   gtk_widget_show (sbut);
   gtk_table_attach (GTK_TABLE (table), sbut, 0, 2, 3, 4,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_FILL), 0, 0);
   gtk_signal_connect (GTK_OBJECT (sbut), "clicked",GTK_SIGNAL_FUNC (stop_process),NULL);
 }

gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
gtk_widget_show(table);
 gtk_grab_add (window);
gtk_widget_show(window);

}
////////////////////////////////////////////////////////////////////
void UpdateProgress(int pos, int len)
{
gfloat pvalue;
int pct;

 if(len > 0) {
   pvalue=(gfloat)pos / (gfloat)len;
   pct=pvalue * 100;
   if(pdata->nLastPct != pct) {
     gtk_progress_set_percentage (GTK_PROGRESS (pdata->progressbar),pvalue);
     while(gtk_events_pending()) {
       gtk_main_iteration();
     }
     pdata->nLastPct=pct;
   }
 }
}
////////////////////////////////////////////////////////////////////
void EndProgress ()
{
pdata->bProgressUp=FALSE;
gtk_widget_destroy(pdata->window);
free(pdata);
}
////////////////////////////////////////////////////////////////////
gint CanWindowClose(GtkWidget *widget)
{
  return(pdata->bProgressUp);

}
/////////////////////////////////////////////////////////////////
void stop_process()
{

  stopper=TRUE;
  return;
}
