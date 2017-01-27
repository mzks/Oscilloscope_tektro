#include <stdio.h>
#include <stdlib.h>
#include "COscillo.h"
#include <TGraph.h>
#include <TFile.h>

int main(int argc, char *argv[]){
  int n_event = 1;
  int n_channel=4;
  char input_fname[1024]="TDS";

  fprintf(stderr,"***convert***\n");
  fprintf(stderr,"convert ch(1-4) num [in_filename(head)]\n");
  /**commad line parameters**/
  if(argc>1)    n_event=atoi(argv[1]);
  if(argc>2)    n_channel=atoi(argv[2]);
  if(argc>3)    sprintf(input_fname,"%s",argv[3]);
  fprintf(stderr,"num of events:%d\n",n_event);
  fprintf(stderr,"num of channel:%d\n",n_channel);
  fprintf(stderr,"input file:%s_*_ch?.tds\n",input_fname);
  fprintf(stderr,"output file:%s_*_ch?.root\n",input_fname);
  
  COscillo oscillo("dummy");
  TGraph gr;
  oscillo.ConvertToTGraph(input_fname, n_event, n_channel, &gr);
  //oscillo.ConvertToTGraph(argv[3], &gr);
  oscillo.Close();
  fprintf(stderr,"\ndone.\n");
  //  TFile file("out.root", "recreate");
  //gr.Write("graph");
  //file.Close();
    return 0; 
}
