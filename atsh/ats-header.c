/*
ATS-HEADER.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"
///////////////////////////////////////////////////////////////////////
void create_hsep(GtkWidget *vbox)
{
  GtkWidget *hsep1;

  hsep1=gtk_hseparator_new();
  gtk_widget_ref (hsep1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "hsep1", hsep1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (vbox), hsep1, TRUE, TRUE,0);
  gtk_widget_show (hsep1);  

return;
}
///////////////////////////////////////////////////////////////////////
void hclose(GtkWidget *widget, gpointer data)
{

  free(hdata);
  gtk_widget_destroy (GTK_WIDGET (window1));
  return;
}


///////////////////////////////////////////////////////////////////////
void show_header (void)
{

  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *label1, *label2,*label3,*label4,*label5,*label6,*label7,*label8,*label9,*label10,
    *label11;
  char *ndata;
  int n1=2, n2=0, length=0;

  if(floaded==FALSE) {return;}
  
  hdata=(char*)malloc(1024 * sizeof(char)); 
  ndata=(char*)malloc(32 * sizeof(char)); 

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
  gtk_window_set_title (GTK_WINDOW (window1), ("Data of ATS File "));
  gtk_window_set_policy (GTK_WINDOW (window1), FALSE, TRUE, TRUE);
  gtk_window_set_position(GTK_WINDOW(window1),GTK_WIN_POS_CENTER);
  gtk_widget_show(window1);
  gtk_signal_connect (GTK_OBJECT (window1), "destroy",
			GTK_SIGNAL_FUNC (hclose),NULL);


  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window1), vbox1);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (window1), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);
  //////////////////////////////////////////////////////////////
  *hdata=0;
  strcat(hdata,"FILENAME: ");
  strcat(hdata,ats_tittle);
  label1 = gtk_label_new (hdata);
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox2), label1, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
  
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////

 if(FILE_HAS_PHASE) {
   n1+=1;
 }
 if(FILE_HAS_NOISE) {
   n2+=1;
 }

 length =  sizeof(ATS_HEADER) +
   ((int)atshed->par * (int)atshed->fra * sizeof(double) * n1) +
   (sizeof(double) * (int)atshed->fra) + 
   (NB_RES * (int)atshed->fra * sizeof(double) * n2);


 *hdata=0;
 *ndata=0;
 strcat(hdata,"FILE SIZE: ");
 sprintf(ndata,"%d",length); 
 strcat(hdata,ndata); 
 strcat(hdata," bytes");
 label2 = gtk_label_new (hdata);
 gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
 gtk_widget_ref (label2);
 gtk_object_set_data_full (GTK_OBJECT (window1), "label2", label2,
			   (GtkDestroyNotify) gtk_widget_unref);
 gtk_widget_show (label2);
 gtk_box_pack_start (GTK_BOX (vbox2), label2, TRUE, TRUE, 10);
 create_hsep(vbox2);
  ///////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"SAMPLING RATE: ");
  sprintf(ndata,"%d",(int)atshed->sr); 
  strcat(hdata,ndata); 
  strcat(hdata," Hz.");
  label3 = gtk_label_new (hdata);
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_box_pack_start (GTK_BOX (vbox2), label3, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"FRAME SIZE: ");
  sprintf(ndata,"%d (%4.3f secs.)",(int)atshed->fs,atshed->fs / atshed->sr ); 
  strcat(hdata,ndata); 
  label4 = gtk_label_new (hdata);
  gtk_widget_ref (label4);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label4", label4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label4);
  gtk_box_pack_start (GTK_BOX (vbox2), label4, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"WINDOW SIZE: ");
  sprintf(ndata,"%d",(int)atshed->ws); 
  strcat(hdata,ndata); 
  label5 = gtk_label_new (hdata);
  gtk_widget_ref (label5);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label5", label5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label5);
  gtk_box_pack_start (GTK_BOX (vbox2), label5, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label5), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"PARTIALS PER FRAME: ");
  sprintf(ndata,"%d",(int)atshed->par); 
  strcat(hdata,ndata); 
  label6 = gtk_label_new (hdata);
  gtk_widget_ref (label6);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label6", label6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label6);
  gtk_box_pack_start (GTK_BOX (vbox2), label6, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label6), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"FRAMES: ");
  sprintf(ndata,"%d",(int)atshed->fra); 
  strcat(hdata,ndata); 
  label7 = gtk_label_new (hdata);
  gtk_widget_ref (label7);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label7", label7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label7);
  gtk_box_pack_start (GTK_BOX (vbox2), label7, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label7), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"DURATION: ");
  sprintf(ndata,"%7.3f",(float)atshed->dur); 
  strcat(hdata,ndata); 
  strcat(hdata," secs.");
  label8 = gtk_label_new (hdata);
  gtk_widget_ref (label8);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label8", label8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label8);
  gtk_box_pack_start (GTK_BOX (vbox2), label8, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label8), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"MAX. AMP.: ");
  sprintf(ndata,"%5.3f",(float)atshed->ma); 
  strcat(hdata,ndata); 
  label9 = gtk_label_new (hdata);
  gtk_widget_ref (label9);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label9", label9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label9);
  gtk_box_pack_start (GTK_BOX (vbox2), label9, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"MAX. FREQ.: ");
  sprintf(ndata,"%7.2f",(float)atshed->mf); 
  strcat(hdata,ndata); 
  strcat(hdata," Hz.");
  label10 = gtk_label_new (hdata);
  gtk_widget_ref (label10);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label10", label10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label10);
  gtk_box_pack_start (GTK_BOX (vbox2), label10, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label10), GTK_JUSTIFY_LEFT);
  create_hsep(vbox2);
  //////////////////////////////////////////////////////////////////
  *hdata=0;
  *ndata=0;
  strcat(hdata,"TYPE: ");
  sprintf(ndata,"%d",(int)atshed->typ); 
  strcat(hdata,ndata); 
  switch ((int)atshed->typ) {
  case 1:
    strcat(hdata,"   (Only Amplitude and Frequency data on file)   ");
    break;
  case 2:
    strcat(hdata,"   (Amplitude, Frequency, and Phase data on file)  ");
    break;
  case 3:
    strcat(hdata,"   (Amplitude, Frequency, and Residual Analysis data on file)   ");
    break;
  case 4:
    strcat(hdata,"   (All Amplitude, Frequency, Phase, and Residual Analysis data on file)   ");
    break;  
  }

  label11 = gtk_label_new (hdata);
  gtk_widget_ref (label11);
  gtk_object_set_data_full (GTK_OBJECT (window1), "label11", label11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label11);
  gtk_box_pack_start (GTK_BOX (vbox2), label11, TRUE, TRUE, 10);
  gtk_label_set_justify (GTK_LABEL (label11), GTK_JUSTIFY_LEFT);

  gtk_grab_add (window1);
  free(ndata);

  return;
}

