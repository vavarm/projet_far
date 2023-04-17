#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  
  printf("Début programme\n");
  int nb_octet_recu = 0;
  int nb_octet;
  int s;
  int dS = socket(PF_INET, SOCK_STREAM, 0); 
  printf("Socket Créé\n");


  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ;
  int b = bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
  if (b == -1){
    printf("ERROR : bind, le port est peut être déjà utilisé\n");
    exit(0);
  }
  printf("Socket Nommé\n");

  int l = listen(dS, 7) ;
  if (l == -1){
    printf("ERROR : listen\n");
    exit(0);
  }
  printf("en attente de connexion\n");

  struct sockaddr_in aC ;
  socklen_t lg = sizeof(struct sockaddr_in) ;
  int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
  printf("Client Connecté\n");
  printf("x-----------------------------------x\n");
  sleep(10);
  char msg [50] ;
  int x = 10 ;
  nb_octet = recv(dSC, msg, 50, 0) ;
  nb_octet_recu += nb_octet;
  printf("| Message reçu : \t \t \t \t%s\n", msg) ;
  printf("|----%d octet recu \n", nb_octet);
  printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);
  
  if (strncmp(msg, "shutdown", 8) == 0){
      x = 11;
      s = send(dSC, &x, sizeof(int), 0) ;
      if (s == -1){
        printf("ERROR : send\n");
        exit(0);
      }
      shutdown(dSC,2) ;
      shutdown(dS,2) ;
      printf("Fin du programme \n");
      exit(0);
  }
  
  s = send(dSC, &x, sizeof(int), 0) ;
  if (s == -1){
      printf("ERROR : send\n");
      exit(0);
    }
  printf("X-----------------------------------X\n");

  while (strncmp(msg, "shutdown", 8) != 0){
    nb_octet = recv(dSC, msg, 50, 0) ;
    
    printf(" strncmp = : %d \n", strncmp(msg, "shutdown", 8));
    printf(" msg = %s \n", msg);

    for (int i = 0; i < 50; i++){
      printf("%c \n", msg[i]);
    }
    if (strncmp(msg, "shutdown", 8) == 0){
      break;
    }
    nb_octet_recu += nb_octet;
    printf("| Message reçu : \t \t \t \t %s\n", msg) ;
    printf("|----%d octet recu \n", nb_octet);
    printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);
    
    int s = send(dSC, &x, sizeof(int), 0) ;
    if (s == -1){
      printf("ERROR : send\n");
      exit(0);
    }
    printf("X-----------------------------------X\n");
  }

  printf("Message reçu : %s\n", msg) ;
  x = 11 ;
  s = send(dSC, &x, sizeof(int), 0) ;
  if (s == -1){
      printf("ERROR : send\n");
      exit(0);
    }

  shutdown(dSC,2) ;
  shutdown(dS,2) ;
  printf("\t \t 🛑 Fin du programme 🛑\n");
}