/*
ATSH-UNDO.C
Oscar Pablo Di Liscia / Juan Pampin
 */

#include "atsh.h"

extern ATS_SOUND *ats_sound;
extern UNDO_DATA *undat;
extern int floaded, scale_type, aveclen;
extern short smr_done;
extern char undo_file[];
int ned = 0, led = 1, undo = TRUE;
extern SELECTION selection, position;
extern ATS_HEADER atshed;

////////////////////////////////////////////////////////////////////////////
void do_undo(GtkWidget *widget,gpointer data)
{
  int aflag;
  FILE *pundo;
  int seek_point=0; 
  int i,x;
  short shaux;
  double daux;

  if(undo==FALSE) {return;}
  if(floaded==FALSE || ned == 0) {return;} 

  aflag=GPOINTER_TO_INT(data); 
  led +=aflag;
  if(led < 0) {led=0; ned=1;}
  if(led == ned) {led=ned-1;} 

  if((pundo=fopen(undo_file,"rb"))==NULL) {
    Popup("Could not open the UNDO backup file: /nUNDO DISABLED");
    undo=FALSE;
    return;
  }
  fseek(pundo, (long int)seek_point, SEEK_SET); 
  //find out where data starts and store it in seek_point
  
  for(i=0; i < led; ++i){
    seek_point += (int)(atshed.par * sizeof(short));
    seek_point += (int)((undat[i].to - undat[i].from) + 1 ) * sizeof(double) * undat[i].nsel;
  }

  //go in seek_point bytes 
  fseek(pundo, (long int)seek_point, SEEK_SET);
  //read the backup data
  for(i=0; i < (int)atshed.par; ++i) {
    if(fread(&shaux,1,sizeof(short),pundo)==0) {
      Popup("Could not read on backup file: \nUNDO DISABLED");
      fclose(pundo);
      undo=FALSE;
      return;
    }
    selected[i]=shaux;
    //g_print(" %d ", selected[i]);
  }
  //g_print("retrieving(%d): from=%d to=%d \n",ned,undat[led].from,undat[led].to);
  
    for(i=undat[led].from; i < undat[led].to+1; ++i) { 
    
      for(x = 0; x < (int)atshed.par; ++x) {
	if(selected[x]==TRUE) {
	  if(fread(&daux,1,sizeof(double),pundo)==0) {
	    Popup("Could not read on backup file:(2) \nUNDO DISABLED");
	    //g_print("\n led= %d ned=%d \n", led, ned);
	    fclose(pundo);
	    undo=FALSE;
	    return;
	  }
	  switch (undat[led].ed_kind) {
	  case AMP_EDIT:
	    ats_sound->amp[x][i]=daux;
	    break;
	  case FRE_EDIT:
	    ats_sound->frq[x][i]=daux;
	    break;
	  }
	}
      }
    }    
 
 
 selection.from=undat[led].from;
 selection.to=undat[led].to;
 selection.f1=undat[led].f1;
 selection.f2=undat[led].f2;
 vertex1=FALSE; vertex2=TRUE; //something IS selected  
 // set_avec(selection.to - selection.from);
 fclose(pundo);
 if(scale_type==SMR_SCALE) { //smr values are computed only if the user is viewing them
   atsh_compute_SMR(ats_sound,selection.from,selection.to);
 }
 else {
   smr_done=FALSE;
 }
 draw_pixm(); 
 repaint(NULL);

 return;
}
////////////////////////////////////////////////////////////////
void backup_edition(int eddie)
{
  FILE *pundo;
  int i, x;
  short shaux;
  double daux;

  if(undo==FALSE) {return;}
  pundo=fopen(undo_file,"ab");
  if(pundo==0) {
    Popup("Could not open the UNDO backup file: \nUNDO DISABLED");
    undo=FALSE;
    return;
  }
fseek(pundo, 0L, SEEK_END);
//store edition data
 undat=(UNDO_DATA*)realloc(undat,( ned + 1 ) * sizeof(UNDO_DATA));
 
 undat[ned].from=selection.from;
 undat[ned].to  =selection.to;
 undat[ned].f1  =selection.f1;
 undat[ned].f2  =selection.f2;
 undat[ned].ed_kind= eddie;
 undat[ned].nsel  =0;
 //g_print("saving(%d): from=%d to=%d",ned,undat[ned].from,undat[ned].to);
 //write first the selected partials data
 for(i = 0; i < (int)atshed.par; ++i) {
   shaux=selected[i];
   if(selected[i]==TRUE) {
     ++undat[ned].nsel;
   }
   if(fwrite(&shaux,1,sizeof(short),pundo)==FALSE) {
     Popup("Could not write on backup file: \nUNDO DISABLED");
     fclose(pundo);
     undo=FALSE;
     return;
   }
 }
 
 //if changed, write either amplitude or frequency data
 //ONLY selected partials data are written 
   for(i=0; i < aveclen; ++i) {
     
     for(x=0; x < (int)atshed.par; ++x) {
       if (selected[x]==TRUE){ 
	 switch (undat[ned].ed_kind) {
	 case AMP_EDIT:
	   daux=ats_sound->amp[x][selection.from + i];
	   break;
	 case FRE_EDIT:
	   daux=ats_sound->frq[x][selection.from + i];
	   break;
	 }
	 if(fwrite(&daux,1,sizeof(double),pundo)==FALSE) {
	   Popup("Could not write on backup file: \nUNDO DISABLED");
	   fclose(pundo);
	   undo=FALSE;
	   return;
	 }
       }
     }
   }
 

 //
 ++ned;    //update editions counter
 led=ned;  //set the edition level to NED
 fclose(pundo);
 return;
}
////////////////////////////////////////////////////////////////









