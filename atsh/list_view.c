/*
LIST_VIEW.C
Oscar Pablo Di Liscia / Juan Pampin
*/

#include "atsh.h"

extern char *ats_tittle;

/////////////////////////////////////////////////////////////////////////////
void from_now(GtkWidget *widget, GtkAdjustment *adj)
{
  selection->from=(int)adj->value;

  if(selection->from > selection->to ) {selection->from = selection->to;}
  sprintf(ffro," (from frame = %d ", selection->from+1);
  gtk_label_set_text(GTK_LABEL(fr_from),ffro); 
    
  set_avec();
  //backup_edition(SEL_EDIT);

  clist_update(adj);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void to_now(GtkWidget *widget, GtkAdjustment *adj)
{
  selection->to=(int)adj->value;
  if(selection->to < selection->from) {selection->to = selection->from;}
  sprintf(fto," to frame = %d )", selection->to+1);
  gtk_label_set_text(GTK_LABEL(fr_to),fto); 
  
  set_avec();
  //backup_edition(SEL_EDIT);
  clist_update(adj);
  return;
}
/////////////////////////////////////////////////////////////////////////////
void update_time(gfloat valt, gint valf)
{ 

  *n_text=0;
  sprintf(n_text,"%8.3fs  to %8.3fs    ",valt, valt + ats_sound->time[0][1]);
  gtk_label_set_text((GtkLabel*)ti_num, n_text);    
  *n_text=0;
  sprintf(n_text," %d ",valf+1);
  gtk_label_set_text((GtkLabel*)fr_num, n_text); 

return;
}
/////////////////////////////////////////////////////////////////////////////
void redraw_screen()
{
int temp=0, i;
 
 for(i=0; i<(int)atshed->par; i++) { //check whether something is selected
   if (selected[i]==TRUE) {
     ++temp; //it is...
     break;
   }
 }    
 
 if(temp == 0) { //nothing selected...
   vertex1=vertex2=FALSE;
 }   
 else { //something is selected
   vertex1=FALSE;
   vertex2=TRUE;
 }
 draw_pixm();
 //repaint(NULL);
 return;
}
/////////////////////////////////////////////////////////////////////////////
void clist_update(GtkAdjustment *adj)
{  
int indx, loff, x;

loff=(int)adj->value;
update_time(ats_sound->time[0][(int)adj->value], (int)adj->value);
 
 gtk_clist_clear((GtkCList*)clist);
 gtk_clist_freeze((GtkCList*)clist); 
    
 for ( indx=0 ; indx < (int)atshed->par ; indx++ ) {
          
      *nbuf=*abuf=*fbuf=*pbuf=*sbuf=0;
      sprintf(nbuf," %d ",indx+1);
      sprintf(abuf," %8.5f ",ats_sound->amp[indx][loff]);
      sprintf(fbuf," %8.3f ",ats_sound->frq[indx][loff]);
      sprintf(sbuf," %8.3f ",ats_sound->smr[indx][loff]);

      if (FILE_HAS_PHASE) {
	sprintf(pbuf," %8.5f ",ats_sound->pha[indx][loff]);
      }
      
      ats_data[0]=nbuf;
      ats_data[1]=abuf;
      ats_data[2]=fbuf;
      ats_data[3]=sbuf;
      ats_data[4]=pbuf;

      gtk_clist_append( (GtkCList *) clist,ats_data);
      
      if(selected[indx]==1 &&  (int)adj->value >= selection->from && (int)adj->value <=selection->to) {
	for(x = 0; x < 4; ++x) {
	  gtk_clist_select_row((GtkCList*)clist,indx,x);
	}
      }
      
 }
    
 gtk_clist_thaw((GtkCList*)clist);
 set_avec();
 //backup_edition(SEL_EDIT);

return;
}
/////////////////////////////////////////////////////////////////////////////

void delete_window (GtkWidget *widget, gpointer data)
{
int temp=0, i;
 
 for(i=0; i<(int)atshed->par; i++) { //check whether something is selected
    if (selected[i]==TRUE) {
      ++temp;
      break; //it is...
    }
  }    
  
 if(temp == 0) { //nothing selected...
   vertex1=vertex2=FALSE;
  }   
 else { //something is selected
   vertex1=FALSE;
   vertex2=TRUE;
 }


  g_free(nbuf);
  g_free(abuf);
  g_free(fbuf);
  g_free(pbuf);
  g_free(sbuf);
  g_free(n_text);
  g_free(ffro);
  g_free(fto);
  
  draw_pixm();
  //repaint(NULL);
  gtk_widget_destroy (GTK_WIDGET (data));
  

}

/////////////////////////////////////////////////////////////////////////////
void init_str()
{


  nbuf=(gchar*)g_malloc(32*sizeof(gchar));
  abuf=(gchar*)g_malloc(32*sizeof(gchar));
  fbuf=(gchar*)g_malloc(32*sizeof(gchar));
  pbuf=(gchar*)g_malloc(32*sizeof(gchar));
  sbuf=(gchar*)g_malloc(32*sizeof(gchar));
  n_text=(gchar*)g_malloc(32*sizeof(gchar));
  fto=(gchar*)g_malloc(64*sizeof(gchar));
  ffro=(gchar*)g_malloc(64*sizeof(gchar));
  *nbuf=*abuf=*fbuf=*pbuf=*n_text=*fto=*ffro=0;

return;
}

//////////////////////////////////////////////////////////////////////////////
/* If we come here, then the user has selected a row in the list. */
void selection_made(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data )
{
  selected[row]=1;
  return;
}
////////////////////////////////////////////
void unselection_made(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data )
{
  selected[row]=0;
  return;
}
//////////////////////////////////////////////////////////////////////////////
int list_view()
{                                  
    GtkWidget *window;
    GtkWidget *vbox, *hbox1, *hbox2, *hbox3;
    GtkWidget *scrolled_window;
    gchar *titles[5] = { "Partial #", "Amplitude", "Frequency","SMR", "Phase"};
    GtkWidget *sc_tittle, *ti_tittle; 
    GtkObject *adj1;
    GtkWidget *scrollbar;
    GtkWidget *fromb, *tob, *redrawb;
    GtkWidget *hboxb;
    int i;
   
    first=TRUE;

    if(*ats_tittle==0) { //no ATS file data alocated, just ignore the call
      return(0); 
    }
    
    init_str();
    offset=0;
    maxval=(atshed->par > 35.? 35:(int)atshed->par ); //clip the max val for clist to 35.
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(GTK_WIDGET(window),450,22 * maxval);

    gtk_window_set_title(GTK_WINDOW(window), ats_tittle);
    gtk_signal_connect(GTK_OBJECT(window),
		       "destroy",
		       GTK_SIGNAL_FUNC(delete_window),
		       GTK_WIDGET(window));
    
    vbox=gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);
   
    //FRAME LABEL 
    hbox1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
    gtk_widget_show(hbox1);
  
    sc_tittle = gtk_label_new ("Current frame");
    gtk_box_pack_start (GTK_BOX (hbox1), sc_tittle, FALSE, FALSE,10);
    gtk_misc_set_alignment(GTK_MISC(sc_tittle),0.,.8);
    gtk_widget_show (sc_tittle);

    fr_num = gtk_label_new (" 0    ");
    gtk_box_pack_start (GTK_BOX (hbox1), fr_num, FALSE, FALSE,10);
    gtk_misc_set_alignment(GTK_MISC(fr_num),0.,.8);
    gtk_widget_show (fr_num);         

    //vsep=gtk_vseparator_new();
    //gtk_box_pack_start (GTK_BOX (hbox1), vsep, TRUE, TRUE, 30);
    //gtk_widget_show (vsep);

    sprintf(ffro," (from frame = %d ", selection->from+1);
    fr_from = gtk_label_new (ffro);
    gtk_box_pack_start (GTK_BOX (hbox1), fr_from, FALSE, FALSE,5);
    gtk_misc_set_alignment(GTK_MISC(fr_from),0.,.8);
    gtk_widget_show (fr_from);         
    
    sprintf(fto," to frame = %d )", selection->to+1);
    fr_to = gtk_label_new (fto);
    gtk_box_pack_start (GTK_BOX (hbox1), fr_to, FALSE, TRUE, 5);
    gtk_misc_set_alignment(GTK_MISC(fr_to),0.,.8);
    gtk_widget_show (fr_to);         

   //TIME LABEL
    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    gtk_widget_show(hbox2);   
   
    ti_tittle = gtk_label_new ("Current time ");
    gtk_box_pack_start (GTK_BOX (hbox2), ti_tittle, FALSE,FALSE,10);
    gtk_misc_set_alignment(GTK_MISC(ti_tittle),0.,.8);
    gtk_widget_show (ti_tittle);

    ti_num = gtk_label_new ("0. segs.");
    gtk_box_pack_start (GTK_BOX (hbox2), ti_num, FALSE, FALSE,10);
    gtk_misc_set_alignment(GTK_MISC(ti_num),0.,.8);
    gtk_widget_show (ti_num);         
 

    //SCALER   
    adj1 = gtk_adjustment_new (0.0, 0.0,atshed->fra, 1., 10., 1.0);
    hbox3 = gtk_hbox_new(FALSE, 0);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj1), selection->from);
    gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, TRUE, 0);
    gtk_widget_show(hbox3);
    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
   
    gtk_range_set_update_policy (GTK_RANGE (scrollbar),GTK_UPDATE_DISCONTINUOUS);
    gtk_signal_connect (GTK_OBJECT(adj1), "value_changed",
                        GTK_SIGNAL_FUNC (clist_update), NULL);
    gtk_box_pack_start (GTK_BOX (hbox3), scrollbar, TRUE, TRUE, 0);
    
    gtk_widget_show (scrollbar);
    
    /*Create "from","to" and "redraw" buttons */
    hboxb=gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox),hboxb , FALSE, FALSE, 0);
    gtk_widget_show(hboxb);
    
    fromb=gtk_button_new_with_label("From=Now");
    gtk_box_pack_start(GTK_BOX(hboxb), fromb, FALSE, FALSE, 5);
    gtk_widget_show(fromb);
    gtk_signal_connect (GTK_OBJECT (fromb), "clicked",
			GTK_SIGNAL_FUNC (from_now),GTK_OBJECT(adj1));

    tob=gtk_button_new_with_label("To=Now");
    gtk_box_pack_start(GTK_BOX(hboxb), tob, FALSE, FALSE, 5);
    gtk_widget_show(tob);
    gtk_signal_connect (GTK_OBJECT (tob), "clicked",
			GTK_SIGNAL_FUNC (to_now),GTK_OBJECT(adj1));
    
    redrawb=gtk_button_new_with_label("Redraw");
    gtk_box_pack_start(GTK_BOX(hboxb), redrawb, FALSE, FALSE, 5);
    gtk_widget_show(redrawb);
    gtk_signal_connect (GTK_OBJECT (redrawb), "clicked",
			GTK_SIGNAL_FUNC (redraw_screen),NULL);
    
    /////////////////////////////////////////
     /* Create the CList*/
    clist = gtk_clist_new_with_titles( 5, titles);

    /* selection made */
    gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_EXTENDED);
    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		       GTK_SIGNAL_FUNC(selection_made),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(clist), "unselect_row",
		       GTK_SIGNAL_FUNC(unselection_made),
		       NULL);
    gtk_clist_set_shadow_type (GTK_CLIST(clist), GTK_SHADOW_OUT);
    for(i=0; i<5; ++i) {
      gtk_clist_set_column_width (GTK_CLIST(clist), i, 75 );
    }
    
    /* Create a scrolled window to pack the CList widget into */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
 
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_window);
    gtk_container_add(GTK_CONTAINER(scrolled_window), clist);

    /* Add the CList widget to the vertical box and show it. */
    gtk_widget_show(clist);
    clist_update(GTK_ADJUSTMENT(adj1));
    first=FALSE;
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_grab_add (window);
    gtk_widget_show(window);
    
    return(0);
}
