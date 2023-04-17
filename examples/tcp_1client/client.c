#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

  printf("Début programme\n");
  int dS = socket(PF_INET, SOCK_STREAM, 0);
  printf("Socket Créé\n");

  struct sockaddr_in aS;
  aS.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
  aS.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;
  if (connect(dS, (struct sockaddr *) &aS, lgA) == -1){
    printf("ERROR : erreur de connexion \n");
    exit(0);
  }else{printf("Socket Connecté\n");}
  printf("x-----------------------------------x\n");
  


  char * m = malloc(50);
  int r = 10;
  fgets(m, 50, stdin);
  //printf("%s", m);
  while (r != 11) {
    int nb_octet = send(dS, m, strlen(m)+1 , 0) ;
    if (nb_octet == -1){
      printf("ERROR : send\n");
      exit(0);
    }
    printf("|---- %d octets envoyés\n ", nb_octet);
    int b = recv(dS, &r, sizeof(int), 0) ;
    if (b == -1){
      printf("ERROR : recv\n");
      exit(0);
    }
    printf("\t \t \t \t    %d\n", r) ;
    if(r==11){
      break;
    }
    printf("x-----------------------------------x\n");
    fgets(m, 50, stdin);
    
  }

  shutdown(dS,2) ;
  printf("x-----------------------------------x\n");
  printf("\t 🛑 Fin du programme 🛑\n");
}