#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

int init_connect(char *,int );
int url_encode(char *, char *, int);
int setStopAfter(char *,char *);
int getStopAfter(char *,char *, int );
int setSingleSequence(char *);
int isSingleSequence(char *);
int setRunStop(char *);
int isRunStop(char *);
int setState(char *,char *);
int getState(char *,char *, int );
int setStateRun(char *);
int isStateRun(char *);
int setStateStop(char *);
int isStateStop(char *);
int sendCommand(char *,char *, char *, int );
int getWaveform(char *,int , char *, int ,int);
int strip_http_headers(char *, int);
int debug=0;

int init_connect(char *remotehostname,int port)

{
  int     sock;
  static  struct  sockaddr_in sock_name;
  struct  hostent             hostentstruct;  /* Storage for hostent data.  */
  struct  hostent             *hostentptr;    /* Pointer to hostent data.   */
  static  char                hostname[256];  /* Name of local host.        */
  int     retval;                             /* helpful for debugging */
  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	perror( "socket");
	return -1;
  }
  if ((hostentptr = gethostbyname (remotehostname)) == NULL) {
	perror( "gethostbyname");
	close(sock);
	return -1;
  }
  hostentstruct = *hostentptr;
  sock_name.sin_family = hostentstruct.h_addrtype;
  sock_name.sin_port = htons(port);
  sock_name.sin_addr = * ((struct in_addr *) hostentstruct.h_addr);
  retval = connect(sock,(struct sockaddr *)(&sock_name), sizeof (sock_name));
  if (retval != 0) {
	perror("connect");
	close(sock);
	return -1;
  }
  return sock;
}

int url_encode(char *src, char *dst, int len){
  int i;
  int srclen = strlen(src);
  if (len < srclen*3){
	return -1; /* too short to store everything */
  }  
  for(i = 0 ; i < srclen ; i++){
	snprintf((dst+(i*3)),4,"%%%.2x",src[i]);
  }
//fprintf(stderr,"url_encode()\n srclen -> %d\n dst -> %s\n",srclen,dst);
  return (srclen*3);
} 

int setStopAfter(char *hostname,char *strMode)
{
  int ret;
  char cmd_str[1024]="ACQuire:STOPAfter \0";  
  if (strlen(strMode) > 1000){
    /**	return NULL;**/
    return 0;
  }
  strncat(cmd_str, strMode, 1024);
  ret = sendCommand(hostname, cmd_str, NULL, 0);
  
  return ret;
  
}

int getStopAfter(char *hostname, char *buf, int maxlen)
{
  int ret;
  char cmd_str[1024]="ACQuire:STOPAfter?\0";
  ret = sendCommand(hostname, cmd_str, buf, maxlen);
  return ret;
}

int setSingleSequence(char *hostname)
{
  int ret;
  ret = setStopAfter(hostname, "SEQuence\0");
}

int isSingleSequence(char *hostname)
{
  int ret;
  char status[1024];
  int  len = 1024;
  ret = getStopAfter(hostname, status, len);
  if (strstr(status,"SEQ") == status){
	printf("In the single SEQ.\n");
	return 1;
  }else{
	printf("Not in the single SEQ.\n");
	return 0;
  }

}

int setRunStop(char *hostname)
{
  int ret;
  ret = setStopAfter(hostname, "RUNSTop\0");

  return ret;
  
}
  
int isRunStop(char *hostname)
{
  int ret;
  char status[1024];
  int  len = 1024;

  ret = getStopAfter(hostname, status, len);

  if (strstr(status,"RUNSTop") != status){
	return 1;
  }else
	return 0;
}

int setState(char *hostname, char *state)
{
  int ret;
  char cmd_str[1024]="ACQuire:STATE \0";
//fprintf(stderr,"setState()\n cmd_str = %s\n state = %s\n",cmd_str,state);
  
  if (strlen(state) > 1000){
    /**	return NULL;**/
    return 0;
  }
  strncat(cmd_str, state, 1024);
//fprintf(stderr," cmd_str after -> %s\n state -> %s\n",cmd_str,state);
  ret = sendCommand(hostname, cmd_str, NULL, 0);
//fprintf(stderr," ret=sendCommand -> %d\n",ret);
  
  return ret;
  
}

int
getState(char *hostname, char *buf, int maxlen)
{
  int ret;
  char cmd_str[1024]="ACQuire:STATE?\0";

  ret = sendCommand(hostname, cmd_str, buf, maxlen);
//fprintf(stderr,"getState()\n ret -> %d\n");
  return ret;

}

int
setStateRun(char *hostname)
{
  int ret;

  ret = setState(hostname, "RUN");
//fprintf(stderr,"setStateRun : %d\n",ret);

  return ret;
  
}

int
isStateRun(char *hostname)
{
  int ret;
  char status[1024];
  int  len = 1024;

  ret = getState(hostname, status, len);

  if (  (strstr(status,"1") == status)
	  ||(strstr(status,"RUN") == status)
	  ||(strstr(status,"ON") == status)){
	return 1;
  }else
	return 0;
}

int
setStateStop(char *hostname)
{
  int ret;
  ret = setState(hostname, "STOP");
//fprintf(stderr,"setStateStop()\n ret -> %d\n",ret);
  return ret;  
}

int
isStateStop(char *hostname)
{
  int ret;
  char status[1024];
  int  len = 1024;

  ret = getState(hostname, status, len);
//fprintf(stderr,"isStateStop()\n ret -> %d\n hostname -> %s\n len -> %d\n",ret,hostname,len);
    //fprintf(stderr,"status %s\n",status);
  if (  (strstr(status,"0") == status)
	||(strstr(status,"STOP") == status)
	||(strstr(status,"OFF") == status)){
	return 1;
  }else
	return 0;
}

int
sendCommand(char *hostname,char *cmd_str, char *buf, int maxlen)
{
  int fd;
  char url_string[4096];
  char tmp_str[4096];
  char ack='\n';
  char *msg,*lasts;

  int ret,len;

  int flag;

#define FALSE 0
#define TRUE  1

  fd = init_connect(hostname,81);
//fprintf(stderr,"sendCommand()\n cmd_str -> %s\n tmp_str -> %s\n fd -> %d\n",cmd_str,tmp_str,fd);
  ret = url_encode(cmd_str, tmp_str, 4000);
//fprintf(stderr,"1st ret -> %d\n fd -> %d\n",ret,fd);
  snprintf(url_string,4096,"GET /?command=%s HTTP/1.1\n",tmp_str);
//fprintf(stderr,"url_string -> %s\n",url_string);
  ret = send(fd,url_string,strlen(url_string),0);
//fprintf(stderr," 2nd ret -> %d\n",ret);
  
  if (ret < 0){
	perror("Failed to send URL request.");
	return ret;
  }
//fprintf(stderr," 3rd ret -> %d\n fd -> %d\n tmp_str -> %s\n",ret,fd,tmp_str);
  
  memset(tmp_str,0,sizeof(tmp_str));
//fprintf(stderr,"after memset tmp_str -> %d\n",tmp_str);
  ret = recv(fd,tmp_str,4096,0);
//fprintf(stderr," 4th ret -> %d\n",ret);

  len = 0;
  if (strstr(tmp_str,"Continue") != NULL){
    send(fd,&ack,1,0);
    while( (4096-len) > 0){
      ret = recv(fd,tmp_str+len,4096-len,0);
      if (ret <=0 ) break;
      len += ret;
    }
  }
//fprintf(stderr,"5th ret -> %d\n",ret);
  flag = FALSE;
  
  msg = strtok_r(tmp_str,"\r\n",&lasts);
  while(flag == FALSE){
    if (  (strstr(msg,"HTTP/1.1") == msg)
	  ||(strstr(msg,"Date:")    == msg)
	  ||(strlen(msg)            ==   0)){
      msg = strtok_r(NULL,"\r\n",&lasts);	  
      if (msg == NULL){
	flag = TRUE;
	ret = 0;
	  }
	}else{
	  flag = TRUE;
	  ret  = strlen(msg);
	}
  }

  if (ret > maxlen){
	len = maxlen;
  }else{
	len = ret;
  }

  if (len >0){
	memcpy(buf,msg,len);
  }
  shutdown(fd,2);
  close(fd);
//fprintf(stderr,"last ret -> %d\n",ret);
  return ret;
}

int getWaveform(char *hostname,int ch, char *buf, int maxlen,int outmode)
{
  int ret=0;
  int len=0;
  int fd;
  int i;
  char ack='\n';
  int content_length=0;
#define NCMD 6
#define CMDLEN 256
  char command_string[NCMD*CMDLEN];
  char command_strings[NCMD][CMDLEN]
    = {"POST /getwmf.isf HTTP/1.1\r\n\0",
       "Host:\0"
       "Connection: close\r\n\r\n",
       "command=select:ch%d on\r\n\0",
       "command=save:waveform:fileformat internal\r\n\0",
       "wfmsend=Get\r\n\0"};
  memset(command_string,0,sizeof(command_string));
  fd = init_connect(hostname,81);
  if(debug)   fprintf(stderr,"fd=%d\n",fd);
  snprintf(command_strings[1],256,"Host: %s\r\n\0",hostname);
  snprintf(command_strings[3],256,"command=select:CH%d on\r\n\0",ch);  
  if(outmode)
  snprintf(command_strings[4],256,"command=save:waveform:fileformat spreadsheet\r\n\0");
  else
  snprintf(command_strings[4],256,"command=save:waveform:fileformat internal\r\n\0");
  snprintf(command_strings[5],256,"wfmsend=Get\r\n\0");
  
  for(i = 3 ; i < NCMD ; i++){
	content_length+=strlen(command_strings[i]);
  }
  snprintf(command_strings[2],256,"Connection: close\r\n\r\n");
  snprintf(command_string,sizeof(command_string),"%s%s%s%s%s%s",command_strings[0],command_strings[1],command_strings[2],command_strings[3],command_strings[4],command_strings[5]);
  
  if(debug)fprintf(stderr,"%s",command_string);
  
  ret = send(fd,command_string,strlen(command_string),0);
  if (ret < 0){
	perror("Failed to send URL request.");
	return ret;
  }
  else if(debug) fprintf(stderr,"communication with the host: OK\n");
  if(debug)  fprintf(stderr,"maxlen=%d len=%d\n",maxlen,len);  
  while( (maxlen-len) > 0){
	ret = recv(fd,buf+len,maxlen-len,0);
	if (ret <=0 ) break;
	if (strstr(buf+len,"Continue") != NULL){
	  send(fd,&ack,1,0);
	}
	len += ret;
  }
  if (ret < 0){
	perror("Failed to retrive waveform");
	return ret;
  }
  if(debug) fprintf(stderr,"Received length: %d\n",len);
  ret = strip_http_headers(buf,len);
  if(debug)  fprintf(stderr,"Data length: %d\n",ret);
  shutdown(fd,2);
  close(fd);
  return ret;
}

int
strip_http_headers(char *buf, int len)
{
  char *data_ptr = buf;
  int data_len   = len;
  char *ctmp,*lasts;
  char *httpmsgs[2]={NULL,NULL};
  httpmsgs[0] = strstr(data_ptr,"HTTP/1.1 200 OK");
  httpmsgs[1] = strstr(data_ptr,"HTTP/1.1 100 Continue");
  if (httpmsgs[0] == NULL){
	fprintf(stderr,"There seems to be some trouble in the response.\n");
  }
  data_ptr = (httpmsgs[0]>httpmsgs[1])?httpmsgs[0]:httpmsgs[1];
  data_ptr = strstr(data_ptr,"\r\n\r\n");
  data_ptr+=4;
  data_len -= (data_ptr-buf);
  memmove(buf,data_ptr,data_len);
  return data_len;
}

int main(int argc, char **argv)
{
  int ret;
  int fd[3];
  int i,thisch;
  int num_event =1;
  int n_channel = 4;
  int outmode =0;
  char output_fname[1024]="TDS";
  char output_fname_head[1024]="TDS";
  char hostname[256]="TDS\0";
  char str[1000];
  char buf[1024*1024];

  //fprintf(stderr,"***read_TDS***\n");
  //fprintf(stderr,"read_TDS ch(1-4) num [out_filename] [1/0 (output format 1 for ascii )][hoatname/IP]\n");
  /**commad line parameters**/
  if(argc>1)    num_event=atoi(argv[1]);
  if(argc>2)    n_channel=atoi(argv[2]);
  if(argc>3)    sprintf(output_fname_head,"%s",argv[3]);
  if(argc>4)    outmode=atoi(argv[4]);
  if(argc>5)    strcpy(hostname,argv[5]);
  fprintf(stderr,"num of events=%d\n",num_event);
  fprintf(stderr,"num of channels=%d\n",n_channel);
  if(outmode)
  fprintf(stderr,"output file=%s_*_ch?.csv (ascii mode)\n",output_fname_head);
  else
  fprintf(stderr,"output file=%s_*_ch?.tds (TDS internal mode)\n",output_fname_head);
  fprintf(stderr,"hostname=%s\n\n",hostname);

  /**initialize**/
  if(debug)  printf("Set the state to STOP.\n");
//fprintf(stderr,"main()\n ret = %d\n",ret);
  ret = setStateStop(hostname);
  //fprintf(stderr,"1st ret = %d\n",ret);
  if(debug) printf("Waiting for the state to be STOPPED\n");
  ret = 0;
//fprintf(stderr,"after waiting ret -> %d\n",ret);
  while (ret == 0){
    usleep(1);
    ret = isStateStop(hostname); 
//fprintf(stderr,"in while ret -> %d\n",ret);
    }
  if(debug) printf("Set the state to single seq.\n");
  ret = setSingleSequence(hostname);

  /**data loop**/
  for (i = 0 ; i < num_event ; i++){
    fprintf(stderr,"event %d/%d\r",i+1,num_event);

    if(debug)printf("Start the run\n");
    ret = setStateRun(hostname);
    if(debug)printf("Waiting for the trigger\n");
    if(debug)printf("(Waiting for the state to be STOPPED)\n");
    ret = isStateStop(hostname); 
    while (ret == 0){
      usleep(10);
      ret = isStateStop(hostname); 
      if(debug) printf(".");
      fflush(stdout);
    }
    if (debug)    printf("\n");
    if(debug) printf("Retriving the waveform.\r");
 
    for(thisch=0;thisch<n_channel;thisch++){
      if(debug)fprintf(stderr,"%d",thisch);
      if(outmode)
	sprintf(output_fname,"%s_%d_ch%d.cvs",output_fname_head,i,thisch+1);
	
      else
	sprintf(output_fname,"%s_%d_ch%d.tds",output_fname_head,i,thisch+1);
      fd[0] = open(output_fname,O_RDWR | O_CREAT,0644);
      if (fd[0] < 0){
	perror("Failed to open output file.");
	exit(1) ;
      }
      ret = getWaveform(hostname, thisch+1, buf, 1024*1024,outmode);
      ret = write(fd[0],buf,ret);
    }
    if(debug)    fprintf(stderr,"%s ",buf);

    if(debug)fprintf(stderr,"event number = %d\n",i+1);
    if (ret < 0){
      perror("Failed to dump data to disk..");
      close(fd[0]);
      exit(1);
    }
  close(fd[0]);

  }
  fprintf(stderr,"\ndone.\n");
  exit(0);

}

