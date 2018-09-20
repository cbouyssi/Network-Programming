#include "serveur_UDP2.h"


#define BUFSIZE 1024
#define RCVSIZE 1024
#define SNDSIZE 1024
#define ACKSIZE 7

int main (int argc, char *argv[]) {


  if(argc != 2){
    printf("Nombres de parametres invalide \n");
    exit(0);
  }


  struct sockaddr_in adresse, adresse_data, client ;
  int port_co= atoi(argv[1]);
  int port_data=port_co + 1;
  char port_data_str[15];
  sprintf(port_data_str,"%d", port_data);
  int valid= 1;
  

  struct timeval RTT;


  socklen_t alen= sizeof(client);

  char buffer[BUFSIZE];
  FRAG message;
  message = (FRAG)malloc(sizeof(struct frag));


  //create socket UDP
  int desc_co= socket(AF_INET, SOCK_DGRAM, 0);
  int desc_data= socket(AF_INET, SOCK_DGRAM, 0);

  if (desc_co < 0) {
    perror("cannot create socket\n");
    return -1;
  }

  if (desc_data < 0) {
    perror("cannot create socket\n");
    return -1;
  }

  setsockopt(desc_co, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
  setsockopt(desc_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));

  adresse.sin_family= AF_INET;
  adresse.sin_port= htons(port_co);
  adresse.sin_addr.s_addr= htonl(INADDR_ANY);

  adresse_data.sin_family= AF_INET;
  adresse_data.sin_port= htons(port_data);
  adresse_data.sin_addr.s_addr= htonl(INADDR_ANY);

  if (bind(desc_co, (struct sockaddr*) &adresse, sizeof(adresse)) == -1) {
    perror("Bind fail\n");
    close(desc_co);
    return -1;
  }

  if (bind(desc_data, (struct sockaddr*) &adresse_data, sizeof(adresse_data)) == -1) {
    perror("Bind fail\n");
    close(desc_data);
    return -1;
  }

  memset(buffer,0,RCVSIZE);
  fd_set fdset;


  int pid;

  while (1) {
    //Activation des descripteurs
    FD_ZERO(&fdset);
    FD_SET(desc_co, &fdset);
    select(desc_co+1, &fdset, NULL, NULL, NULL);

    if(FD_ISSET(desc_co, &fdset)){
      printf("mon PID : %d\n", getpid());
      desc_data= socket(AF_INET, SOCK_DGRAM, 0);
      setsockopt(desc_data, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
      port_data++; // pour faire en sorte que les connections ne s'ouvrent pas sur les mêmes ports
      adresse_data.sin_family= AF_INET;
      adresse_data.sin_port= htons(port_data);
      adresse_data.sin_addr.s_addr= htonl(INADDR_ANY);
      if (bind(desc_data, (struct sockaddr*) &adresse_data, sizeof(adresse_data)) == -1) {
        perror("Bind fail\n");
        close(desc_data);
        return -1;
      }
      sprintf(port_data_str,"%d", port_data);
      printf("le nouveau port devient : %s\n", port_data_str);
      int k = tripleHandShake (desc_co, buffer, port_data_str, &client, alen, &RTT);
      data_exchange(desc_data, message, client, alen, buffer, RTT);
      k=0;
    }

    
      

    
  }
}

int data_exchange(int desc_data, FRAG message, struct sockaddr_in  client, socklen_t alen, char buffer[], struct timeval RTT){
  printf("mon pid : %d\n", getpid());
  int fast;
  int ack = 0;
  int maxAck = 0;
  int ret;
  fd_set fd_in;
  struct timeval rtt;
  struct timeval start,end;
  int cont =1;
  int seq = 1;
  int i=0;
  int window=15;


  char nomfichier[BUFSIZE];
  memset(nomfichier,0,BUFSIZE);               
  recvfrom(desc_data,nomfichier,RCVSIZE,0, (struct sockaddr*) &client, &alen);
  FILE * file;
  file=fopen(nomfichier, "rb");
  if(file==NULL){
    printf("Error opening file\n");
    exit(0);
  }

  //on place tout le fichier dans le buffer
  fseek(file,0,SEEK_END);// on se place à la fin du fichier
  long sizeOfFile=ftell(file); //donne taille fichier car ftell donne position courante
  fseek(file,0,SEEK_SET);
  int finalSeq=sizeOfFile/DATASIZE;
  int sizeFinalSeq=sizeOfFile%DATASIZE;
  if(sizeFinalSeq!=0){
    finalSeq++;
  }

  while(cont){
    FD_ZERO( &fd_in );
    FD_SET( desc_data, &fd_in );
    rtt.tv_sec = 0;
    rtt.tv_usec =1000;
    ret = select( desc_data + 1 , &fd_in, NULL, NULL, &rtt ); 

    if ( ret == -1 ){
      //printf("Houston we've got a problem\n");  // report error and abort
    }
    else if(ret==0){ //PAQUET PERDU
      fast = maxAck + 1;
      seq = maxAck + 1; //penser à tester si dernier paquet
      sprintf(message->seq,"%06d", fast);
      fseek(file,maxAck*DATASIZE,SEEK_SET);
      fread(message->data, DATASIZE, 1, file);
      //memcpy(message->data, bufFile+1018*(maxAck), 1018);
      sendto(desc_data,message,1460,0, (struct sockaddr*) &client, alen);
      //printf("TIMEOUT on reprend à la séquence : %d\n", seq);
      if (window > 10){
        window = (int)window/2;
        i = 0;
      }
      if(i>0){
        i--; 
      }         
    }

    if ( FD_ISSET(desc_data, &fd_in) && i == window){     //ACK RECU
      memset(buffer,0, BUFSIZE);
      ack = rcvACK(desc_data, client, alen, buffer);
      //printf("On a reçu l' ACK : %d\n", ack);
      window++;
      if(ack > maxAck){
        maxAck = ack;
      }
      if(i>0){
        i--;
      }

      if(maxAck > seq){
        seq = maxAck + 1;
      }
      else if(maxAck == finalSeq ){
        //printf("recu last ack \n");
        
        for(cont = 0; cont <10; cont++){
            sendto(desc_data,"FIN\0", 4,0,(struct sockaddr*) &client, alen);
        }
        cont = 0;
        exit(0);
      }
    }
    while((i < window) && cont ){
      memset(message,0,sizeof(message));
      sprintf(message->seq,"%06d", seq);
      if(seq == finalSeq){
        i=window;
	fseek(file,(finalSeq-1)*DATASIZE,SEEK_SET);
      	fread(message->data, sizeFinalSeq, 1, file);
        //printf("envoie last seq \n");
        sendto(desc_data,message,sizeFinalSeq+6,0, (struct sockaddr*) &client, alen);
        break;
      }else{
	  fseek(file,(seq-1)*DATASIZE,SEEK_SET);
       	  fread(message->data, DATASIZE, 1, file);
          sendto(desc_data,message,1460,0, (struct sockaddr*) &client, alen);
          //printf("ma Fenetre : %d\n", window);
         // printf("ou je suis dans ma Fenetre : %d\n", i);
         // printf("On envoie la séquence : %d\n", seq);
      }
      if(seq < finalSeq){
        seq++;
      }
      i++;
    }
  }
  close(desc_data);
  return 0;
}




int tripleHandShake (int desc_co, char buffer[], char* port_data, struct sockaddr_in * client, socklen_t alen, struct timeval * RTT){
  struct timeval start,end;
  int msgSize= recvfrom(desc_co,buffer,RCVSIZE,0, (struct sockaddr*) client, &alen);
  //printf("UDP_CO : %s\n", buffer);
  if(strcmp(buffer,"SYN\0") == 0){
    char syn_ack_port[12];
    memset(syn_ack_port, 0, 12);
    strcat(syn_ack_port, "SYN-ACK");
    strcat(syn_ack_port, port_data);
    if(gettimeofday(&start,NULL)) {
      //printf("time failed\n");
      exit(1);
    }
    sendto(desc_co,syn_ack_port ,SNDSIZE,0, (struct sockaddr*) client, alen);
  }
  memset(buffer,0,RCVSIZE);
  msgSize=recvfrom(desc_co,buffer,RCVSIZE,0, (struct sockaddr*) client, &alen);
  if(gettimeofday(&end,NULL)) {
    //printf("time failed\n");
    exit(1);
  }
  if(strcmp(buffer,"ACK\0") == 0){
    memset(buffer,0,RCVSIZE);
    if(end.tv_usec<start.tv_usec)
      RTT->tv_usec =end.tv_usec+(1000000-start.tv_usec);
    else
      RTT->tv_usec =end.tv_usec-start.tv_usec;
    //printf("Mon RTT : %d \n", RTT->tv_usec);
    return 1;
  }
}

int rcvACK(int desc_data, struct sockaddr_in client, socklen_t alen, char * buffer){
  char ACK[ACKSIZE];
  memset(ACK, 0, ACKSIZE);
  int size = recvfrom(desc_data, buffer, RCVSIZE,0,(struct sockaddr*) &client, &alen);
  strcat(ACK, buffer+3);
  return atoi(ACK);
}

