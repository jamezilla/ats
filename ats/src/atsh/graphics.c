/*
ATSH-GRAPHICS.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"

#define MAX_COLOR_VALUE 65535 //this is the max value the color can hold (unsigned short)



float res_data[ATSA_CRITICAL_BANDS+1]=ATSA_CRITICAL_BAND_EDGES;
extern GtkWidget *main_graph;
extern GtkWidget *label;
extern GtkObject *hadj1,*hadj2;
extern GtkWidget *vscale1, *vscale2;
extern GtkWidget *hruler, *vruler;
extern VIEW_PAR h, v;
extern GtkWidget *statusbar;
extern gint context_id;
extern int floaded;
extern short smr_done;
GdkPixmap *pixmap = NULL;
int view_type = NULL_VIEW, scale_type = AMP_SCALE, interpolated = TRUE;
extern SELECTION selection, position;
extern ATS_SOUND *ats_sound;
extern ATS_HEADER atshed;

//////////////////////////////////////////////////////////////////////////////
gint configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
  if(pixmap != NULL) gdk_pixmap_unref(pixmap);

  pixmap= gdk_pixmap_new(widget->window,
			 widget->allocation.width,
			 widget->allocation.height,
			 -1);
  draw_pen= gdk_gc_new (pixmap);

  if(floaded) draw_pixm();
  else draw_default(widget);
  
  return(1);
}
/////////////////////////////////////////////////////////////////////////////
gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window,
		  widget->style->white_gc,
		  pixmap,
		  event->area.x,event->area.y,
		  event->area.x,event->area.y,
		  widget->allocation.width,
		  widget->allocation.height
		  );
  return(0);
}
////////////////////////////////////////////////////////////////////////////
void change_color (int nRed, int nGreen, int nBlue )
{

  draw_col->red = nRed;
  draw_col->green = nGreen;
  draw_col->blue = nBlue;
  
  gdk_color_alloc (gdk_colormap_get_system (), draw_col);
  gdk_gc_set_foreground (draw_pen, draw_col);
}
////////////////////////////////////////////////////////////////////////////
//THIS FUNCTION IS NOT USED NOW...BUT SHOULD BE USED...NEED TO FIX SOME THINGS
void erase_selection(int pfrom, int pto)//erase previous selection
{
  int  i,j, val, vant;
  float curx, cury, nextx ,nexty;
  float ffac1, ffac2=0.; 

  frame_step=(float)main_graph->allocation.width / ((float)h.diff+1.);
 
  if(NOTHING_SELECTED) { 
     draw_selection_line(selection.x1);	
     return;
  }
  
  
  //we must find out the size and location of the previous selection
  curx= ((float)(pfrom - (h.viewstart-1)) * frame_step); 
  nextx=((float)(pto - (pfrom - 1)) * frame_step);
  gdk_draw_rectangle(pixmap,main_graph->style->white_gc,
  		     1,curx,0,nextx,main_graph->allocation.height);
  cury=nexty=0.;
  nextx=curx;

 //now we redraw ONLY the previous selection 

 for(i=pfrom; i < pto+1; i++) {
  	
   nextx+=frame_step;
	
   for(j=0; j < atshed.par; ++j) {
     if(ats_sound->amp[j][i] > 0. ) {
       if(scale_type==AMP_SCALE) { //using amp values
	 val=MAX_COLOR_VALUE - (int)(pow(ats_sound->amp[j][i], valexp) *(float)MAX_COLOR_VALUE);
       }
       else { //using smr values
	 val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(pow(db2amp_spl(ats_sound->smr[j][i]), valexp) *(float)MAX_COLOR_VALUE);
       }
       
       change_color(val,val,val);
       
       if(ats_sound->frq[j][i] >= (float)v.viewstart && ats_sound->frq[j][i] <= (float)v.viewend) {
	 ffac1=(float)v.diff / (ats_sound->frq[j][i]- (float)v.viewstart); 
	 cury=main_graph->allocation.height -(main_graph->allocation.height / ffac1);
	 if(interpolated == TRUE) { //INTERPOLATING OR NOT
	   if(ats_sound->frq[j][i+1]==0.) nexty = cury; //handle special case
	   else {
	     ffac2=(float)v.diff / (ats_sound->frq[j][i+1]- (float)v.viewstart);
	     nexty=main_graph->allocation.height-(main_graph->allocation.height / ffac2);
	   }
	   gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)nexty);
	 }
	 else {
	   //NEXTY NOT USED
	   gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)cury);
	 }
       }	 
       vant=val; 
     }
   }

   curx+=frame_step;      
 }  


 //mark start of selection
 draw_selection_line( selection.x1);	
 repaint(NULL);
 return;
}
////////////////////////////////////////////////////////////////////////////
void draw_selection()//draw current selection
{
  int  i,j, val, vant;
  float curx, cury, nextx ,nexty;
  float ffac1, ffac2=0.; 
  int offset=10; //we need this offset to erase the selection line

  frame_step=(float)main_graph->allocation.width / ((float)h.diff + 1.);
  //we must find out the size and location of the current selection
  curx= ((float)(selection.from - (h.viewstart-1)) * frame_step); 
  nextx=((float)((selection.to - selection.from)+ offset) * frame_step);
  gdk_draw_rectangle(pixmap,main_graph->style->white_gc,
  		     1,curx,0,nextx,main_graph->allocation.height);

  //now we redraw ONLY the current selection
  cury=nexty=0.;
  nextx=curx;

  for(i=selection.from; i < selection.to+offset; i++) {
    if(i > (int)atshed.fra-1) break; 
    nextx+=frame_step;
    
    for(j=0; j < atshed.par; ++j) {
      
      if(ats_sound->amp[j][i] > 0.) {
	
	if(selected[j]==1 && i >= selection.from && i <= selection.to){
	  if(scale_type==AMP_SCALE) {
	    val=MAX_COLOR_VALUE - (int)(ats_sound->amp[j][i] * (float)MAX_COLOR_VALUE);
	  }
	  else { //using smr values
	    val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(db2amp_spl(ats_sound->smr[j][i]) *(float)MAX_COLOR_VALUE);
	  }
	  change_color(val,0,0);
	}
	else {
	  if(scale_type==AMP_SCALE) { //using amp values
	    val=MAX_COLOR_VALUE - (int)(pow(ats_sound->amp[j][i], valexp) *(float)MAX_COLOR_VALUE);
	  }
	  else { //using smr values
	    val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(pow(db2amp_spl(ats_sound->smr[j][i]), valexp) *(float)MAX_COLOR_VALUE);
	  }
	  change_color(val,val,val);
	}
	if(ats_sound->frq[j][i] >= (float)v.viewstart && ats_sound->frq[j][i] <= (float)v.viewend) {
	  ffac1=(float)v.diff / (ats_sound->frq[j][i]- (float)v.viewstart); 
	  cury=main_graph->allocation.height -(main_graph->allocation.height / ffac1);
	  if(interpolated == TRUE) { //INTERPOLATING OR NOT
	    if(ats_sound->frq[j][i+1]==0.) nexty = cury; //special case
	    else {
	      ffac2=(float)v.diff / (ats_sound->frq[j][i+1]- (float)v.viewstart);
	      nexty=main_graph->allocation.height-(main_graph->allocation.height / ffac2);
	    }
	    gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)nexty);
	  }
	  else {
	    //NEXTY NOT USED
	    gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)cury);
	  }
	  
	}	 
	vant=val; 
      }
    }
    curx+=frame_step;      
  }  
  //dump pixmap on screen
  repaint(NULL);
  return;
}

////////////////////////////////////////////////////////////////////////////
void draw_pixm()//draws the spectrum on a pixmap
{
  int  i,j, val, vant;
  float curx, cury, nextx ,nexty;
  float ffac1, ffac2=0.; 
  float x_line=-1.;
  float band_step;
  float linear_res;
  //erase previous graph
  gdk_draw_rectangle(pixmap,main_graph->style->white_gc,
		     1,0,0,main_graph->allocation.width,main_graph->allocation.height);
  //init values
  curx=nextx=cury=nexty=0.;
  /*
  h.diff      ranges from 0 to frames-1 and equals (h.viewend - h.viewstart)       
  h.viewstart ranges from 1 to frames  
  h.viewend   ranges from 1 to frames
  */
  frame_step=(float)main_graph->allocation.width / ((float)h.diff+1.);
  freq_step=(float)main_graph->allocation.height / (float) v.diff+1.;

  //NOW DRAW ACCORDING view_type...
  
  switch (view_type) {
  case SON_VIEW:
    {////////////////////////////////////////////////////////////////////

      curx=nextx=cury=nexty=0.;
  
      for(i=(int)h.viewstart-1; i < (int)h.viewend; ++i) {
	
	nextx+=frame_step;
	
	for(j=0; j < (int)atshed.par; ++j) {
	  if(ats_sound->amp[j][i] > 0. ) {
	    if(scale_type==AMP_SCALE) { //using amp values
	      val=MAX_COLOR_VALUE - (int)(pow(ats_sound->amp[j][i], valexp) *(float)MAX_COLOR_VALUE);
	    }
	    else { //using smr values
	      val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(pow(db2amp_spl(ats_sound->smr[j][i]), valexp) *(float)MAX_COLOR_VALUE);
	    }
	    if(selected[j]==1 && i >= selection.from && i <= selection.to) {
	      if(scale_type==AMP_SCALE) {
		val=MAX_COLOR_VALUE - (int)(ats_sound->amp[j][i] * (float)MAX_COLOR_VALUE);
	      }
	      else { //using smr values
		val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(db2amp_spl(ats_sound->smr[j][i]) *(float)MAX_COLOR_VALUE);
	      }
	      change_color(val,0,0);
	      }
	      else {
		change_color(val,val,val);
	      }

	      if(ats_sound->frq[j][i] >= (float)v.viewstart && ats_sound->frq[j][i] <= (float)v.viewend) {
		ffac1=(float)v.diff / (ats_sound->frq[j][i]- (float)v.viewstart); 
		cury=main_graph->allocation.height -(main_graph->allocation.height / ffac1);
		if(interpolated == TRUE) { //INTERPOLATING OR NOT
                  if(ats_sound->frq[j][i+1]==0.) nexty = cury; //handle special case
		  else {
		    ffac2=(float)v.diff / (ats_sound->frq[j][i+1]- (float)v.viewstart);
		    nexty=main_graph->allocation.height-(main_graph->allocation.height / ffac2);
		  }
		  gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)nexty);
		}
		else {
		  //NEXTY NOT USED
		  gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)cury);
		}

	      }	 
	      vant=val; 
	  }
	}
	if(STARTING_SELECTION && i == selection.from) {//catch x location of selection line
	  x_line=curx;
	}
	curx+=frame_step;      
      }  
      break;
    }//SONOGRAM
 
  case RES_VIEW:
    {////////////////////////////////////////////////////////////////////
    
      //init values
      curx=nextx=nexty=0.;
      band_step=main_graph->allocation.height/(float)ATSA_CRITICAL_BANDS; 

      for(i=h.viewstart-1; i < h.viewend; ++i) {  

	nextx+=frame_step;
	cury=main_graph->allocation.height;
	
	for(j=0; j <ATSA_CRITICAL_BANDS; ++j) {
	  linear_res= (float)sqrt((float)ats_sound->band_energy[j][i]/
				  ((float)atshed.ws * (float)ATSA_NOISE_VARIANCE));
	  val=MAX_COLOR_VALUE - (int)((float)pow(linear_res, valexp) * (float)MAX_COLOR_VALUE);
	  change_color(val,val,val);
	  cury -= band_step;
	  gdk_draw_rectangle(pixmap,draw_pen,TRUE,(int)curx,(int)cury,(int)frame_step+1,(int)band_step+1);
	}
	curx+=frame_step; 
      }
      break;
    }//RESIDUAL VIEW

  case SEL_ONLY:
    {////////////////////////////////////////////////////////////////////
       curx=nextx=cury=nexty=0.;
       for(i=h.viewstart-1; i < h.viewend; ++i) {
	 nextx+=frame_step;
	 for(j=0; j < (int)atshed.par; ++j) {
	   
	   if(selected[j]==1 && i >= selection.from && i <= selection.to && ats_sound->amp[j][i] > 0.) {
	     if(scale_type==AMP_SCALE) {
	       val=MAX_COLOR_VALUE - (int)(ats_sound->amp[j][i] * (float)MAX_COLOR_VALUE);
	     }
	     else { //using smr values
		val=ats_sound->smr[j][i] < 0.0? MAX_COLOR_VALUE : MAX_COLOR_VALUE - (int)(db2amp_spl(ats_sound->smr[j][i]) *(float)MAX_COLOR_VALUE);
	      }  
	     change_color(val,0,0);  
	     if(ats_sound->frq[j][i] >= (float)v.viewstart && ats_sound->frq[j][i] <= (float)v.viewend ) {
	       ffac1=(float)v.diff / (ats_sound->frq[j][i]- (float)v.viewstart); 
	       cury=main_graph->allocation.height -(main_graph->allocation.height / ffac1);
	       if(interpolated == TRUE) { //INTERPOLATING OR NOT
		 if(ats_sound->frq[j][i+1]==0.) nexty = cury; //special case
		 else {
		   ffac2=(float)v.diff / (ats_sound->frq[j][i+1]- (float)v.viewstart);
		   nexty=main_graph->allocation.height-(main_graph->allocation.height / ffac2);
		 }
		 gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)nexty);
	       }
	       else {
		 //NEXTY NOT USED
		 gdk_draw_line(pixmap,draw_pen,(int)curx,(int)cury,(int)nextx,(int)cury);
	       }
	       
	       }	 
	       vant=val;
	   }
	 }
	 if(STARTING_SELECTION && i == selection.from) {//catch x location of selection line
	   x_line=curx;
	 }
	 curx+=frame_step;      
       }
       break;
    }//SELECTED VIEW
    
  }//END CASE
  
  
  if(STARTING_SELECTION && x_line >= 0.) {
    draw_selection_line( x_line );
  }
  repaint(NULL);
  return;
}
////////////////////////////////////////////////////////////////////////////
void repaint(gpointer data)
{
  //repaint the widget
  update_rect.x=0;
  update_rect.y=0;
  update_rect.width=main_graph->allocation.width;
  update_rect.height=main_graph->allocation.height;
  gtk_widget_draw(main_graph,&update_rect);
}
////////////////////////////////////////////////////////////////////////////

void draw_default(GtkWidget *widget) //draws the default screen on a pixmap
{
  int i, x = 0, height = widget->allocation.height, val = 16384;
  int xdif = widget->allocation.width / 100, width = widget->allocation.width;

  for(i=0; i<50; i++) {
    change_color(val/2, 0, val);
    gdk_draw_rectangle(pixmap, draw_pen, 1, x, 0, width, height);
    x += xdif;
    width -= (2*xdif);
    val += (MAX_COLOR_VALUE-16384) / 50;
  }
}
/////////////////////////////////////////////////////////////////////////////////
void update_value(GtkAdjustment *adj)
{  
  if(floaded) {
    valexp=adj->value / 100.;
    draw_pixm();
  }
}
/////////////////////////////////////////////////////////////////////////////////
void set_spec_view(void) //SPECTRAL view switch between AMP & SMR plot
{
  if(floaded) {
    gtk_widget_show(GTK_WIDGET(vscale1));
    gtk_widget_show(GTK_WIDGET(vscale2));
    view_type=SON_VIEW;
    gtk_ruler_set_range (GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
			 (float)v.viewstart/1000.,(float)v.diff/1000.);
    gtk_widget_show(GTK_WIDGET(vruler));
    scale_type=AMP_SCALE;
    draw_pixm();
  }
}
/////////////////////////////////////////////////////////////////////////////////
void set_smr_view(void) //SPECTRAL view switch between AMP & SMR plot
{
  if(floaded) {
    gtk_widget_show(GTK_WIDGET(vscale1));
    gtk_widget_show(GTK_WIDGET(vscale2));
    view_type=SON_VIEW;
    gtk_ruler_set_range (GTK_RULER (vruler),(float)v.viewend/1000.,(float)v.viewstart/1000.,
			 (float)v.viewstart/1000.,(float)v.diff/1000.);
    gtk_widget_show(GTK_WIDGET(vruler));
    scale_type=SMR_SCALE;
    if(!smr_done) atsh_compute_SMR(ats_sound, 0, atshed.fra);
    draw_pixm();
  }
}


/////////////////////////////////////////////////////////////////////////////////
void set_res_view(void) //Show residual data, if any, on a Bark scale
{
  if(floaded && (FILE_HAS_NOISE)) {
    gtk_widget_hide(GTK_WIDGET(vscale1));
    gtk_widget_hide(GTK_WIDGET(vscale2));
    gtk_widget_hide(GTK_WIDGET(vruler));
    view_type=RES_VIEW;
    interpolated=FALSE;//for sonogram return
    draw_pixm();
  }
}
///////////////////////////////////////////////////////////
void set_interpolated_view(void) //switch between interpolated & non-interpolated
{
  if(floaded) {
    if(interpolated) interpolated=FALSE;
    else interpolated=TRUE;
    draw_pixm();
  }
}

///////////////////////////////////////////////////////////
gint motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
int x,y,rband;
GdkModifierType state;

 if(floaded) {
   gdk_window_get_pointer(event->window, &x, &y, &state); //get new position
   if(x <= main_graph->allocation.width && x > 0 &&
      y <=main_graph->allocation.height && y > 0) { //print only if in region
     *info=0;
     frame_step=(float)main_graph->allocation.width / (float) (h.diff+1.);
     //position.from store always the horizontal position of the mouse...
     position.from= (h.viewstart-1) + (int)((float) x / frame_step);     
     //we must now track the vertical position of the mouse pointer
     //according the view_type
     if(VIEWING_DET) { 
       freq_step=(float)main_graph->allocation.height / (float) v.diff;
       position.f1  = v.viewstart + (v.diff - (int)((float) y / freq_step));
       
       if(scale_type==AMP_SCALE) sprintf(info,"Sinusoidal Amplitude Plot:  FRAME %d (%.3f s)    FREQ %d Hz", 
                                         position.from+1, 
                                         ats_sound->time[0][position.from], 
                                         position.f1);
       else sprintf(info,"Sinusoidal SMR Plot:  FRAME %d (%.3f s)    FREQ %d Hz", 
                    position.from+1, 
                    ats_sound->time[0][position.from], 
                    position.f1);
       
       gtk_ruler_set_range (GTK_RULER (vruler),(float)v.viewend / 1000.,
                            (float)v.viewstart / 1000.,
                            (float)position.f1 / 1000.,
                            (float)v.diff / 1000.);  		  
     }
     if(VIEWING_RES) {
       freq_step=main_graph->allocation.height / (float)ATSA_CRITICAL_BANDS;
       rband    = ATSA_CRITICAL_BANDS-(int)((float)y /freq_step); 
       position.from= (h.viewstart-1) + (int)((float) x / frame_step);     
       
       sprintf(info,"Noise Plot:  FRAME %d (%.3f s)    BAND %d (%d-%d Hz)    ENERGY %.5f", 
               position.from+1,
               ats_sound->time[0][position.from], 
               rband,
               (int)res_data[rband-1],(int)res_data[rband],
               ats_sound->band_energy[rband-1][position.from] ); 
     }
   }
   gtk_statusbar_pop(GTK_STATUSBAR(statusbar), context_id);
   gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, info);
   
   set_hruler((double)h.viewstart, (double)h.viewend, (double)position.from, (double)h.diff); 
   //get new position
   x=event->x;
   y=event->y;
   state=event->state;   
 }
   return(TRUE);
}
/////////////////////////////////////////////////////////////// 
void unzoom()
{
  if(floaded) {
  
    //init only horizontal scalers 
    set_hruler((double)1.,(double)atshed.fra,(double)1.,(double)atshed.fra);
  
    h.viewstart=1;
    h.viewend=(int)atshed.fra;
    h.diff=h.viewend - h.viewstart;      
    h.prev=h.diff; 
    h_setup();
    draw_pixm();
  }
}
///////////////////////////////////////////////////////////////
void zoom_sel()
{ 
  if(floaded==0 || view_type==RES_VIEW) { return; }
  if(NOTHING_SELECTED) {return;} //nothing selected
 
 h.viewstart=selection.from + 1;
 h.viewend  =selection.to + 1;
 h.diff     =(selection.to - selection.from); 
 h.prev     = h.viewend;

 gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj2),((float)selection.to) - ((float)selection.from - 1.)); 
 gtk_adjustment_set_value(GTK_ADJUSTMENT(hadj1),(float)selection.from +  1.);
 draw_pixm();

 set_hruler((double)h.viewstart, (double)h.viewend, (double)position.from, (double)h.diff);
}
///////////////////////////////////////////////////////////////
void sel_only()
{
  int i, temp=0;

  if(floaded==0 || view_type==RES_VIEW) { return; }
 
  for(i=0; i<(int)atshed.par; i++) { //check whether something is selected
    if (selected[i]==1) ++temp;
  }
  if(temp==0) { return; } //nothing selected...do nothing  
  
  if(view_type==SEL_ONLY) {view_type=SON_VIEW;}
  else{view_type=SEL_ONLY;}

  draw_pixm();
}

///////////////////////////////////////////////////////////////
gint click(GtkWidget *widget, GdkEventButton *event) 
{
  int i,j, temp, index=0;
  int prevfrom, prevto;
  GdkModifierType state;
  float gap=0., mingap=0.;

 if(floaded==0 || view_type==RES_VIEW ) {return TRUE;} 
  if (event->button == 1 && pixmap != NULL)
 state=event->state;   

if (event->button == 1 && pixmap != NULL) {

 if(vertex1==0) {
   vertex1=1;
   vertex2=0;
   
   prevfrom=selection.from; //store previous from FRAME value
   prevto  =selection.to;   //store previous to   FRAME value
   
   //g_print("\nprevfrom=%d prevto=%d\n", prevfrom, prevto);

   selection.from=position.from;
   selection.f1  =position.f1;
   selection.x1=(int)event->x;
   selection.y1=(int)event->y;
   selection.width=main_graph->allocation.width;

   for(i=0; i<(int)atshed.par; i++) {
     selected[i]=0;
   }
   
   if(view_type == SEL_ONLY) {
     sel_only(); //switch to view_all
   }
   
   else {
     erase_selection(prevfrom, prevto); 
   }
   return TRUE;
 }
 
 if(STARTING_SELECTION) {
   vertex2=1;
   vertex1=0;

   selection.x2=(int)event->x;
   selection.y2=(int)event->y;

   selection.to= position.from; 
 
   if(selection.to < selection.from) {
     SWAP_INT(selection.to, selection.from)
       }
   
   selection.f2= position.f1; 
   if(selection.f2 < selection.f1) {
     SWAP_INT(selection.f1, selection.f2)
       }
   if(selection.x2 < selection.x1) {
     SWAP_INT(selection.x1, selection.x2)
       }

   //set_avec(selection.to - selection.from);
   
   for(i=0; i<(int)atshed.par; ++i) { //deselect all
     selected[i]=0;
   }
   if(selection.from != selection.to) {
     temp=0;
     for(i=selection.from; i < selection.to; ++i) {
       for(j=0; j < (int)atshed.par; ++j) {     
	 if(selected[j]==0) {
	   if((int)ats_sound->frq[j][i] >= selection.f1 && 
	      (int)ats_sound->frq[j][i] <= selection.f2) {
	     selected[j]=1;
	     ++temp;
	   }        
	 }
       }
       if(temp == (int)atshed.par-1) { break; }
     }
   }
   else { //just one frame
     for(j=0; j < (int)atshed.par; ++j) {     	
	   if((int)ats_sound->frq[j][selection.from] >= selection.f1 && 
	      (int)ats_sound->frq[j][selection.from] <= selection.f2) {
	     selected[j]=1;
	   }        
       }   
   }
   //if we are here something is selected
   //previous selection (if any) is supposed to be erased previously
   //so, we should only redraw CURRENT selection
   if(view_type == SEL_ONLY) {
     sel_only(); //switch to view_all
   }
   else {
	draw_selection();
   }
}
return TRUE;
} //Button 1
////////////////////////////
else {//Either Button 2 or 3
	 
  if(STARTING_SELECTION) { //do nothing
    return TRUE;
  } 
  mingap	=	atshed.mf;
  for(i=0; i<(int)atshed.par; ++i) { //locate the closer frame and partial     
    gap= fabs(position.f1 -(ats_sound->frq[i][(int)position.from]));
    if(gap < mingap) {
      mingap=gap;
      index = i;
    }
  }
  if(SOMETHING_SELECTED) {
    if(position.from < selection.from || position.from > selection.to) {
      vertex1=0;
      vertex2=0;
      for(i=0; i<(int)atshed.par; i++) {
	selected[i]=0;
      }
    }
  }
  if(NOTHING_SELECTED) { //set selection WIDTH to all
    for(i=0; i<(int)atshed.par; i++) {
      selected[i]=0;
    }
    set_selection(h.viewstart, h.viewend,0,main_graph->allocation.width,
		  main_graph->allocation.width);
    vertex1=0; vertex2=1; //now something IS selected
    //set_avec(selection.to - selection.from);
  }
	
  if(selected[index]==0)	
    selected[index]=1;
  else
    selected[index]=0;
  
  draw_selection();
	
} //Either Button 2 or 3

return TRUE;
}
///////////////////////////////////////////////////////////
void sel_all()
{
int i;

if(floaded==0 || view_type==RES_VIEW) { return; }

for(i=0; i<(int)atshed.par; i++) { //select all
     selected[i]=1;
}

set_selection(h.viewstart, h.viewend,0,main_graph->allocation.width,
		     main_graph->allocation.width);
vertex1=0; vertex2=1; //something IS selected
//set_avec(selection.to - selection.from);
draw_pixm(); 
}
//////////////////////////////////////////////////////////
void sel_un()
{
int i;

if(floaded==0 || view_type==RES_VIEW) { return; }

for(i=0; i<(int)atshed.par; i++) { //deselect all
     selected[i]=0;
   }
 vertex1=vertex2=0; //nothing selected 
 set_selection(0,0,0,0,main_graph->allocation.width);
 selection.f1=selection.f2=0;
 draw_pixm(); 
 //set_avec(selection.to - selection.from);
}
//////////////////////////////////////////////////////////
void sel_even()
{
int i;
if(floaded==0 || view_type==RES_VIEW) { return; }
 
 for(i=0; i<(int)atshed.par; i++) { //select all
   selected[i]=1;
 }
 
 set_selection(h.viewstart, h.viewend,0,main_graph->allocation.width,
		     main_graph->allocation.width);
 for(i=0; i<(int)atshed.par-1; i+=2) { //DEdeselect all odd
   selected[i]=0;
   
}
 vertex1=0; vertex2=1; //something IS selected
 //set_avec(selection.to - selection.from);
 draw_pixm(); 
}
//////////////////////////////////////////////////////////
void sel_odd()
{
int i;
if(floaded==0 || view_type==RES_VIEW) { return; }

 for(i=0; i<(int)atshed.par; i++) { //deselect all
   selected[i]=0;
 }
 set_selection(h.viewstart, h.viewend,0,main_graph->allocation.width,
		     main_graph->allocation.width);
 for(i=0; i<(int)atshed.par-1; i+=2) { //select all odd
   selected[i]=1;
   
}

 vertex1=0; vertex2=1; //something IS selected
 //set_avec(selection.to - selection.from);
 draw_pixm(); 
}
//////////////////////////////////////////////////////////
void revert_sel() 
{

int i;
if(floaded==0 || view_type==RES_VIEW) { return; }
 if(NOTHING_SELECTED) {return;} 

 for(i=0; i<(int)atshed.par; i++) { //revert
   
   selected[i]+=1;
   if(selected[i] > 1) {
     selected[i]=0;
   }
 }
 draw_pixm(); 
}
//////////////////////////////////////////////////////////
void set_selection(int from, int to, int x1, int x2, int width)
{

  selection.from=from-1;
  selection.to  =to-1;
  selection.x1  =x1;
  selection.x2  =x2;
  selection.width  =width;
}
//////////////////////////////////////////////////////////
void draw_selection_line(int x)
{
change_color(0,0,50000);
gdk_draw_line(pixmap,draw_pen,x,0,x,(int)main_graph->allocation.height);
}
//////////////////////////////////////////////////////////
void set_hruler(double from, double to, double pos, double max)
{
  gtk_ruler_set_range (GTK_RULER (hruler),
                       from,
                       to  + 1.,
                       pos + 1.5,
                       max + 1. );
}
