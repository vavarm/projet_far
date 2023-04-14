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
  int test_error;

  int dS = socket(PF_INET, SOCK_DGRAM, 0); 
  printf("Socket Créé\n");

  struct sockaddr_in ad;
  ad.sin_family = AF_INET;
  ad.sin_addr.s_addr = INADDR_ANY ;
  ad.sin_port = htons(atoi(argv[1])) ;
  test_error = bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
  if (test_error == -1){
    printf("ERROR : bind, le port est peut être déjà utilisé\n");
    exit(0);
  }
  printf("Socket Nommé\n");

  struct sockaddr_in aE ;
  socklen_t lg = sizeof(struct sockaddr_in) ;
  
  printf("x-----------------------------------x\n");

  sleep(10);
  //================================== RECEPTION DU PREMIER MESSAGE ====================================
  char msg [30] ;
  int x = 10 ;
  nb_octet = recvfrom(dS, msg, sizeof(msg), 0, (struct sockaddr*)&aE, &lg) ;
  nb_octet_recu += nb_octet;
  printf("| Message reçu : \t \t \t \t%s\n", msg) ;
  printf("|----%d octet recu \n", nb_octet);
  printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);
  

  //=================================== SI LE MESSAGE EST SHUTDOWN =================================
  
  if (strncmp(msg, "shutdown", 8) == 0){
      x = 11;
      test_error = sendto(dS, &x, sizeof(int), 0, (struct sockaddr*)&aE, lg) ;
      if (test_error == -1){
        printf("ERROR : send\n");
        exit(0);
      }
      close(dS);
      printf("Fin du programme \n");
      exit(0);
  }
  
  test_error = sendto(dS, &x, sizeof(int), 0, (struct sockaddr*)&aE, lg) ;
  if (test_error == -1){
      printf("ERROR : send\n");
      exit(0);
    }
  printf("X-----------------------------------X\n");

  //=============================== TANT QUE LE MESSAGE N'EST PAS SHUTDOWN =================================
  // continu de recevoir des messages
  while (strncmp(msg, "shutdown", 8) != 0){
    nb_octet = recvfrom(dS, msg, sizeof(msg), 0, (struct sockaddr*)&aE, &lg)  ;
    if (strncmp(msg, "shutdown", 8) == 0){
      break;
    }
    nb_octet_recu += nb_octet;
    printf("| Message reçu : \t \t \t \t %s\n", msg) ;
    printf("|----%d octet recu \n", nb_octet);
    printf("|----%d octet recu depuis le debut de la connexion\n", nb_octet_recu);
    
    test_error = sendto(dS, &x, sizeof(int), 0, (struct sockaddr*)&aE, lg) ;
    if (test_error == -1){
      printf("ERROR : send\n");
      exit(0);
    }
    printf("X-----------------------------------X\n");
  }

  //=================================== FERME QUAND LE MESSAGE EST SHUTDOWN =================================

  printf("Message reçu : %s\n", msg) ;
  x = 11 ;
  test_error = sendto(dS, &x, sizeof(int), 0, (struct sockaddr*)&aE, lg) ;
  if (test_error == -1){
      printf("ERROR : send\n");
      exit(0);
    }

  close(dS);
  printf("\t \t 🛑 Fin du programme 🛑\n");
}