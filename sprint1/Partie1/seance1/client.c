#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    if(argc!=4){
        printf("Arguments: adresse(127.0.0.1) port numClient(1 ou 2)\n");
        exit(0);
    }

    int numClient = atoi(argv[3]);

    if(numClient!=1 && numClient!=2){
        printf("numClient = 1 ou = 2");
        exit(0);
    }

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
  
    char msg[50];

    while(1){
        if(numClient==1){
            fgets(msg, 50, stdin);
            if(send(dS, msg, strlen(msg)+1, 0)==-1){
                shutdown(dS,2);
                exit(0);
            }
            if(strncmp(msg, "fin", 3)==0){
                shutdown(dS,2);
                exit(0);
            }
            if(recv(dS, &msg, strlen(msg)+1, 0)==-1){
                shutdown(dS,2);
                exit(0);
            }
            printf("Message reçu: %s\n", msg);
            if(strncmp(msg, "fin", 3)==0){
                shutdown(dS,2);
                exit(0);
            }
        }
        if(numClient==2){
            if(recv(dS, &msg, strlen(msg)+1, 0)==-1){
                shutdown(dS,2);
                exit(0);
            }
            printf("Message reçu: %s\n", msg);
            if(strncmp(msg, "fin", 3)==0){
                shutdown(dS,2);
                exit(0);
            }
            fgets(msg, 50, stdin);
            if(send(dS, msg, strlen(msg)+1, 0)==-1){
                shutdown(dS,2);
                exit(0);
            }
            if(strncmp(msg, "fin", 3)==0){
                shutdown(dS,2);
                exit(0);
            }
        }
    }
}