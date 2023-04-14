#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

  printf("Début programme\n");
  int dS = socket(PF_INET, SOCK_DGRAM, 0);
  printf("Socket Créé\n");

  struct sockaddr_in aD;
  aD.sin_family = AF_INET;
  inet_pton(AF_INET,argv[1],&(aD.sin_addr)) ;
  aD.sin_port = htons(atoi(argv[2])) ;
  socklen_t lgA = sizeof(struct sockaddr_in) ;

  
  char * m = malloc(30);
  int r = 10;
  fgets(m, 30, stdin);

  while (r != 11) {
    int nb_octet = sendto(dS, m, strlen(m)+1 , 0, (struct sockaddr*)&aD, lgA) ;
    if (nb_octet == -1){
      printf("ERROR : send\n");
      exit(0);
    }
    printf("|---- %d octets envoyés\n ", nb_octet);
    int b = recvfrom(dS, &r, sizeof(int), 0, NULL, NULL) ;
    if (b == -1){
      printf("ERROR : recv\n");
      exit(0);
    }
    printf("\t \t \t \t    %d\n", r) ;
    if(r==11){
      break;
    }
    printf("x-----------------------------------x\n");
    fgets(m, 30, stdin);
    
  }

  close(dS) ;
  printf("x-----------------------------------x\n");
  printf("\t 🛑 Fin du programme 🛑\n");
}