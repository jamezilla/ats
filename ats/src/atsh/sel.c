
/*
ATSH-SEL.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "atsh.h"

extern GtkWidget     *main_graph;
extern VIEW_PAR h, v;
extern int floaded;
extern SELECTION selection, position;
extern ATS_SOUND *ats_sound;
extern ATS_HEADER atshed;

///////////////////////////////////////////////////////
void set_met(GtkWidget *widget, gpointer data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active) 
    {/*If control reaches here, the toggle button is down */                      
	sdata->met=TRUE;
   	gtk_label_set_text(GTK_LABEL(data),"RMS Power"); 
    } 

  else {/* If control reaches here, the toggle button is up */
    	sdata->met=FALSE;
	gtk_label_set_text(GTK_LABEL(data),"   Peak  ");
  }
  return;
}
///////////////////////////////////////////////////////
void get_values(GtkWidget *widget, gpointer data)
{
  int whichparam=GPOINTER_TO_INT(data); 

 
  switch(whichparam) { 
  case 1:
    
    sdata->from=(int)GTK_ADJUSTMENT(widget)->value;
    break;
  case 2:
    sdata->to=(int)GTK_ADJUSTMENT(widget)->value;
    break;
  case 3:
    sdata->step=(int)GTK_ADJUSTMENT(widget)->value;
    break;
  case 4:
    sdata->tres=(float)GTK_ADJUSTMENT(widget)->value;
    break;   
  }
  
  return;
}

///////////////////////////////////////////////////////
void do_smartsel (GtkWidget *widget, gpointer data)
{
  int i;
  float linamp= (float)pow(10., (double)(sdata->tres/20.));
  float amp, ampsum=0., rmspow, max_peak=0.;
  int x, temp;

  if(sdata->from >  sdata->to) {
    SWAP_INT(sdata->from,sdata->to)
  }

  set_selection(h.viewstart, h.viewend,0,main_graph->allocation.width,
		     main_graph->allocation.width);
  vertex1=0; vertex2=1; //something IS selected
  //set_avec(selection.to - selection.from);

  for(i=0; i<(int)atshed.par; i++) { //unselect all
    selected[i]=FALSE;
  }

  for(i=sdata->from; i < sdata->to+1; i+=sdata->step){

    if(i >(int)atshed.par-1 || i >(int)sdata->to) break;
    
    for(x=h.viewstart; x < h.viewend; x++) {  //amplitude evaluation
      amp    =ats_sound->amp[i][x];
      switch(sdata->met) {
      case 0: //peak
	if(amp > max_peak) {
	  max_peak=amp;
	}           
	break;
      case 1: //RMS POW
	ampsum+= amp*amp; 
	break;
      }
    }
   
    switch(sdata->met) {
    case 0: //peak
      if(max_peak >= linamp) {
	selected[i]=TRUE;	
      }
      break;
    case 1: //RMS POW
      rmspow= (float)sqrt((double)ampsum / (double)h.diff ); 
      if(rmspow >= linamp) {
	selected[i]=TRUE;	
      }	
      break;
    }
    

    ampsum=max_peak=0.;

  }

  draw_pixm();
  gtk_widget_destroy (GTK_WIDGET (data));
  return;

}
///////////////////////////////////////////////////////
void destroy_smartsel (GtkWidget *widget, gpointer data)
{
  //we do not retrieve any data

  gtk_widget_destroy (GTK_WIDGET (data));
  return;

}

//////////////////////////////////////////////////////
GtkObject *create_adj(GtkWidget *window, GtkWidget *table, float min, float max, float value,
		      int p1, int p2, int p3, int p4, char *ID)
{
  GtkObject *adj;
  GtkWidget *spin;  
  char *str;
  int flag;
  
  str=(char*)malloc(32*sizeof(char));
  *str=0;
  strcat(str, "spin");
  strcat(str, ID);
  flag=atoi(ID);

  adj  = gtk_adjustment_new(value, min, max, 1. ,10., 10.);
  spin = gtk_spin_button_new(GTK_ADJUSTMENT (adj), 1, 0);

  gtk_widget_ref (spin);
  gtk_object_set_data_full (GTK_OBJECT (window), "spin", spin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (spin);
  gtk_table_attach (GTK_TABLE (table), spin, p1, p2, p3, p4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (spin, 75, 22);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);

  gtk_signal_connect (GTK_OBJECT (adj) ,"value_changed", GTK_SIGNAL_FUNC(get_values),GINT_TO_POINTER(flag));

  free(str);
  return(adj);

}
//////////////////////////////////////////////////////

void create_sel_dlg (void)
{
  GtkWidget *window1;
  GtkWidget *table1;

  GtkWidget *lfrom;
  GtkWidget *lto; 
  GtkWidget *lstep;
  GtkWidget *ltres;
  GtkWidget *lmet;

  GtkWidget *OK;
  GtkWidget *cancel;
  GtkWidget *metb;

  GtkWidget *hseparator2;
  GtkWidget *hseparator1;
  
  GtkObject *step_adj; 
  GtkObject *to_adj;
  GtkObject *from_adj;
  GtkObject *tres_adj;

  GtkWidget *tlabel;

  if(floaded==FALSE) {return;}
 
  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_container_set_border_width (GTK_CONTAINER (window1), 10);
  gtk_window_set_title (GTK_WINDOW (window1),("Selection "));
  gtk_window_set_position (GTK_WINDOW (window1), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window1), 180, 180);
  gtk_window_set_policy (GTK_WINDOW (window1), FALSE, FALSE, FALSE);
  gtk_signal_connect (GTK_OBJECT (window1), "destroy",
			GTK_SIGNAL_FUNC (destroy_smartsel),window1);

  table1 = gtk_table_new (7, 2, FALSE);
  gtk_widget_ref (table1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "table1", table1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (window1), table1);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);

/////////LABELS/////////////////////////////////////////////////
  lfrom=create_label("From Partial #",     0,1,0,1, window1 ,table1, "1");
  lto  =create_label("To   Partial #",     0,1,1,2, window1 ,table1, "2");
  lstep=create_label("Jump by       ",     0,1,2,3, window1 ,table1, "3");
  ltres=create_label("Amp. Treshold(dB)",  0,1,3,4, window1 ,table1, "4");
  lmet =create_label("Amp. Evaluation" ,   0,1,4,5, window1 ,table1, "5");

///////////////BUTTONS////////////////////   
  
  tlabel = gtk_label_new ("");
  gtk_widget_show (tlabel);

  metb= gtk_toggle_button_new();
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(metb),sdata->met);
  gtk_signal_connect (GTK_OBJECT (metb), "toggled",
		      GTK_SIGNAL_FUNC (set_met),tlabel);
  gtk_container_add (GTK_CONTAINER (metb), tlabel);
  gtk_table_attach (GTK_TABLE (table1), metb, 1, 2, 4, 5,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (metb, 72, 32);
  set_met(GTK_WIDGET(metb), tlabel);
  gtk_widget_show (metb);
 

  OK = gtk_button_new_with_label (("Do It"));
  gtk_widget_ref (OK);
  gtk_object_set_data_full (GTK_OBJECT (window1), "OK", OK,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (OK);
  gtk_table_attach (GTK_TABLE (table1), OK, 1, 2, 6, 7,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (OK, 72, 32);
 
  cancel = gtk_button_new_with_label (("Forget It"));
  gtk_widget_ref (cancel);
  gtk_object_set_data_full (GTK_OBJECT (window1), "cancel", cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancel);
  gtk_table_attach (GTK_TABLE (table1), cancel, 0, 1, 6, 7,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cancel, 72, 32);
  
  
  hseparator2 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator2", hseparator2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator2);
  gtk_table_attach (GTK_TABLE (table1), hseparator2, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 10);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 10);

  
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

  
  from_adj=create_adj(window1, table1, 1., atshed.par,  sdata->from,  1,2,0,1, "1");
  to_adj  =create_adj(window1, table1, 1., atshed.par,  sdata->to  ,  1,2,1,2, "2");
  step_adj=create_adj(window1, table1, 1., atshed.par-1,sdata->step,  1,2,2,3, "3");
  tres_adj=create_adj(window1, table1, -120., 0.,sdata->tres,          1,2,3,4, "4");

  gtk_signal_connect (GTK_OBJECT (OK), "clicked",GTK_SIGNAL_FUNC (do_smartsel),window1);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",GTK_SIGNAL_FUNC (destroy_smartsel),window1);

  gtk_grab_add (window1);
  gtk_widget_show (window1);

  return;
}

