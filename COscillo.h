#ifndef COscillo_H
#define COcsillo_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "TSocket.h"
#include "TString.h"
#include "TGraph.h"
#include "TFile.h"

class COscillo
{
public:
  COscillo(const char* h, int p=80){host=h; port=p;}
  ~COscillo(){Close();}

  //  bool GetWaveForm(const char* file);
  bool ConvertToTGraph(const char* file, int n_event,int n_channel,TGraph *gr);
  // bool GetWaveFormByTGraph(TGraph *gr);
  void Close(){};
  
private:
  const char* host;
  int port;
  int iNumPoints;            /* The number of points in the array   */
  double dXIncr;              /* Delta X between points */
  double dYMult;
  double dYOff;
  double dPtoff;
  double dYZero;
  
  //  void SkipHeader(FILE* fp);
  bool GetParamDouble(char* buf, char* pat, double *dValue);
  bool GetParamInteger(char* buf, char* pat, int *iValue);
  bool AdvanceToData(int iHandle);
  bool ScaledSimple2 (int iInFileHandle, TGraph *gr);
};


/**void COscillo::SkipHeader(FILE* fp)
{
  int line_size = 0;
  
  while(true){
    switch(fgetc(fp)){
    case '\r':
      break;
    case '\n':
      if(line_size == 0){
	return;
      }else{
	line_size = 0;
	break;
      }
    default:
      line_size++;
      break;
    }
  }
}
**/
 /**
bool COscillo::GetWaveForm(const char* file)
{
  TSocket socket(host, port);

  TString str = "POST /getwmf.isf HTTP/1.1\r\n";
  str += "Host: "; str+=host; str+="\r\n";
  str += "Connection: close\r\n";
  str += "\r\n";
  str += "command=select:ch1 on\r\n";
  str += "command=save:waveform:fileformat internal\r\n";
  str += "wfmsend=Get\r\n";

  socket.SendRaw(str.Data(), str.Length());

  TString tmpFile(file);
  tmpFile.Append("~");

  FILE* fp = fopen(tmpFile.Data(), "wb");
  
  const int size = 5120;
  char buf[size];
  int len = socket.RecvRaw(buf, size);

  while(len>0){
    fwrite(buf, sizeof(char), len, fp);
    len = socket.RecvRaw(buf, size);
  }
  socket.Close();
  fclose(fp);

  char cHead[9];
  int  iRes;
  FILE* fp_in = fopen(tmpFile, "rb");

  while(true){
    fscanf(fp_in, "%s %d", cHead, &iRes);  
    if(iRes == 100){
      this->SkipHeader(fp_in);
    }else if(iRes >= 200 && iRes < 300){
      this->SkipHeader(fp_in);
      break;
    }else{
      printf("error, unkown responce code, %s %d\n", cHead, iRes);
      return false;
    }
  }
 
   FILE* fp_out = fopen(file, "wb");
   len = fread(buf, sizeof(char), size, fp_in);
  while(len > 0 || !feof(fp_in)){
    fwrite(buf, sizeof(char), len, fp_out);
    len = fread(buf, sizeof(char), size, fp_in);
  }
  fclose(fp_in);
  fclose(fp_out);
  
  remove(tmpFile.Data());
  
  return true;
}
 **/

bool COscillo::GetParamDouble(char* buf, char* pat, double *dValue)
{
  char* found = strstr(buf, pat);
  if(found == NULL) return false;
  found += strlen(pat);
  *dValue = atof(found);
  return true;
}

bool COscillo::GetParamInteger(char* buf, char* pat, int *iValue)
{
  char* found = strstr(buf, pat);
  if(found == NULL) return false;
  found += strlen(pat);
  *iValue = atoi(found);

  return true;
}

bool COscillo::AdvanceToData(int iHandle)
{
  char c;
  int iStat;
  int iNumDigits;

  /* Find the '#' the marks the binary field */
  c = ' ';
  while (c != '#'){
    iStat = read (iHandle, &c, 1);
    if (iStat != 1){
      printf ("*** Error - reading the input file.\n");
      return false;
    }
  }

  /* The value in variable 'c' is the ASCII for the number of digits
     to skip over.
   */
  iStat = read (iHandle, &c, 1);
  if (iStat != 1){
    printf ("*** Error - reading the input file.\n");
    return false;
  }
  
  iNumDigits = (int) (c - '0');
  while (iNumDigits > 0){
    iStat = read (iHandle, &c, 1);
    if (iStat != 1){
      printf ("*** Error - reading the input file.\n");
      return false;
    }
     --iNumDigits;
  }
  return true;
}

bool COscillo::ScaledSimple2 (int iInFileHandle, TGraph *gr)
{
  int lLcvPoint;       /* Used to step through the points */
  char c[2];           /* Buffer to read the point        */
  double dPoint;
  double dTime;
  int iStat;

  for (lLcvPoint = 0; lLcvPoint < iNumPoints; lLcvPoint++){
    iStat = read (iInFileHandle, c, 2);
    if (iStat != 2){
      printf ("*** Error - read data points.\n");
      return false;
    }

    short sPoint = (c[1]&0xFF) | ((c[0]&0xFF)<<8);
    dPoint = dYZero + dYMult * ((double)sPoint  - dYOff);
    dTime = dXIncr * ((double) lLcvPoint - dPtoff);
    //iStat = fprintf (fOutFile, "%lg,%g\n", dTime, dPoint);
    //if (iStat < 0){
    //  printf ("*** Error - writing output file.\n");
    //  return false;
    //}
    gr->SetPoint(lLcvPoint, dTime, dPoint);
  }
  return true;
}

bool COscillo::ConvertToTGraph(const char* file, int n_event,int n_channel,TGraph *gr)
{
  char infname[128],outfname[128];
  int iBit_NR = 16;
  int debug=0;
  int index,ch;
  for(index=0;index<n_event;index++){
    fprintf(stderr,"converting %d  / %d\r",index+1,n_event);
    for(ch=0;ch<n_channel;ch++){
      sprintf(infname,"%s_%d_ch%d.tds",file,index,ch+1);
      int iHandle = open (infname, O_RDONLY);
      // get header;
      char buf[512];
      read(iHandle, buf, 512);
      buf[511] = '\0';

      sprintf(outfname,"%s_%d_ch%d.root",file,index,ch+1);
      if(debug)fprintf(stderr,"converting %s to %s.",infname,outfname);
    
      lseek (iHandle, 0L, SEEK_SET);
    if(debug)fprintf(stderr,".");  
    //bool bEnvelop = false;      /* True if this is an envelop */
    if(!this->GetParamInteger(buf, "NR_PT", &iNumPoints)){
      if(debug)fprintf(stderr,"NumPoints %d\n", iNumPoints);
    return false;
  }
  if(!this->GetParamDouble(buf, "XINCR", &dXIncr)) return false;
  //printf("XIncr %g\n", dXIncr);
  if(!this->GetParamDouble(buf, "YMULT", &dYMult)) return false;
  //printf("YMult %g\n", dYMult);
  if(!this->GetParamDouble(buf, "YOFF", &dYOff)) return false;
  //printf("YOff %g\n", dYOff);
  if(!this->GetParamDouble(buf, "PT_OFF", &dPtoff)) return false;
  //printf("PT_Off %g\n", dPtoff);
  if(!this->GetParamDouble(buf, "YZERO", &dYZero)) return false;
  //printf("YZero %g\n", dYZero);
  if(!this->GetParamInteger(buf, "BIT_NR", &iBit_NR)) return false;
  //printf("Bit %d\n", iBit_NR);
  
  // move to data start point
  if(!this->AdvanceToData(iHandle)) return false;  
  if(debug)fprintf(stderr,"numofpoints=%d\n", iNumPoints);
  //convert data
  //TGraph gr(iNumPoints);
  if(debug)fprintf(stderr,"iBIT=%d\n",iBit_NR);
  
  gr->Set(iNumPoints);
  
  switch(iBit_NR){
  case 8:
    //ScaledSimple (iInFileHandle, &gr);
    break;
  case 16:
    this->ScaledSimple2 (iHandle, gr);
    break;
  default:
    break;
  }
  TFile tfile(outfname, "RECREATE");
  (*gr).Write("graph");
   tfile.Close();
   if(debug)fprintf(stderr,"done");  
   close(iHandle);
    }


  }
  return true;

}

#endif // COcsillo_H
